import random
import math
import copy
import yaml
import argparse

from Queue import BisectedPriorityQueue
from Host import Host
from Network import Network

class Sim:
    def __init__(self, config):
        self.network = Network(config['Simulation']['hosts']['total'], config['Network'])

        self.hosts = [Host(i, self.network, config, self) for i in range(config['Simulation']['hosts']['total'])]
        self.entities = self.hosts + [self.network]
        self.messages = []

        # Timestep counter
        global time
        time = 0

        # Initial ID to assign to new entries generated by the sendmsg function
        self.id = 0

        # Number of time steps to simulate for
        self.timeSteps = config['Simulation']['cycles']

        self.config = config

    '''
    Iterate through simulation steps up to the max simulation time
    ''' 
    def simulate(self):
        for t in range(self.timeSteps):
            self.step()
            global time 
            time += 1

    def step(self):
        # Simulate all of the agents for this timestep
        random.shuffle(self.entities)
        for entity in self.entities:
            entity.tick()

        self.sweep()

    def sweep(self):
        for message in self.messages:
            if message.receiverReceived == message.length and message.endTime == 0:
                message.endTime = time

    def completeMessages(self, host, token):
        return [m for m in self.messages if m.src == host.id and m.endTime != 0 and m.token == token]

    def avgCmplTime(self, host, token):
        complete = self.completeMessages(host, token)
        sumDir = 0
        for entry in complete:
            sumDir += (entry.endTime - entry.startTime)
         
            return sumDir / len(complete)

    def dumpStats(self):
        for host in self.hosts:
            for rate in self.config['Host']['Rates']:
                print(f'Host: {host.id}, Rate: {rate['token']}:')
                print(f'   - Total Complete Messages    : {len(self.completeMessages(host, rate['token']))}')
                print(f'   - Avg. Cmpl. Time            : {self.avgCmplTime(host, rate['token'])}')
                # print(f'   - Msg / Unit Time   : {self.msgThroughput(time)}')

def main(args):
    with open(args.config) as file:
        config = yaml.safe_load(file)

    sim = Sim(config) 
    sim.simulate()
    sim.dumpStats()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', type=str, help="yaml config file")

    args = parser.parse_args()

    main(args)


    
