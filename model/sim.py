'''
Minimal PIFO vs Mutable vs Idealized SRPT Test Cases

PIFO: We consider a single PIFO queue. The queue is arbitarily large. Two operations are defined on the queue: enqueue, dequeue/modify head. An enqueue operation can insert a new entry to any slot within the PIFO. A dequeue/modify operation can only remove or modify the element at the head of the queue. We assume an ideal case that enqueue and dequeue/modify operations can both be performed in a single timestep, and that they can be performed simultaneously. 

Alongside the PIFO exists a space for entries that are inelegible for scheduling. We assume this is arbitrarily large, and a single entry can be read and written simultaneously in a single timestep. 

The PIFO observes the following enqueue/dequeue/modify rules:
1) If a new entry arrives (send message), and it is active (bytes are eligable to be sent), then it is enqueued to the PIFO in the slot corresponding to its computed SRPT priority.
2) If a new entry arrives (send message), and it is inactive, it is added to the off-queue space.
3) If an entry at the head of the PIFO is active, one packet is sent and the entry is updated to reflect that change. If the entry is empty as a result (no more packets to send), then it is dequeued from the PIFO.
4) If an entry at the head of the PIFO is inactive, the entry is dequeued and inserted into the off-queue space.


Mutable Queue: We consider a single mutable queue. The queue is arbitarily large. Two operations are defined on the queue: enqueue, dequeue/modify head. An enqueue operation can insert a new entry to the head of the queue. A dequeue/modify operation can only remove or modify the element at the head of the queue. We assume an ideal case that enqueue, dequeue/modify operations can be performed in a single timestep, and that they can be performed simultaneously.

At the conclusion of each timestep half of the entries in the queue will compare themselves to their immediate neighbor and decided whether they should swap positions based on their comparative priority. The half of entries performing this comparison will alternate between the even and odd entries each timestep. The entries will swap based on the following rules:
- If the neighbor further from the head in the queue has a better strict priority then swap positions.
- If the neighbor further from the head in the queue has equal strict priority and better SRPT priority, then swap positions.
- TODO mention modification -- this is a property of the queue

The mutable queue observes the following enqueue/dequeue/modify rules:
1) If a new entry arrives, and it is active, then it is enqueued to the mutable queue with an active priority.
2) If a new entry arrives, and it is inactive, then it is enqueued to the mutable queue with an inactive priority.
3) If an entry later becomes active, an update message is enqueued to percolate upwards and communicate the change.
3) If an entry later becomes inactive, an update message is enqueued to percolate upwards and communicate the change.
4) If the entry at the head of the mutable queue is active, one packet is sent and the entry is updated to relfect that change. If the entry is empty as a result, then it is dequeued from the mutable queue.

Idealized Queue: The idealized queue will implement SRPT among its active entries, be unbounded in size, and always reflect the currently activity of any given entry. 

Workloads:
1) Everything is active the entire time: A set of entries (send message requests) arrive to each of the queues in a single timestep. All of these entries are active and their state will remain unchanged.
2) Active during insert -> inactive after insert: A set of entries (send message requests) arrive to each of the queues in a single timestep. All of these entries are curently active. At some point during the lifetime of these entries, an entry that was once active now becomes inactive.
3) Inactive during insert -> active after insert: A set of entries (send message requests) arrive to each of the queues in a single timestep. Some of these entries are inactive. At some point these entries become active.

Distrbutions:
- Reduced: There are only a handful (5-10) initial entries. An update can change the state of 1-2 of these at once. In workload 1) all of these entries begin and remain active. In workload 2) all of these entries are active at insertion, and will be completly added to each queue. At this point the simulation begins and each timestep gives some propability of an entry becoming inactive. Once an entry becomes inacitve, it will NOT later become active and be reinserted. In workload 3) the entries will be added to the queue based on the predefined method for active/inactive entries. Each timestep will have a probability of activating one deactivated entry. 
- "Realistic": There are 50-100 initial entries. An activation can change the state of 5-10 of these at once. 
- Pathalogical: There are 500-1000 initial entries. An activation can change the state of 50-100 of these at once.


Evaluation:
Number of dry fires
Cycles spent reinserting
Total number of cycles
Inaccuracies?

# Simulation Parameters:
#   - PCIe throughput 
#   - Link throughput  
#   - PCIe latency
#   - Number of simultanous connections
#   - Cache size 
#   - Distribution of incoming data message lengths
#   - Number of simultanous users? Is the data interleaved or more like Homa?
#   - Distribution of outgoing data message lengths

# Simulation Evaluation:
#   - Time to completion
#   - P99 completion time
#   - Some measure of latency?
'''

import random
import math
import copy

# class Entry:
#     def __init__(self, id, payloads, grant, st=0, priority=0):
#         self.id = id
#         self.payloads = payloads
#         self.grant = grant
#         self.st = st
#         self.et = 0
#         self.ugrant = 0
#         self.origPayload = len(payloads)
#         self.priority = priority
# 
#     def exhausted(self):
#         return len(self.payloads) == 0
# 
#     def payloadSize(self):
#         return len(self.payloads)
# 
#     def __repr__(self):
#         return f'ID: {self.id}, Payload: {self.payloads}, Grant: {self.grant}, Priority: {self.priority}'

# class Queue:
#     def __init__(self, name):
#         # Entries that have been completed by the queue
#         self.complete = []
# 
#         # just use a list to store the elements and always search the elements
#         self.queue = []
# 
#         # Tried to send an ungranted packet
#         self.idleCycles = 0
# 
#         # Number of bytes sent
#         self.totalBytes = 0
# 
#         self.cmplTime = 0
# 
#         # The name of the queue for printing purposes
#         self.name = name
# 
#         # Payload data cache
#         self.cache = []
# 
#     def cmplBytes(self):
#         sum = 0
#         for entry in self.complete:
#             sum += entry.origPayload
#         return sum
# 
#     def msgThroughput(self):
#         return len(self.complete) / self.cmplTime
# 
#     def avgCmplTime(self):
#         sumDir = 0
#         for entry in self.complete:
#             sumDir += (entry.et - entry.st)
# 
#         return sumDir / len(self.complete)
# 
#     def dumpStats(self):
#         print(f'Queue: {self.name} Statistics:')
#         print(f'   - Total Cycles      : {self.cmplTime}')
#         print(f'   - Total Bytes       : {self.totalBytes}')
#         print(f'   - Idle Cycles       : {self.idleCycles}')
#         print(f'   - Completed Entries : {len(self.complete)}')
#         print(f'   - Cmpl Bytes Sent   : {self.cmplBytes()}')
#         print(f'   - Avg. Cmpl. Time   : {self.avgCmplTime()}')
#         print(f'   - Msg / Unit Time   : {self.msgThroughput()}')


class Message:
    def __init__(self, id, payload):
        self.id = id
        self.payload = payloads
        # self.grant = grant
        # self.st = st
        # self.et = 0
        # self.ugrant = 0
        # self.origPayload = len(payloads)
        # self.priority = priority

    # def exhausted(self):
    #     return len(self.payloads) == 0

    # def payloadSize(self):
    #     return len(self.payloads)

    # def __repr__(self):
    #    return f'ID: {self.id}, Payload: {self.payloads}, Grant: {self.grant}, Priority: {self.priority}'

class QueueEntry:
    def __init__(self, id, priority):
        self.id = id
        self.priority = priority 

class Sender():
    def __init__(self, network):
        self.network = network
        self.priortyQueue = PriorityQueue()
        self.generator = self.messagesGenerator()
        self.nextMessage = next(generator)
        self.activeMessages = []
        self.id = 0

    def cycle(self, time):
        # Do we need to generate a new message from the poisson process?
        if (self.nextMessage.st < time):
            self.nextMessage = next(generator)

        # If this cycle is when the new sendmsg request is ready
        if (self.nextMessage.st == time):
            # Initiate the sendmsg request
            self.sendmsg(self.nextMessage)

        # Take care of stuff to send out onto network
        self.egress()

        # Take care of stuff coming from network
        self.ingress()

    def sendmsg(self, message):
        self.activeMessages.append(message)
        queueEntry = QueueEntry(self.id)
        self.id += 1

        self.priorityQueue.enqueue(queueEntry)
    
    def messagesGenerator(self):
        currPayloadID = 0
        # Poisson arrival times + poisson packet sizes for outgoing messages
        t = 0
        while(1):
            t += random.expovariate(.1)
            # .1 = 1/desired mean
            # Compute the number of units of payload
            payload = math.floor(random.expovariate(.1))
            granted = math.floor(random.expovariate(.1))
            
            # Construct a new entry to pass to the queues
            message = Message(self.id, list(range(currPayloadID, currPayloadID + payload)))
            # entry = Entry(self.id, list(range(currPayloadID, currPayloadID + payload)), granted, math.floor(t))
            currPayloadID += payload
            
            # Move to the next message ID
            self.id += 1
            yield entry

    def egress(self, message):
        # TODO Read from head of priority queue
        # self.network.route(message)
        None
    
    def ingress(self, message):
        # TODO These are always grants
        None

class Receiver():
    def __init__(self, network):
        # Number of bytes outstaning
        self.outsanding = 0
        self.priorityQueue = PriorityQueue()

    def egress(self):
        # TODO grants going out. Increases outstanding bytes
        None

    def ingress(self):
        # TODO packet data arriving. Modifies outstanding bytes
        None

class Network():
    # TODO use this to add delay over the network
    def __init__(self, hosts):
        self.hosts = hosts
        self.mesh  = [[] for i in range(len(hosts)*len(hosts))]

    def route(self, message):
        mesh[message.sender + message.receiver * len(self.hosts)]
        This should be handled by the receiver function call
        # self.hosts[message.destination].ingress(message)

class BisectedPriorityQueue():
    def __init__(self):
        # TODO make these configurable

        # Fast on chip single-cycle priority queue
        self.onChip  = PriorityQueue(1024, 1)

        # Slow off chip multi-cycle priority queue
        self.offChip = PriorityQueue(math.inf, 10)

    def enqueue(self, entry):
        if (!self.onChip.full()):
            # If the onChip queue is not full insert into it
            self.onChip.enqueue(entry)
        elif (entry.priority < self.onChip.tail()):
            # If the new element is better than the worst onChip
            # Insert onChip and carry the tail
            self.offChip.enqueue(self.onChip.tail())
            self.onChip.enqueue(entry)
        else:
            # Otherwise place in offChip queue
            self.offChip.enqueue(entry)

    def dequeue(self):
        None

    def cycle(self):
        None
       
class PriorityQueue():
    def __init__(self, elements, delay):
        # Every delay number of cycles the best value will be updated
        
        self.queue = []
        self.delay = delay

    def enqueue(self, entry):
        # TODO dequeue the current best element
        self.queue.append(entry)
        
    def dequeue(self):
        # If we are storing no entries there is no packet to send
        if (len(self.queue) == 0): return None

        # Find the highest priority elements
        ideal = min(self.queue, key=lambda entry: entry.priority)

        # Reduce the priority as we send one packet
        ideal.priority -= 1

        if ideal.priority == -1:
            self.queue.remove(ideal)

        return ideal


# Current Goal: Create a sender object which instantiates a fast on chip queue with 1 cycle latency, and a slow queue of infinite size with X cycle latency. The sender can utilize some queueing dicipline. The sender gets sendmsg requests which causes insertion into the queue/s. The sendmsg request is destined for a particular receiver with some number of bytes.

# Create a network object which takes the output of a sender and gives it to respective array of receivers. Make number of receivers configurable. Receivers practice their own SRPT.
class Sim:
    def __init__(self):
        # Tested queuing strategies
        # self.senders = Sender()

        # Timestep counter
        self.time = 0

        # Initial ID to assign to new entries generated by the sendmsg function
        self.id = 0

        # self.grantGen = self.grantsGenerator()

        # self.nextMsg   = next(self.msgGen)
        # self.nextMsg   = next(self.msgGen)
        # self.nextGrant = next(self.grantGen)

        # Number of messages to complete
        self.maxCmplMsg = 10

    '''
    Iterate through simulation steps up to the max simulation time
    ''' 
    def simulate(self):
        while(not self.cmpl):
            self.step()
            self.time += 1

    '''
    One iteration of the system includes:
      1) 1 packet goes out
      2) The sendmsg function is invoked
      3) The grants function is invoked
    For each queue
    '''  
    def step(self):
                
        # Simulate all of the queues for this timestep
        for queue in self.queues:
            if (msgout != None):
                queue.totalBytes += 1

            if (msgout == None):
                queue.idleCycles += 1
            elif (msgout.exhausted()):
                msgout.et = self.time
                queue.complete.append(msgout)

            queue.tick()

    # If both the packet size and send time are poisson it models a M/M/1 queue?
    def dumpStats(self):
        for queue in self.queues:
            queue.dumpStats()

if __name__ == '__main__':
    sim = Sim() 
    sim.simulate()
    sim.dumpStats()


# TODO also need to measure latency

# TODO The probability of grants decreases as time goes on which is probably biased? Maybe should simulate the receiver as well performing their own grant scheduling?
# TODO Keith-The World is not Poisson
# TODO Can you get better average completion time by ignoring messages that are not completely granted? Maybe depends on the distribution of incoming grants? Maybe some other scheduling policy is desirable?
# TODO maybe also measure starvation?
# TODO flow control on individual substreams? Or on per message basis

# TODO need concrete bulk activation/deactivation examples
# The key question is can all scheduling decisions be made at time of enqueue
#   Processor sharing + SRPT? Seems like a lot of long activate deactive cycles. Maybe you want to weight SRPT somehow within the SRPT queue.
#   Multiple RPCs over a single flow. Maybe this is the norm for TCP based RPC systems? Would not want a seperate flow for each recevier. Would not want to startup a TCP connection for a new RPC
#   Simplicity of design?
#   Granting system?
#   Updates to individual flows? Starvation prevention?
#   No-reneging of windows?
#   Active queue management?


# Look into least slack time first
# Curious about market-based congestion control processes
# Outgoing messages are sorted in a queue by their "bid". Each message individually describes how to compute their bid/what they are willing to pay? Sending a packet consumes "bids" worth of tokens from that message.
# Maybe the operator defines a system wide marginal utility for sending a messages next packet. Messages define a marginal utility for their packet being sent next. And compare against the two? SRPT can be expressed within marginal utilities. The message with the least remaining bytes has the highest marginal utility for the next packet, and so forth. Maybe the scheduling policy is marginal utility (partial derivative) which is simpler way to express scheduling?

# The scheduler is responsible for computing the marginal utility and inserting them into a PIFO like queue. In the case of SRPT at least, marginal utility is a function dependent on the number of bytes remaining. This does not really do much for you I think.

# Take the cost function, compute the marginal utility for each "good", time, bytes sent, etc. Could have one queue for each good. When a new message wants to be sent, compute the marginal utilities for each good, and insert them into their respective queues. Why can't PIFO just have a seperate FIFO queue that it occasionally pulls from? Push updates to other queues?

# PIFO assumes that packet ranks increase monotonically within each flow

# PIFO is O(n) comparisons?




# !!!!! TODO  should reoder every cycle regardless of ops
class Mutable(Queue):
    def __init__(self):
        super().__init__("Mutable")

    def enqueue(self, entry):
        if (entry.grant == 0):
            entry.priority = 1
            self.queue.insert(0, entry)
        else:
            entry.priority = 0
            self.queue.insert(0, entry)

        # print(f'Enqueue: {entry}')
        self.order()

    def dequeue(self):
        # Nothing to send
        if (len(self.queue) == 0): return None

        # Will only pop from head of queue
        head = self.queue[0]

        '''
        If the entry at the head is granted, send a packet. If sending
        that packet causes the entry to be completely sent, then we
        are done and destroy it.
        '''
        if (head.priority == 0):
            # Send one packet
            # head.payload -= 1
            head.payload.pop(-1)
            head.grant   -= 1

            # Did we send the whole message?
            if (head.exhausted()):
                self.queue.pop(0)
            elif (head.grant == 0):
                head.priority = 1

            self.order()
            return head 
        else:
            self.order()
            # There can be no possible active entries in the queue
            return None

    def grant(self, grant):
        grant.priority = 2
        self.queue.insert(0, grant)
        self.order()

    def order(self):
        if (len(self.queue) != 0 and self.queue[-1].priority == 2):
            self.queue.pop(-1)

        # swap (0,1) (2,3) (4,5) (6,7) 
        for even in range(int(len(self.queue)/2)):
            low  = self.queue[even]
            high = self.queue[even+1]

            if (low.priority > high.priority):
                # Is this a grant
                if (low.priority == 2 and low.id == high.id):
                    # print("REACTIVE")
                    high.grant += low.grant
                    high.priority = 0
                self.queue[even+1] = low
                self.queue[even]   = high
            elif (low.priority == high.priority and low.payloadSize() > high.payloadSize()):
                self.queue[even+1] = low
                self.queue[even]   = high

        # swap (1,2) (3,4) (5,6) (7,8)
        for odd in range(int((len(self.queue)-1)/2)):
            low  = self.queue[odd+1]
            high = self.queue[odd+2]

            if (low.priority > high.priority):
                # Is this a grant
                if (low.priority == 2 and low.id == high.id):
                    # print("REACTIVE")
                    high.grant += low.grant
                    high.priority = 0
                self.queue[odd+2] = low
                self.queue[odd+1] = high
            elif (low.priority == high.priority and low.payloadSize() > high.payloadSize()):
                self.queue[odd+2] = low
                self.queue[odd+1] = high

    def tick(self):
        self.order()
 

class PIFO(Queue):
    def __init__(self):
        super().__init__("PIFO")

        # entries which cannot be inserted into the queue due fc state
        self.blocked = []

    def enqueue(self, entry):
        # If this entry has granted bytes, find its place in the queue, otherwise buffer
        if (entry.grant != 0):
            # Find the first entry in the queue that has a byte count greater then our new entry's byte count
            insert = next((e[0] for e in enumerate(self.queue) if e[1].payload > entry.payload), len(self.queue))
            self.queue.insert(insert, entry)
        else:
            self.blocked.append(entry)

    def dequeue(self):
        # Nothing to send
        if (len(self.queue) == 0): return None

        # PIFO will only pop from head of queue
        head = self.queue[0]

        '''
        If the entry at the head is granted, send a packet. If sending
        that packet causes the entry to be completely sent, then we
        are done and destroy it.
        '''
        if (head.grant != 0):
            # Send one packet
            # head.payload -= 1
            head.payload.pop(-1)
            head.grant   -= 1

            # Did we send the whole message?
            if (head.exhausted()):
                self.queue.pop(0)
            elif (head.grant == 0):
                self.queue.pop(0)
                self.blocked.append(head)

            return head 
        else:
            # The entry is not granted and we will block it
            self.queue.pop(0)
            self.blocked.append(head)

            return None

    def grant(self, grant):
        # this is fine because when the entry gets to the head of the queue it can access this data immediately
        loc = next((e[1] for e in enumerate(self.queue) if e[1].id == grant.id), None)

        if loc != None:
            loc.grant += grant.grant

        loc = next((e[1] for e in enumerate(self.blocked) if e[1].id == grant.id), None)

        if loc != None:
            loc.grant += grant.grant
            self.enqueue(loc)
            self.blocked.remove(loc)

    def tick(self):
        None
 
