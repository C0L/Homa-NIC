# One object on the network
class DataPacket:
    def __init__(self, message):
        # TODO should hold range of data
        self.message = message

class GrantPacket:
    def __init__(self, message):
        self.message = message
    
class Message:
    def __init__(self, src, dest, length, unscheduled, startTime):
        self.src  = src 
        self.dest = dest

        self.length = length

        self.senderSent    = 0
        self.senderGranted = unscheduled

        self.receiverGranted  = unscheduled
        self.receiverReceived = 0

        self.startTime = startTime
        self.endTime   = 0

        self.grantEntry = None
        self.dataEntry = None

    def __repr__(self):
        return f'''
        Source: {self.src}
        Dest: {self.dest}
        Length: {self.length}
        Start: {self.startTime}
        End: {self.endTime}
        '''
