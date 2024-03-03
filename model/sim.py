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

class Entry:
    def __init__(self, id, payload, grant):
        self.id      = id
        self.payload = payload
        self.grant   = grant
        self.st      = 0
        self.et      = 0

    def __repr__(self):
        return f'ID: {self.id}, Payload: {self.payload}, Grant: {self.grant}'

class Queue:
    def __init__(self, name):
        # Entries that have been completed by the queue
        self.complete = []

        # Tried to send an ungranted packet
        self.dryfires = 0

        # The name of the queue for printing purposes
        self.name = name 

    def minAvgCmplTime(self):
        sumDir = 0
        for entry in self.complete:
            sumDir += (entry.et - entry.st)

        return sumDir / len(self.complete)

    def dumpStats(self):
        print(f'Queue: {self.name} Statistics:')
        print(f'   - Dryfires  : {self.dryfires}')
        print(f'   - MinAvgCmpl: {self.minAvgCmpl()}')
        # number of inaccuracies
        # number of dry fires (no packet goes out?)

class PIFO(Queue):
    def __init__(self, fcOracle):
        super().__init__(fcOracle)

        # entries which cannot be inserted into the queue due fc state
        self.blocked = []

        # entries with packets to send
        self.queue   = []

    def enqueue(self, entry):
        # If this entry has granted bytes, find its place in the queue, otherwise buffer
        if (self.fcOracle(entry)):
            # Find the first entry in the queue that has a byte count greater then our new entry's byte count
            insert = next(e[0] for e in enumerate(self.queue) if e[1].bytes > entry.bytes)
            self.queue.insert(insert, entry)
        else:
            self.blocked.append(entry)

    def dequeue(self):
        # If we are storing no entries there is no packet to send
        if (len(self.queue)): return None

        # PIFO will only pop from head of queue
        head = self.queue[0]

        '''
        If the entry at the head is granted, send a packet. If sending
        that packet causes the entry to be completely sent, then we
        are done and destroy it.
        ''' 
        if (self.fcOracle(head) != 0):
            # Send one packet
            self.queue[0].bytes - 1

            # Did we send the whole message?
            if (self.queue[0].bytes == 0):
                self.queue.pop()

            return head
        else:
            # The entry is not granted and we will block it
            self.queue.pop()
            self.blocked.append(head)
            self.dryfires += 1
            return None

    def tick(self):
        update = [i for i,v in enumerate(a) if v > 2] 
        # Iterate through all entries and check FC state, enqueue if needed
        

class Mutable(Queue):
    def __init__(self, fcOracle):
        super(fcOracle)

        self.queue = []

    def enqueue(self, entry):
        self.queue.insert(0, entry)

    def dequeue(self):
        # If we are storing no entries there is no packet to send
        if (len(self.queue)): return None

        # PIFO will only pop from head of queue
        head = self.queue[0]

        '''
        If the entry at the head is granted, send a packet. If sending
        that packet causes the entry to be completely sent, then we
        are done and destroy it.
        ''' 
        if (self.fcOracle(head) != 0):
            # Send one packet
            self.queue[0].bytes - 1

            # Did we send the whole message?
            if (self.queue[0].bytes == 0):
                self.queue.pop()

            return head
        else:
            # The entry is not granted. Should only happen when there
            # are no active messages in the entire system
            self.dryfires += 1

    def tick(self):
        None
        # perform the swap operations
        # Check if there are updates to fc state, and enqueue update signal if needed

class Ideal(Queue):
    def __init__(self):
        super().__init__('Ideal')

        # just use a list to store the elements and always search the elements
        self.queue = []

    def enqueue(self, entry):
        print(f'enqueue: {entry}')
        self.queue.append(entry)
        
    def dequeue(self):
        # If we are storing no entries there is no packet to send
        if (len(self.queue) == 0): return None

        # TODO cleaner to remove if empty
        ideal = min(self.queue, key=lambda entry: math.inf if entry.grant == 0 or entry.payload == 0 else entry.payload)

        # Is there actually bytes to send, otherwise empty cycle nothing to do
        if (ideal.grant != 0 and ideal.payload != 0):
            # print(f'dequeue: {ideal}')
            ideal.payload -= 1
            ideal.grant   -= 1
            if (ideal.payload == 0):
                self.complete.append(ideal)
        else:
            self.dryfires += 1
        #     # The entry is not granted and we will block it
        #     self.queue.pop()
        #     self.blocked.append(head)
        #     self.dryfires += 1
            return None

        # return ideal

    def tick(self):
        None

# class FcOracle:
#     def __init__(self):
#         self.fcState = {}
# 
#     '''
#     Given some entry, return the number of bytes that can be send
#     '''
#     def fcOracle(self, entry):
#         None
#         # TODO return the number of bytes that can be sent for an entry
# 
#     '''
#     Mutate the flow control sate based on fcFunc
#     '''
#     def fcUpdate():
#         None
#         # TODO mutate flow control state
# 
#     '''
#     Add a new entry needing flow control managment by invoking the
#     flow control function for an initial value 
#     '''
#     def fcInclude(self, entry):
#         None
        # self.fcState[entry.id] = granted
        # TODO add a new entry to be flow control managed, call the fc function, and return the initial assignment

class Sim:
    def __init__(self):
        # Tested queuing strategies
        self.queues  = [Ideal()] # [PIFO(self.fc.fcOracle), Ideal(self.fc.fcOracle), Mutable(self.fc.fcOracle)]

        # Timestep counter
        self.time = 0

        # Initial ID to assign to new entries generated by the sendmsg function
        self.id = 0

        # Maximum number of timesteps to run experiment
        self.maxSimTime = 10000

        # Poisson distribution sendmsg requests
        self.messages = [0] * self.maxSimTime

        # Poisson arrival times + poisson packet sizes for outgoing messages
        t = 0
        for i in range(self.maxSimTime):
            t += random.expovariate(.2)
            if (math.floor(t) < self.maxSimTime):
                # .1 = 1/desired mean
                # Compute the number of units of payload
                payload = math.floor(random.expovariate(.1))
                granted = math.floor(random.expovariate(.1))

                # Construct a new entry to pass to the queues
                entry = Entry(self.id, payload, granted)

                # Move to the next message ID
                self.id += 1

                # Insert the new entry at the poisson process arrival time
                self.messages[math.floor(t)] = entry

        print(self.messages)

    '''
    Iterate through simulation steps up to the max simulation time
    ''' 
    def simulate(self):
        for t in range(self.maxSimTime):
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
        message = self.messages[self.time]

        # if (message != 0):
        #      self.fc.fcInclude(message)

        for queue in self.queues:
            if (message != 0):
                queue.enqueue(message) # pass the sendmsg to the queue if it exists
            packet = queue.dequeue()   # get a packet out from the queue

    # If both the packet size and send time are poisson it models a M/M/1 queue?
    def dumpStats(self):
         
        for queue in self.queues:
            queue.dumpStats()

if __name__ == '__main__':
    sim = Sim() 
    sim.simulate()
    sim.dumpStats()
