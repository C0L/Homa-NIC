import random
import math
import copy
import yaml
import argparse
import numpy as np
import matplotlib.pyplot as plt
import numpy as np

import statistics

import sys
sys.set_int_max_str_digits(0)

class Message:
    def __init__(self, id, length, startTime):
        self.id = id

        # For SRPT the priority is the length
        self.priority = length 
        self.length   = length

        self.startTime = startTime
        self.endTime   = 0

    def __repr__(self):
        return f'{self.id} : {self.priority}'

# class BisectedPriorityQueue():
#     def __init__(self, config):
#         # Fast on chip single-cycle priority queue
#         self.onChip  = PriorityQueue(config['onChipDepth'], "On Chip")
# 
#         # Slow off chip multi-cycle priority queue
#         self.offChip = PriorityQueue(config['offChipDepth'], "Off Chip")
# 
#         self.commDelay  = config['commDelay']
#         self.queueDelay = config['queueDelay']
# 
#         self.promoting = []
#         self.demoting  = []
# 
#         self.time = 0
# 
#     def enqueue(self, entry):
#         self.onChip.enqueue(entry)
# 
#     def dequeue(self):
#         return self.onChip.dequeue()
# 
#     def done(self):
#         return self.onChip.done() and self.offChip.done()
# 
#     def tick(self):
#         demote = self.onChip.demote()
# 
#         if demote != None:
#             self.demoting.append((self.time, demote))
# 
#         if self.time % self.queueDelay == 0:
#             promote = self.offChip.promote()
# 
#             if promote != None:
#                 self.promoting.append((self.time, promote))
# 
#         # TODO this should be made more realistic
# 
#         # Impose communication penalty dmoting on-chip to off-chip
#         if len(self.demoting) != 0:
#             head = self.demoting[0]
#             if self.time - head[0] > self.commDelay:
#                 self.offChip.enqueue(head[1])
#                 self.demoting.remove(head)
# 
#         # Impose communication penalty promoting off-chip to on-chip
#         if len(self.promoting) != 0:
#             head = self.promoting[0]
#             if self.time - head[0] > self.commDelay:
#                 self.promoting.remove(head)
#                 self.onChip.enqueue(head[1])
#                 demote = self.onChip.demote()
#                 if demote != None:
#                     self.demoting.append((self.time, demote))
# 
#         self.onChip.tick()
#         self.offChip.tick()
# 
#         self.time += 1
# 
#     def remains(self):
#         return self.onChip.remains() + self.offChip.remains()
# 
#     def stats(self):
#         self.onChip.stats()
#         self.offChip.stats()

class PriorityQueue:
    def __init__(self, size):
        self.size   = size  
        self.queue  = []
        self.record = []
        self.time   = 0

    def enqueue(self, entry):
        insert = next((e[0] for e in enumerate(self.queue) if e[1].priority > entry.priority), len(self.queue))
        self.queue.insert(insert, entry)


        # self.queue.append(entry)
        assert len(self.queue) <= self.size+1, \
            "Queue Overflow" 
        
    def dequeue(self):
        # If we are storing no entries there is no work to do 
        if (len(self.queue) == 0): return None

        # Find the highest priority element
        # ideal = min(self.queue, key=lambda entry: entry.priority)
        ideal = self.queue[0]

        # Reduce the priority as we send one packet
        ideal.priority = ideal.priority - 1
        
        # If the entry is done remove it
        if (ideal.priority <= 0):
            self.queue.remove(ideal)

        return ideal

    def trace(self):
        return self.record
    
    def tick(self):
        # print(self.queue[0:16])
        self.record.append(copy.copy(self.queue))
        self.time += 1

class Host:
    def __init__(self, config):
        self.size = config['onChipDepth']
        self.mid = 0
        self.time = 0

        # Generator for payloads and sizes
        self.generator = [self.mm1(r['arrival'], r['service'], r['token']) for r in config['rates']]
        self.events    = [next(gen) for gen in self.generator]

        self.queue = PriorityQueue(self.size)

    def tick(self):
        # Do we need to generate a new message from the poisson process?
        self.events = [next(fut) if curr.startTime < self.time else curr for fut, curr in zip(self.generator, self.events)]

        # If this cycle is when the new sendmsg request is ready 
        for event in self.events:
            if event.startTime <= self.time and event.priority != 0:
                self.queue.enqueue(event)

        self.queue.tick()
        self.time += 1

    def egress(self):
        entry = self.queue.dequeue()
        if (entry != None):
            return entry

        return None

    def mm1(self, arrival, service, token):
        # Poisson arrival times + poisson packet sizes for outgoing messages
        t = 0
        while(1):
            t += random.expovariate(arrival)
            # Compute the number of units of payload
            length = round(random.expovariate(service))

            message = Message(id=self.mid, length=length, startTime=round(t))

            self.mid += 1

            yield message

    # compute the average occupancy time of elements in the queue
    def flux(self, records):
        print("computing flux")
        runs = [[] for i in range(self.size)]

        # TODO this misses the very last trace

        # Iterate through slots
        for slot in range(self.size):
            run = 0
            last = None

            # Iterate through entries
            for record in records:
                current = None
                if len(record) > slot:
                    current = record[slot]

                if current != None:
                    if current == last: # is it a continuation
                        run += 1
                    else: # is it a new run
                        if run != 0:
                            runs[slot].append(run)
                        run = 1
                else:
                    if run != 0:
                        runs[slot].append(run)
                        run = 0

                last = current

        for run in reversed(runs[0:32]):
            if run != []:
                print(statistics.mean(run))

    def stats(self):
        records = self.queue.record
        avgDepth = statistics.mean([len(r) for r in records])
        print(avgDepth)
        self.flux(records)
        # for record in records:
            
            # print(len(record))

class Sim:
    def __init__(self, config):
        # Timestep counter
        global time
        time = 0

        # Initial ID to assign to new entries generated by the sendmsg function
        self.id = 0

        # Number of time steps to simulate for
        self.timeSteps = config['completions']

        self.host = Host(config)

        self.config = config

        self.duration = []

    def simulate(self):
        global time 
        # while True:
        for i in range(self.timeSteps):
            self.host.tick()
            packet = self.host.egress()

            if (packet != None and packet.priority <= 0):
                packet.endTime = time
                self.duration.append(time - packet.startTime)

            time += 1

        # remains = self.host.remains()
        remains = 0
        durations = np.array(self.duration)
        return durations, remains

    def stats(self):
        for rate in self.config['rates']:
            durations = np.array(self.duration)
            print(f'Host: 0, Rate: {rate["token"]}:')
            print(f'   - Total Complete Messages    : {durations.shape}')
            print(f'   - Avg. Cmpl. Time            : {durations.mean()}')
            print(f'   - Inaccuracies : ')
            print(f'   - Dry-fires    : ')
            self.host.stats()


if __name__ == '__main__':
    config = {
        'completions': 500000,
        # 'completions': 500000,
        'rates': [],
        'commDelay' : 140,
        'queueDelay': 40,
        'onChipDepth': 10,
        'offChipDepth': 1000000
    }

    # TODO will pull dist from file eventually

    fig, axs = plt.subplots(1, figsize=(6,4))
    fig.tight_layout(pad=3.0)

    onChipDepth = 10 
    config['onChipDepth'] = 2**onChipDepth

    xs = []
    mcts = []

    # for rate in range(1,100):
    # rate = .99
    rho = .99
    for mst in [1, 2, 4, 8, 16, 32, 64, 128, 256]:
        mu = 1/mst

        # desired utilization (vary rho from .1 -> .9)
        # rho = rate/100

        # Compute corresponding service rate
        lamda = rho * mu

        config['rates'] = [{'arrival' : lamda, 'service': mu, 'token': 'RPC', 'dict': 'Poisson'}]
        print(config)

        sim = Sim(config)
        result, rem = sim.simulate()

        # need to not count simulations which are unstable
        print(rem)
        xs.append(rho)
        mcts.append(result.mean())
        print(sim.stats())

        axs.plot(xs, mcts, label=f'Size: {2**onChipDepth}')

        axs.set_title(r'Mean Completition Time by Utilization (Mean Service Time = {mst})')
        axs.legend(loc='upper left', title='On Chip Queue Size')
        axs.set_ylabel('Mean Completition Time')
        axs.set_xlabel(r'Utilization $(\frac{\lambda}{\mu})$')
        axs.set_ylim(0,5)

        plt.savefig(f'mm1_bisected_{mst}.png')
        plt.plot(xs, mcts, label=f'Perfect')


    ''' Vary Utilization
    mst = 1

    mu = 1/mst

    # Iterate through utilization .1->.9
    for rate in range(1,100):
        # desired utilization (vary rho from .1 -> .9)
        rho = rate/100

        # Compute corresponding service rate
        lamda = rho * mu

        config['rates'] = [{'arrival' : lamda, 'service': mu, 'token': 'RPC', 'dict': 'Poisson'}]
        print(config)

        sim = Sim(config)
        result, rem = sim.simulate()

        # need to not count simulations which are unstable
        print(rem)
        xs.append(rho)
        mcts.append(result.mean())
        print(sim.stats())

        axs.plot(xs, mcts, label=f'Size: {2**onChipDepth}')

        axs.set_title(r'Mean Completition Time by Utilization (Mean Service Time = 1)')
        axs.legend(loc='upper left', title='On Chip Queue Size')
        axs.set_ylabel('Mean Completition Time')
        axs.set_xlabel(r'Utilization $(\frac{\lambda}{\mu})$')
        axs.set_ylim(0,5)

    plt.savefig(f'mm1_bisected.png')
    plt.plot(xs, mcts, label=f'Perfect')
    '''




'''
if __name__ == '__main__':
    config = {
        'completions': 500000,
        'rates': [],
        'commDelay' : 140,
        'queueDelay': 40,
        'onChipDepth': 10,
        'offChipDepth': 1000000
    }

    fig, axs = plt.subplots(1, figsize=(6,4))
    # fig, axs = plt.subplots(9, figsize=(8,32))
    fig.tight_layout(pad=3.0)
    # fig.tight_layout(pad=1.0)
    ax = 0

    # Iterate through mean service times
    for mst in range(1,2):
    # for mst in range(1,10):
        mu = 1/mst

        depths = list(range(1,6))
        depths.append(18)
        for onChipDepth in depths:
            config['onChipDepth'] = 2**onChipDepth

            xs = []
            mcts = []

            # Iterate through utilization .1->.9
            # for rate in range(1,2):
            for rate in range(1,100):
                # desired utilization (vary rho from .1 -> .9)
                rho = rate/100

                # Compute corresponding service rate
                lamda = rho * mu

                config['rates'] = [{'arrival' : lamda, 'service': mu, 'token': 'RPC', 'dict': 'Poisson'}]
                print(config)

                # need to not count simulations which are unstable
                
                sim = Sim(config)
                result, rem = sim.simulate()
                if rem < 100:
                    print(rem)
                    xs.append(rho)
                    mcts.append(result.mean())
                print(sim.stats())

            axs.plot(xs, mcts, label=f'Size: {2**onChipDepth}')

            axs.set_title(r'Mean Completition Time by Utilization (Mean Service Time = 1)')
            axs.legend(loc='upper left', title='On Chip Queue Size')
            axs.set_ylabel('Mean Completition Time')
            axs.set_xlabel(r'Utilization $(\frac{\lambda}{\mu})$')
            axs.set_ylim(0,5)

            # axs[ax].plot(xs, mcts, label=f'Size: {2**onChipDepth}')
            # axs[ax].set_title(r'Utilization: $\frac{\lambda}{\mu}$ = [0.1,.9], Mean Service Time $\frac{1}{\mu}$ = ' + str(1/mu))
            # axs[ax].legend(loc='upper left')
            # axs[ax].set_ylabel('Mean Completition Time')
            # axs[ax].set_xlabel(r'Utilization (\frac{\lambda}{\mu})$')

            # axs[ax].set_xlabel(r'$\rho = \frac{\lambda}{\mu}$')

        ax += 1

    plt.savefig(f'mm1_bisected.png')
    plt.plot(xs, mcts, label=f'Perfect')

'''

# Vary across 5 workloads

# Plot 1:
# - Perfect queue
# - average time between changes by position 
# - Number of slots moved per cycle on average?

# Plot 2:
# - Perfect queue
# - p50 length function of queue position

# Plot 3:
# - Perfect queue
# - p99 length function of queue position

