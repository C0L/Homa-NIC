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
'''





















# * Why a mutable priority queue?
# 
# PIFO does rate limiting on the input side before it is enqueued into the pifo. 
# 
# ** Bulk entry activation
# If multiple messages from a single sender are destined for a single receiver, and that receiver is oversubscribed, the outoging messages will need to be stashed. When the receiver eventually becomes mavailible all of the blocked entries will need to be added to the priority queue to find the best choice. Adding the entries to the queue will also block unrelated messages from being added to the queue (can only add a single element per cycle)
# 
# ** Dequeue Dry-fires
# If we add all of the messages destined for a single receiver while that receiver is availible, and the receiver subsequently goes unavailible, then when these messages come to the head of the queue we have a dry-fire. This is a wasted few cyles where we do not know what the best message to send is. If there were a lot of messages of this type then the delay will be significant.
# 
# ** Premption
# Assume we are selectively adding messages into the priority queue based on the number of availible bytes the receiver has granted. If a receiver grants us 10 bytes and we have 3 messages for that receiver with (2,3,4) bytes we can add them all into the queue. If find that we want to send a new message with 1 byte then we want to add it to the queue, but we are going to exceed the 10 bytes granted to us. If we did not add it to the queue then we are not prempting.
# 
# ** Per-flow Scheduling Policies
# 
# PIFO is unable to update per-flow and cannot implement the anti-starvation policy of pFabric.
# 
# ** Weight Updates?
# 
# 
# ** Multi-level PQ?
# 
# 
# No change in eligibility - all jobs are eligible
# 
# A job goes from active during insert to inactive after insert
# PIFO waits until it gets to the front and dry fires
# 
# A job goes from inactive on insert to active after insert

# The time between when a set of jobs arrives and when a job is activated? Do we let the queue stabalize?

# Make the simplifying assumption that a stable state is reached?

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

# Questions:
# One of our central claims is that the mutability of the relative scheduling order of arbitrary flows is valuable. There are a number of places I think this could be used but in our simulations I think we are going to begin by showing it is valuable to mutate entries in the priorty queue in response to changing flow control state and cache state. We want to compare against PIFO which does not have mutability for arbirary flows. When I simulate this I need some realistic penelty for PIFO deciding to send a packet and the data for that packet not being cached on chip, and the ineligibility of a receiver to accept the data.

# The cache problem is more straightforward. PIFO would effectively trigger a cache miss every time it scheduled a packet which is what the deployments of PIFOs on NICs so far have done. Our queue would trigger a cache miss only if our prefetcher system had failed to collect the data. That seems like a reasonable comparison.

# The comparison is less straight forward if the PIFO schedules a packet to be sent that is not eligible to receive data. I have not seen any work yet that considers the relationship between a PIFO scheduler and the flow control state to compare against, so if the PIFO selects a packet and we know the receiver is unable to accept, do we just send it anyway, do we block, do we defer that data till later. I am not sure what is a realistic comparison because no one has really considered this.

# Better examples of needing to updating entries

# Multiple messages bound for the same receiver
# Message is desirable, becomes undesirable, flow control state changes (unrealistic)
# Great triggering the reorderring of many outgoing messages
# Systems that are not all or nothing (message has some percentage of the reeivers bandwidth)

# Why not use the dumb 1 queue of things

# Seperate implementation from the high level function of the queue(insertion and mutability)

# find and jot down some dumb examples

# What flow control system to implement and does that mean I need to simulate other users in the system? Or is there a way to do this more abstractly detached from the greater network context.

class simulator():
    msgbuffers = [] 
    __init__(self, h2c_msg_):

class host():
    __init__(self): 

class pifo():


if __name__ == '__main__':
    simulator()
