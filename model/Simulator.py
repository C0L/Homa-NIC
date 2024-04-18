import random
import math
import copy
import yaml
import argparse

class Message:
    def __init__(self, length):
        # For SRPT the priority is the length
        self.priority = length 
        self.length   = length

        self.startTime = startTime
        self.endTime   = 0

# TODO just simulatig the data movement delay, need cycle delay of priority queue
class BisectedPriorityQueue():
    def __init__(self, config):
        # Fast on chip single-cycle priority queue
        self.onChip  = PriorityQueue(config['onChipDepth'], "On Chip")

        # Slow off chip multi-cycle priority queue
        self.offChip = PriorityQueue(config['offChipDepth'], "Off Chip")

        self.commDelay  = config['commDelay']
        self.queueDelay = config['queueDelay']

        self.promoting = []
        self.demoting  = []

        self.time = 0

    def enqueue(self, entry):
        self.onChip.enqueue(entry)

    def dequeue(self):
        return self.onChip.dequeue()

    def tick(self):
        demote = self.onChip.demote()

        if demote != None:
            self.demoting.append((self.time, demote))

        if self.time % self.queueDelay == 0:
            promote = self.offChip.promote()

            if promote != None:
                self.promoting.append((self.time, promote))

        # TODO this should be made more realistic

        # Impose communication dmoting on-chip to off-chip
        if len(self.demoting) != 0:
            head = self.demoting[0]
            if self.time - head[0] > self.commDelay:
                self.offChip.enqueue(head[1])
                self.demoting.remove(head)

        # Impose communication promoting off-chip to on-chip
        if len(self.promoting) != 0:
            head = self.promoting[0]
            if self.time - head[0] > self.commDelay:
                self.promoting.remove(head)
                self.onChip.enqueue(head[1])
                demote = self.onChip.demote()
                if demote != None:
                    self.demoting.append((self.time, demote))

        self.onChip.tick()
        self.offChip.tick()

        self.time += 1

    def stats(self):
        self.onChip.stats()
        self.offChip.stats()

class PriorityQueue():
    def __init__(self, size, token):
        self.size  = size  
        self.token = token
        self.queue = []
        self.record = []
        self.time = 0

    def enqueue(self, entry):
        self.queue.append(entry)
        assert len(self.queue) <= self.size, \
            "Queue Overflow" 
        
    def dequeue(self):
        # If we are storing no entries there is no work to do 
        if (len(self.queue) == 0): return None

        # Find the highest priority elements
        ideal = min(self.queue, key=lambda entry: entry.priority)

        # Reduce the priority as we send one packet
        ideal.priority = max(0, ideal.priority - ideal.increment)
        ideal.cycles   = max(0, ideal.cycles - ideal.increment)
        
        # If the entry is done remove it
        if (ideal.priority == 0 or ideal.cycles == 0):
            self.queue.remove(ideal)

        return ideal

    def promote(self):
        if len(self.queue) == 0:
            return None

        # Find the highest priority element
        best = min(self.queue, key=lambda entry: entry.priority)

        self.queue.remove(best)

        return best 

    def demote(self):
        if len(self.queue) == self.size:
            worst = max(self.queue, key=lambda entry: entry.priority)
            self.queue.remove(worst)
            return worst 

        return None

    def tick(self):
        self.record.append(len(self.queue))
        self.time += 1

    def stats(self):
        print(f'PriorityQueue: {self.token} (Size: {self.size}):')
        print(f'   - Average Occupancy: {sum(self.record)/len(self.record)}')
        print(f'   - Median Occupancy:  {statistics.median(self.record)}')
        print(f'   - Max Occupancy:     {max(self.record)}')

class Host():
    def __init__(self):
        # Generator for payloads and sizes
        self.generator = [self.msgGen(r['rate'], r['size'], r['token']) for r in config['Host']['Rates']]
        self.events    = [next(gen) for gen in self.generator]

    def tick(self):
        # Do we need to generate a new message from the poisson process?
        self.events = [next(fut) if curr.startTime < self.time else curr for fut, curr in zip(self.generator, self.events)]

        # If this cycle is when the new sendmsg request is ready 
        for event in self.events:
            if event.startTime <= self.time and event.length != 0:  
                # Initiate the sendmsg request
                self.queue.enqueue(event)

        self.queue.tick()

    def sendmsg(self, message):
        # Construct a queue entry for this message id
        self.queue.enqueue(message)

    '''
    Called from simulator
    '''
    def egress(self):
        entry = self.queue.dequeue()
        if (entry != None):
            return entry

        return None

    def mm1(self, rate, size, token):
        # Poisson arrival times + poisson packet sizes for outgoing messages
        t = 0
        while(1):
            t += random.expovariate(1/rate)
            # Compute the number of units of payload
            length = round(random.expovariate(1/size))

            message = Message(id=self.id, length=length, startTime=round(t), token=token)

            yield message

class Sim:
    def __init__(self, config):
        # Timestep counter
        global time
        time = 0

        # Initial ID to assign to new entries generated by the sendmsg function
        self.id = 0

        # Number of time steps to simulate for
        self.timeSteps = 100000

        self.host = Host()

    '''
    Iterate through simulation steps up to the max simulation time
    ''' 
    def simulate(self):
        for t in range(self.timeSteps):
            self.host.tick()
            global time 
            time += 1

    # def avgCmplTime(self, host, token):
    #     complete = self.completeMessages(host, token)
    #     sumDir = 0
    #     for entry in complete:
    #         sumDir += (entry.endTime - entry.startTime)
    #      
    #         return sumDir / len(complete)

    # def stats(self):
    #     for host in self.hosts:
    #         for rate in self.config['Host']['Rates']:
    #             print(f'Host: {host.id}, Rate: {rate["token"]}:')
    #             print(f'   - Total Complete Messages    : {len(self.completeMessages(host, rate["token"]))}')
    #             print(f'   - Avg. Cmpl. Time            : {self.avgCmplTime(host, rate["token"])}')
    #             # print(f'   - Msg / Unit Time   : {self.msgThroughput(time)}')
    #         host.stats()

def main(args):
    with open(args.config) as file:
        config = yaml.safe_load(file)

    sim = Sim(config) 
    sim.simulate()
    sim.stats()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', type=str, help="yaml config file")

    args = parser.parse_args()

    main(args)

# class Sim:
#     def __init__(self, config):
#         self.network = Network(config['Simulation']['hosts']['total'], config['Network'])
# 
#         self.hosts = [Host(i, self.network, config, self) for i in range(config['Simulation']['hosts']['total'])]
#         self.entities = self.hosts + [self.network]
#         self.messages = []
# 
#         # Timestep counter
#         global time
#         time = 0
# 
#         # Initial ID to assign to new entries generated by the sendmsg function
#         self.id = 0
# 
#         # Number of time steps to simulate for
#         self.timeSteps = config['Simulation']['cycles']
# 
#         self.config = config
# 
#     '''
#     Iterate through simulation steps up to the max simulation time
#     ''' 
#     def simulate(self):
#         for t in range(self.timeSteps):
#             self.step()
#             global time 
#             time += 1
# 
#     def step(self):
#         # Simulate all of the agents for this timestep
#         random.shuffle(self.entities)
#         for entity in self.entities:
#             entity.tick()
# 
#         self.sweep()
# 
#     def sweep(self):
#         for message in self.messages:
#             if message.receiverReceived == message.length and message.endTime == 0:
#                 message.endTime = time
# 
#     def completeMessages(self, host, token):
#         return [m for m in self.messages if m.src == host.id and m.endTime != 0 and m.token == token]
# 
#     def avgCmplTime(self, host, token):
#         complete = self.completeMessages(host, token)
#         sumDir = 0
#         for entry in complete:
#             sumDir += (entry.endTime - entry.startTime)
#          
#             return sumDir / len(complete)
# 
#     def stats(self):
#         for host in self.hosts:
#             for rate in self.config['Host']['Rates']:
#                 print(f'Host: {host.id}, Rate: {rate["token"]}:')
#                 print(f'   - Total Complete Messages    : {len(self.completeMessages(host, rate["token"]))}')
#                 print(f'   - Avg. Cmpl. Time            : {self.avgCmplTime(host, rate["token"])}')
#                 # print(f'   - Msg / Unit Time   : {self.msgThroughput(time)}')
#             host.stats()
# 
# def checkConfig(config):
#     totalBandwidth = 0.0
#     for r in config['Host']['Rates']:
#         # Data bandwidth
#         totalBandwidth += r['size'] / r['rate']
# 
#         grantable = r['size'] - config['Host']['unscheduled']
#         # Grants bandwidth
#         if grantable > 0:
#             totalBandwith += grantable / r['rate']
# 
#     print(f'Aggregate Network Demand: {totalBandwidth} per unit time')
#                 
# def main(args):
#     with open(args.config) as file:
#         config = yaml.safe_load(file)
# 
#     checkConfig(config)
#         
#     sim = Sim(config) 
#     sim.simulate()
#     sim.stats()
# 
# if __name__ == '__main__':
#     parser = argparse.ArgumentParser()
#     parser.add_argument('-c', '--config', type=str, help="yaml config file")
# 
#     args = parser.parse_args()
# 
#     main(args)
