import math

class QueueEntry:
    def __init__(self, message, priority, cycles, incr):
        self.message = message
        self.priority = priority
        self.cycles = cycles
        self.increment = incr

class BisectedPriorityQueue():
    def __init__(self, config):
        # Fast on chip single-cycle priority queue
        self.onChip  = PriorityQueue(config['onChipDepth'], config['onChipLatency'])

        # Slow off chip multi-cycle priority queue
        self.offChip = PriorityQueue(config['offChipDepth'], config['offChipLatency'])

    def enqueue(self, entry):
        if (not self.onChip.full() or entry.priority < self.onChip.tail()):
            # If the onChip queue is not full insert into it
            self.onChip.enqueue(entry)
        else:
            # Otherwise place in offChip queue
            self.offChip.enqueue(entry)

    def dequeue(self):
        if (not self.onChip.empty()):
            return self.onChip.dequeue()
        return None

    def cycle(self):
        carryUp = self.onChip.carryUp()
        if (carryUp != None):
            # print("carry up")
            self.offChip.enqueue(carryUp)

        if (not self.onChip.full() and not self.offChip.empty()):
            carryDown = self.offChip.carryDown()
            if (carryDown != None):
                # print("carry down")
                self.onChip.enqueue(carryDown)

        self.onChip.time  += 1
        self.offChip.time += 1

# TODO need to impose delay
class PriorityQueue():
    def __init__(self, maxElements, delay):
        # Every delay number of cycles the best value will be updated
        
        self.queue = []

        self.delay = delay
        self.maxElements = maxElements

        self.time = 0

    def enqueue(self, entry):
        # TODO dequeue the current best element
        self.queue.append(entry)
        
    def dequeue(self):
        # If we are storing no entries there is no packet to send
        if (len(self.queue) == 0): return None

        # Find the highest priority elements
        ideal = min(self.queue, key=lambda entry: entry.priority)

        # Reduce the priority as we send one packet
        ideal.priority = max(0, ideal.priority - ideal.increment)
        ideal.cycles   = max(0, ideal.cycles - ideal.increment)

        if (ideal.priority == 0 or ideal.cycles == 0):
            self.queue.remove(ideal)

        return ideal

    def carryDown(self):
        if (self.time % self.delay == 0):
            # Find the highest priority elements
            ideal = min(self.queue, key=lambda entry: entry.priority)
            self.queue.remove(ideal)
            return ideal
        else:
            return None

    def carryUp(self):
        if (len(self.queue) > self.maxElements):
            worst = max(self.queue, key=lambda entry: entry.priority)
            self.queue.remove(worst)
            return worst 
        else:
            return None

    def cycle(self):
        self.time += 1
            
    def tail(self):
        worst = max(self.queue, key=lambda entry: entry.priority)
        return worst.priority

    def empty(self):
        return len(self.queue) == 0

    def full(self):
        return len(self.queue) == self.maxElements
