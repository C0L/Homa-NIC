import random

# Add RTT of delay in here, which will dicate the receiver grant size
# This is just a mesh of lists for each receiver
class Network():
    # This routes data from ingress to egress
    def __init__(self, channels, config):
        # Packets coming into the switch
        self.ingress = [[] for i in range(channels)]

        # Packets going out of the switch
        self.egress  = [[] for i in range(channels)]

        self.config = config
        self.channels = channels

        self.time = 0

    def peek(self, id):
        return (len(self.ingress[id]) < self.config['bufferDepth'])

    # Outgoing packets towards the network
    def write(self, dest, message):
        # Add to the respective buffer the time of insertion and the message
        if (len(self.ingress[dest]) < self.config['bufferDepth']):
            self.ingress[dest].append((self.time, message))
            return True

        return False 

    # Incoming packets coming from the network
    def read(self, dest):
        # Are there any pending elements on the network
        if (self.egress[dest]):
            if (self.egress[dest][0][0] + int(self.config['intrinsicDelay']/2) < self.time):
                return self.egress[dest].pop(0)[1]
        return None

    def tick(self):
        random.shuffle(self.ingress)
        for buffer in self.ingress:
            if (len(buffer)):
                head = buffer[-1]
                dest = self.egress[head[1].message.dest] 
                if (len(dest) < self.config['bufferDepth']):
                    buffer.remove(head)
                    dest.append(head)

        self.time += 1
