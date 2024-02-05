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

# Why not use the dumb 1 queue of things

Seperate implementation from the high level function of the queue(insertion and mutability)

find and jot down some dumb examples

# What flow control system to implement and does that mean I need to simulate other users in the system? Or is there a way to do this more abstractly detached from the greater network context.

class simulator():
    msgbuffers = [] 
    __init__(self, h2c_msg_):

class host():
    __init__(self): 

class pifo():


if __name__ == '__main__':
    simulator()
