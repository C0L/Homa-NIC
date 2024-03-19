import random
import math
import copy

# TODO maybe these things can be collapsed into one?
class Message:
    '''
    Represents an outgoing message to be transmited or a message being received

    ...

    Attributes
    ----------
    source    : int 
        ID of the sender 
    dest      : int 
        ID of the receiver
    id        : int 
        ID of this message
    length    : int
        Total length of this message
    remaining : int
        Remaining data units to send
    ungranted: int
        Remaining data to grant
    startTime : int
        Time step of this messages creation
    
    Methods
    -------
    __repr__():
        Print out all of the attributes
    '''
    def __init__(self, source = 0, dest = 0, id = 0, length = 0, remaining = 0, ungranted = 0, startTime = 0):
        """
        Fills atrributes for message insantiation

        Parameters
        ----------
        source    : int 
            ID of the sender 
        dest      : int 
            ID of the receiver
        id        : int 
            ID of this message
        length    : int
            Total length of this message
        remaining : int
            Remaining data units to send
        ungranted: int
            Remaining data to grant
        startTime : int
            Time step of this messages creation
        """
        self.source    = source
        self.dest      = dest
        self.id        = id 
        self.length    = length
        self.remaining = remaining
        self.ungranted = ungranted 
        self.startTime = startTime
        self.endTime   = 0

    def __repr__(self):
        return f'''
        Source: {self.source}
        Dest: {self.dest}
        ID: {self.id}
        Length: {self.length}
        Remaining: {self.remaining}
        Ungranted: {self.ungranted}
        Start: {self.startTime}
        End: {self.endTime}
        '''

class QueueEntry:
    def __init__(self, mid, priority, cycles):
        self.mid = mid
        self.priority = priority
        self.validCycles = cycles 

    # Called when the entry is at the head of the queue
    # Returns true or false whether the entry is empty
    def update(self):
        self.priority -= 1
        self.validCycles -= 1
        if (self.validCycles == 0 or self.priority == 0):
            return False
        else:
            return True
    
    def __repr__(self):
        return f'''QueueEntry:
        - ID          : {self.mid}
        - Priority    : {self.priority}
        - ValidCycles : {self.validCycles}
        '''

class Sender():
    def __init__(self, id, network, numSenders, numReceivers, rttData):
        # How many units of RTT delay in network
        self.rttData = rttData

        # Mesh network for sending messages
        self.network = network

        # Bisected priority queue structure
        self.priorityQueue = BisectedPriorityQueue()

        # Number of receivers availible in the network
        self.numReceivers = numReceivers

        # Number of senders in the network
        self.numSenders = numSenders

        # ID for this sender
        self.id  = id

        # Unique ID for messages
        self.mid = 0

        # Generator for payloads and sizes
        self.generator = self.messageGenerator()
        self.nextMessage = next(self.generator)

        # Messages which need to be sent
        self.activeMessages = []

        # Messages fully transmitted
        self.completeMessages = []

        # Messages waiting for grant
        self.pending = []

    def tick(self, time):
        # Do we need to generate a new message from the poisson process?
        if (self.nextMessage.startTime < time):
            self.nextMessage = next(self.generator)

        # If this cycle is when the new sendmsg request is ready
        if (self.nextMessage.startTime == time):
            # Initiate the sendmsg request
            self.sendmsg(copy.deepcopy(self.nextMessage))

        # TODO adding duplicates here
        for message in self.pending:
            if (message.ungranted < message.remaining):
                # Construct a minimal queue entry for this message id. The payload defines the priority but can only send RTTbytes of payload initially.
                print("READD")
                print(message)
                queueEntry = QueueEntry(mid = message.id, priority = message.remaining, cycles = message.remaining - message.ungranted)
                self.priorityQueue.enqueue(queueEntry)
                self.pending.remove(message)

        self.priorityQueue.cycle()

        # Take care of stuff to send out onto network
        self.egress(time)

        # Take care of stuff coming from network
        self.ingress()



    def sendmsg(self, message):
        # All messages initiall begin as granted (for one RTT)
        self.activeMessages.append(message)

        # Construct a minimal queue entry for this message id. The payload defines the priority but can only send RTTbytes of payload initially.
        queueEntry = QueueEntry(mid = message.id, priority = message.remaining, cycles = message.length - message.ungranted)

        self.priorityQueue.enqueue(queueEntry)

    def egress(self, time):
        # Get the queue object output from the priority queue
        queue = self.priorityQueue.dequeue()

        # If there is no queue object then there are no packets to send
        if queue == None:
            return

        # Get the message associated with this queue entry
        message = next((m for m in self.activeMessages if queue.mid == m.id), None)

        # Only send a packet if a message is found 
        if message != None:
            # Construct a packet to send over the network
            self.network.write(message.dest, copy.deepcopy(message))

            # We sent one packet for this message
            message.remaining -= 1

            # If this message has been completely sent, set end time,
            # remove from active messages, add to complete messasges
            # list
            if (message.remaining == 0):
                message.endTime = time
                self.completeMessages.append(message)
                self.activeMessages.remove(message)
                return

            if (queue.validCycles == 0):
                self.pending.append(message)
                return
    
    def ingress(self):
        # Read the next packet from the network
        message = self.network.read(self.id)
        if (message != None):
            search = next((m for m in self.activeMessages if message.id == m.id), None)

            if (search == None):
                # print(message)
                print("BAD ROUTING")

            #print("UPDATE")
            #print(search)
            #print(message)
            search.ungranted = message.ungranted
    
    def messageGenerator(self):
        # Poisson arrival times + poisson packet sizes for outgoing messages
        t = 0
        while(1):
            t += random.expovariate(.1)
            # .1 = 1/desired mean
            # Compute the number of units of payload
            payload = math.floor(random.expovariate(.1)) + 1 + 2 
            # granted = math.floor(random.expovariate(.1))

            dest = random.randint(self.numSenders, self.numSenders + self.numReceivers - 1)

            # Construct a new entry to pass to the queues
            # TODO increment payload by 1?
            # TODO tame this line
            # TODO create new message constructor
            message = Message(source = self.id, dest = dest, id = self.mid, length = payload, remaining = payload, ungranted = payload - self.rttData, startTime = math.floor(t))

            self.mid += 1
            
            yield message 

    # The message throughput at the current time "time"
    def msgThroughput(self, time):
        return len(self.completeMessages) / time

    # Average message completion time at the current time
    def avgCmplTime(self):
        sumDir = 0
        for entry in self.completeMessages:
            sumDir += (entry.endTime - entry.startTime)
            
        return sumDir / len(self.completeMessages)

    def dumpStats(self, time):
        print(f'Sender: {self.id} Statistics:')
        # print(f'   - Total Cycles      : {self.cmplTime}')
        # print(f'   - Total Bytes       : {self.totalBytes}')
        # print(f'   - Idle Cycles       : {self.idleCycles}')
        # print(f'   - Completed Entries : {len(self.complete)}')
        # print(f'   - Cmpl Bytes Sent   : {self.cmplBytes()}')

        print(f'   - Total Messages    : {len(self.completeMessages)}')
        print(f'   - Avg. Cmpl. Time   : {self.avgCmplTime()}')
        print(f'   - Msg / Unit Time   : {self.msgThroughput(time)}')

class Receiver():
    def __init__(self, id, network, rttData):
        # Mesh network for routing
        self.network = network

        # ID for this receiver 
        self.id = id

        # Number of bytes outstaning
        self.outsanding = 0

        # Grant priority queue
        self.priorityQueue = BisectedPriorityQueue()

        # How many units of RTT delay in network
        self.rttData = rttData

        self.grantableMessages = []

    def egress(self):
        # Get the queue object output from the priority queue
        queue = self.priorityQueue.dequeue()

        # If there is no queue object then there are no packets to send
        if queue == None:
            return

        # Get the message associated with this queue entry
        message = next((m for m in self.grantableMessages if queue.mid == m.id), None)

        # Only send a packet if there are messages to grant to
        if message != None:
            # We sent one packet for this message
            message.ungranted -= 1

            # Construct a grant packet to send over the network.
            # Switch the order of source and destination for packet
            # transmit
            self.network.write(message.dest, copy.deepcopy(message))

            # print("GRANT")
            # print(message)

            # Message is fully granted
            if (message.ungranted == 0):
                self.grantableMessages.remove(message)

    def ingress(self):
        # Read the next packet from the network
        message = self.network.read(self.id)
        if (message != None):
            # Add an entry for granting just for the first payload received
            if (message.length == message.remaining):
                # If the message is already fully granted we are done
                if (message.ungranted <= 0):
                    return

                source = message.source
                message.source = message.dest
                message.dest = source

                self.grantableMessages.append(message)

                queueEntry = QueueEntry(mid = message.id, priority = message.ungranted, cycles = message.ungranted)

                self.priorityQueue.enqueue(queueEntry)

    def tick(self, time):
        self.egress()
        self.ingress()
        # TODO reneable
        # self.priorityQueue.cycle()

# Add RTT of delay in here, which will dicate the receiver grant size
# This is just a mesh of lists for each receiver
class Network():
    # TODO use this to add delay over the network
    def __init__(self, hosts, rttData):
        self.links = [[] for i in range(hosts)]

        # How many units of RTT delay in network
        self.rttData = rttData

        self.time = 0

    # Outgoing packets towards the network
    def write(self, dest, message):
        # Add to the respective buffer the time of insertion and the message
        self.links[dest].append((self.time, message))

    # Incoming packets coming from the network
    def read(self, dest):
        # print(f'READ {dest}')
        # print(self.links)
        # Are there any pending elements on the network
        if (self.links[dest]):
            if (self.links[dest][0][0] + int(self.rttData/2) < self.time):
                return self.links[dest].pop(0)[1]
        return None
        # return self.links[dest].pop() if self.links[dest] else None

    def tick(self, time):
        self.time += 1

class BisectedPriorityQueue():
    def __init__(self):
        # TODO make these configurable

        # Fast on chip single-cycle priority queue
        # self.onChip  = PriorityQueue(1024, 1)

        self.onChip  = PriorityQueue(10, 1)

        # Slow off chip multi-cycle priority queue
        self.offChip = PriorityQueue(math.inf, 10)

    def enqueue(self, entry):
        print("ENQUEUE")
        if (not self.onChip.full() or entry.priority < self.onChip.tail()):
            print("ADD ON CHIP")
            # If the onChip queue is not full insert into it
            self.onChip.enqueue(entry)
        else:
            # Otherwise place in offChip queue
            self.offChip.enqueue(entry)

    def dequeue(self):
        if (not self.onChip.empty()):
            # print("POP FROM ON CHIP")
            return self.onChip.dequeue()
        return None

    def cycle(self):
        print("cycle")
        print(self.onChip.queue)
        print(self.offChip.queue)
        carryUp = self.onChip.carryUp()
        if (carryUp != None):
            # print("carry up")
            self.offChip.enqueue(carryUp)

        if (not self.onChip.full() and not self.offChip.empty()):
            # print("carry down")
            carry = self.offChip.carryDown()
            # print(carry)
            
        # TODO carry elements from onChip to offChip and vice versa

# TODO need to impose delay
class PriorityQueue():
    def __init__(self, maxElements, delay):
        # Every delay number of cycles the best value will be updated
        
        self.queue = []
        self.delay = delay
        self.maxElements = maxElements

    def enqueue(self, entry):
        # TODO dequeue the current best element
        self.queue.append(entry)
        
    def dequeue(self):
        # If we are storing no entries there is no packet to send
        if (len(self.queue) == 0): return None

        # Find the highest priority elements
        ideal = min(self.queue, key=lambda entry: entry.priority)

        # Reduce the priority as we send one packet
        status = ideal.update()

        if (status == False):
            self.queue.remove(ideal)

        return ideal

    def carryDown(self):
        # Find the highest priority elements
        ideal = min(self.queue, key=lambda entry: entry.priority)
        self.queue.remove(ideal)
        return ideal

    def carryUp(self):
        if (len(self.queue) > self.maxElements):
            worst = max(self.queue, key=lambda entry: entry.priority)
            self.queue.remove(worst)
            return worst 
        else:
            return None
            
    def tail(self):
        worst = max(self.queue, key=lambda entry: entry.priority)
        return worst.priority

    def empty(self):
        return len(self.queue) == 0

    def full(self):
        return len(self.queue) == self.maxElements

class Sim:
    def __init__(self):
        self.numSenders   = 1
        self.numReceivers = 1

        # How many units of payload for a network round trip (defines network latency)
        self.rttData = 2
        
        self.network = Network(self.numSenders + self.numReceivers, self.rttData)

        # Tested queuing strategies
        self.senders   = [Sender(i, self.network, self.numSenders, self.numReceivers, self.rttData) for i in range(self.numSenders)]
        self.receivers = [Receiver(self.numSenders + i, self.network, self.rttData) for i in range(self.numReceivers)]

        self.agents = self.senders + self.receivers + [self.network]

        # Timestep counter
        self.time = 0

        # Initial ID to assign to new entries generated by the sendmsg function
        self.id = 0

        # Number of time steps to simulate for
        self.timeSteps = 1000

    '''
    Iterate through simulation steps up to the max simulation time
    ''' 
    def simulate(self):
        # while(not self.cmpl):
        for t in range(self.timeSteps):
            self.step()
            self.time += 1

    def step(self):
        # Simulate all of the agents for this timestep
        for agent in self.agents:
            agent.tick(self.time)

    # If both the packet size and send time are poisson it models a M/M/1 queue?
    def dumpStats(self):
        for sender in self.senders:
            sender.dumpStats(self.timeSteps)

if __name__ == '__main__':
    sim = Sim() 
    sim.simulate()
    sim.dumpStats()



# Compare infinitely large on chip queue to other constructions
# TODO unify sender and receiver. Just have a single ID for each agent which has both sender and receiver.
# Share bandwidth between them somehow? Grant whenever need to (which should be much less bandwidth)
# TODO restrict initial payload size to RTT bytes (store in message)....
# TODO compute the "movement" of elements based on depth in queue
# TODO also need to measure latency
# TODO completion time should be measured by receivers



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
# class Mutable(Queue):
#     def __init__(self):
#         super().__init__("Mutable")
# 
#     def enqueue(self, entry):
#         if (entry.grant == 0):
#             entry.priority = 1
#             self.queue.insert(0, entry)
#         else:
#             entry.priority = 0
#             self.queue.insert(0, entry)
# 
#         # print(f'Enqueue: {entry}')
#         self.order()
# 
#     def dequeue(self):
#         # Nothing to send
#         if (len(self.queue) == 0): return None
# 
#         # Will only pop from head of queue
#         head = self.queue[0]
# 
#         '''
#         If the entry at the head is granted, send a packet. If sending
#         that packet causes the entry to be completely sent, then we
#         are done and destroy it.
#         '''
#         if (head.priority == 0):
#             # Send one packet
#             # head.payload -= 1
#             head.payload.pop(-1)
#             head.grant   -= 1
# 
#             # Did we send the whole message?
#             if (head.exhausted()):
#                 self.queue.pop(0)
#             elif (head.grant == 0):
#                 head.priority = 1
# 
#             self.order()
#             return head 
#         else:
#             self.order()
#             # There can be no possible active entries in the queue
#             return None
# 
#     def grant(self, grant):
#         grant.priority = 2
#         self.queue.insert(0, grant)
#         self.order()
# 
#     def order(self):
#         if (len(self.queue) != 0 and self.queue[-1].priority == 2):
#             self.queue.pop(-1)
# 
#         # swap (0,1) (2,3) (4,5) (6,7) 
#         for even in range(int(len(self.queue)/2)):
#             low  = self.queue[even]
#             high = self.queue[even+1]
# 
#             if (low.priority > high.priority):
#                 # Is this a grant
#                 if (low.priority == 2 and low.id == high.id):
#                     # print("REACTIVE")
#                     high.grant += low.grant
#                     high.priority = 0
#                 self.queue[even+1] = low
#                 self.queue[even]   = high
#             elif (low.priority == high.priority and low.payloadSize() > high.payloadSize()):
#                 self.queue[even+1] = low
#                 self.queue[even]   = high
# 
#         # swap (1,2) (3,4) (5,6) (7,8)
#         for odd in range(int((len(self.queue)-1)/2)):
#             low  = self.queue[odd+1]
#             high = self.queue[odd+2]
# 
#             if (low.priority > high.priority):
#                 # Is this a grant
#                 if (low.priority == 2 and low.id == high.id):
#                     # print("REACTIVE")
#                     high.grant += low.grant
#                     high.priority = 0
#                 self.queue[odd+2] = low
#                 self.queue[odd+1] = high
#             elif (low.priority == high.priority and low.payloadSize() > high.payloadSize()):
#                 self.queue[odd+2] = low
#                 self.queue[odd+1] = high
# 
#     def tick(self, time):
#         self.order()
#  
# 
# class PIFO(Queue):
#     def __init__(self):
#         super().__init__("PIFO")
# 
#         # entries which cannot be inserted into the queue due fc state
#         self.blocked = []
# 
#     def enqueue(self, entry):
#         # If this entry has granted bytes, find its place in the queue, otherwise buffer
#         if (entry.grant != 0):
#             # Find the first entry in the queue that has a byte count greater then our new entry's byte count
#             insert = next((e[0] for e in enumerate(self.queue) if e[1].payload > entry.payload), len(self.queue))
#             self.queue.insert(insert, entry)
#         else:
#             self.blocked.append(entry)
# 
#     def dequeue(self):
#         # Nothing to send
#         if (len(self.queue) == 0): return None
# 
#         # PIFO will only pop from head of queue
#         head = self.queue[0]
# 
#         '''
#         If the entry at the head is granted, send a packet. If sending
#         that packet causes the entry to be completely sent, then we
#         are done and destroy it.
#         '''
#         if (head.grant != 0):
#             # Send one packet
#             # head.payload -= 1
#             head.payload.pop(-1)
#             head.grant   -= 1
# 
#             # Did we send the whole message?
#             if (head.exhausted()):
#                 self.queue.pop(0)
#             elif (head.grant == 0):
#                 self.queue.pop(0)
#                 self.blocked.append(head)
# 
#             return head 
#         else:
#             # The entry is not granted and we will block it
#             self.queue.pop(0)
#             self.blocked.append(head)
# 
#             return None
# 
#     def grant(self, grant):
#         # this is fine because when the entry gets to the head of the queue it can access this data immediately
#         loc = next((e[1] for e in enumerate(self.queue) if e[1].id == grant.id), None)
# 
#         if loc != None:
#             loc.grant += grant.grant
# 
#         loc = next((e[1] for e in enumerate(self.blocked) if e[1].id == grant.id), None)
# 
#         if loc != None:
#             loc.grant += grant.grant
#             self.enqueue(loc)
#             self.blocked.remove(loc)
# 
#     def tick(self, time):
#         None
 
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


