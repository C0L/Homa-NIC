import random

# Add RTT of delay in here, which will dicate the receiver grant size
# This is just a mesh of lists for each receiver
class Network():
    # This routes data from ingress to egress
    def __init__(self, channels, config):
        # Packets coming into the switch
        # self.ingress = [[] for i in range(channels)]

        # Packets going out of the switch
        self.egress  = []

        self.config = config
        self.channels = channels

        self.time = 0

    # Outgoing packets towards the network
    def write(self, dest, packet):
        # Add to the respective buffer the time of insertion and the message
        if (len(self.egress) < self.config['bufferDepth']):
            self.egress.append((self.time, dest, packet))
            return True

        return False 

    # Incoming packets coming from the network
    def read(self, dest):
        match = None
        # Are there any pending elements on the network
        for buffered in self.egress:
            if buffered[1] == dest and (buffered[0] + self.config['delay']) < self.time:
                match = buffered
                break

        if match != None:
            self.egress.remove(match)
            return match[2]

        return None

    def tick(self):
        self.time += 1
