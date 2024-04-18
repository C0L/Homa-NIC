import math
import statistics

class QueueEntry:
    def __init__(self, message, priority, cycles, incr):
        self.message = message
        self.priority = priority
        self.cycles = cycles
        self.increment = incr

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
