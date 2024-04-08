import random
import math
import copy

from Queue import BisectedPriorityQueue
from Queue import QueueEntry
from Message import Message
from Message import DataPacket
from Message import GrantPacket 

class Host():
    def __init__(self, id, network, config, simulator):
        self.simulator = simulator

        # How many units of RTT delay in network
        self.unscheduled = config['Host']['unscheduled']

        # Mesh network for sending messages
        self.network = network

        # Bisected priority queue structure
        self.dataPriorityQueue = BisectedPriorityQueue(config['Host']['Queue'])

        # Grant priority queue
        self.grantPriorityQueue = BisectedPriorityQueue(config['Host']['Queue'])

        # Number of receivers available in the network
        self.numHosts = self.network.channels

        # ID for this sender
        self.id  = id

        # Generator for payloads and sizes
        self.generator = [self.msgGen(r['rate'], r['size']) for r in config['Host']['Rates']]
        self.events = [next(gen) for gen in self.generator]

        # Number of units of grants outstaning
        self.outstanding = 0

        self.config = config

        self.time = 0

        self.nextpacket = self.nextPacket()

    def tick(self):
        # Do we need to generate a new message from the poisson process?
        #if (self.nextMessage == None):
        #    self.nextMessage = next(self.generator)
        self.events = [next(fut) if curr[0] < self.time else curr for fut, curr in zip(self.generator, self.events)]

        # If this cycle is when the new sendmsg request is ready 
        for event in self.events:
            if event[0] == self.time:  
                # Construct a new entry to pass to the queues
                message = Message(src=self.id, dest=event[1], length=event[2], unscheduled=self.unscheduled, startTime=event[0])

                # Initiate the sendmsg request
                self.sendmsg(message)
                self.nextMessage = None
                continue

        self.dataPriorityQueue.cycle()
        self.grantPriorityQueue.cycle()

        # Take care of stuff to send out onto network
        self.egress()

        # Take care of stuff coming from network
        self.ingress()

        self.time += 1

    def sendmsg(self, message):
        # Add to global message store
        self.simulator.messages.append(message)

        # Construct a queue entry for this message id
        entry = QueueEntry(message=message, priority=message.length, cycles=self.unscheduled, incr=self.config['Host']['packetsize'])
        message.dataEntry = entry
        self.dataPriorityQueue.enqueue(entry)

    def nextPacket(self):
        while(True):
            if (self.outstanding < self.config['Host']['overcommitment']):
                # Select a packet type to send
                entry = self.grantPriorityQueue.dequeue()
                if (entry != None):
                    packet = GrantPacket(entry.message)
                    # We sent one packet for this message
                    entry.message.receiverGranted += self.config['Host']['packetsize']
                    self.outstanding += self.config['Host']['packetsize']
                    # print(self.outstanding)
                    yield packet
                    continue

            # Get the queue object output from the priority queue
            entry = self.dataPriorityQueue.dequeue()
            if (entry != None):
                packet = DataPacket(entry.message)

                remaining = entry.message.length - entry.message.senderSent
                # Send one packet size, or the remaining number of payloads 
                for chunk in range(min(self.config['Host']['packetsize'], remaining)):
                    entry.message.senderSent += 1
                    yield packet
                    continue

            yield None

    def egress(self):
        if self.network.peek(self.id):
            packet = next(self.nextpacket)
            if packet != None:
                send = self.network.write(packet.message.dest, packet)
        
    def ingress(self):
        # Read the next packet from the network
        packet = self.network.read(self.id)

        if (packet == None):
            return

        match packet:
            case DataPacket():
                # TODO check this logic
                if packet.message.length > self.config['Host']['unscheduled']:
                    if packet.message.receiverReceived > self.config['Host']['unscheduled']:
                        self.outstanding -= 1

                    # Create a grant entry only if this is the first data and the message is not already fully granted
                    if packet.message.receiverReceived == 0:
                        ungranted = packet.message.length - packet.message.receiverGranted - 1
                        # Construct a queue entry for this message id
                        entry = QueueEntry(message = packet.message, priority=ungranted, cycles=ungranted, incr=self.config['Host']['packetsize'])
                        self.grantPriorityQueue.enqueue(entry)

                packet.message.receiverReceived += 1

            case GrantPacket():
                packet.message.senderGranted += self.config['Host']['packetsize']

                reinsert = packet.message.dataEntry.cycles == 0

                packet.message.dataEntry.cycles   += self.config['Host']['packetsize']
                packet.message.dataEntry.priority = max(0, packet.message.dataEntry.priority - self.config['Host']['packetsize'])

                # Readd the entry to the queue if it was removed
                if reinsert:
                    self.dataPriorityQueue.enqueue(packet.message.dataEntry)
            case _:
                print("Unrecognized packet type")

    # TODO could we get an update and remove from the queue in the same cycle
                
    # TODO why even pass this by arg?    
    def msgGen(self, rate, size):
        # Poisson arrival times + poisson packet sizes for outgoing messages
        t = 0
        while(1):
            t += math.ceil(random.expovariate(1/rate))
            # Compute the number of units of payload
            payload = math.floor(random.expovariate(1/size)) + 1
            dest = random.randint(0, self.numHosts-1)

            yield (math.floor(t), dest, payload) 

