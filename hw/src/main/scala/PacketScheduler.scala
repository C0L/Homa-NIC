package gpnic

import chisel3._
import chisel3.util._

/* PacketScheduler - Responsible for deciding the next packet of data
 * to send out. Packet eligibility is determined by grant status and
 * data availability status.
 */ 
class PacketScheduler extends Module {
  val io = IO(new Bundle {
    // TODO update types
    val newMessage = Flipped(Decoupled(new QueueEntry)) // Messages needing to be managed by scheduler
    val newGranted = Flipped(Decoupled(new QueueEntry)) // Arrived grant notifications
    val newCached  = Flipped(Decoupled(new QueueEntry)) // Arrived cached data notifications

    val sendPacket = Decoupled(new QueueEntry) // The next packet to be sent out onto the network
  })

  // Parallel compare priority queue for packet send entries
  val packetQueue = Module(new PriorityQueue(new SendPacketEntry, 32))

  // On newMessage
  //   Insert data being tracked into BRAM

  // On newGranted
  //   read data from BRAM and update
  //   check if granted and cached, if so insert into pqueue

  // On newCached
  //   read data from BRAM and update
  //   check if granted and cached, if so insert into pqueue
}
