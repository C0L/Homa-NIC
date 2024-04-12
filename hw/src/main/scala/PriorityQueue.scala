package gpnic

import chisel3._
import chisel3.util._

trait PriorityQueable extends Bundle {
  def active: Bool

  def pop(): PriorityQueable
  def <(that: PriorityQueable): Bool 
}

// TODO need a comparator function which is able to find RPCs of the same ID to avoid having to do updates. This would allow for more requests to be dispatched with appropriate priortiy.

// class PacketEgressEntry extends Bundle with PriorityQueable {
//   val localID   = UInt(16.W) // The RPC ID of this message
//   val dbuffID   = UInt(10.W) // Cache for this RPC
//   val cached    = UInt(20.W) // How many bytes we have cached on chip
//   val granted   = UInt(20.W) // How many bytes we are granted to send
//   // val totalRem  = UInt(20.W) // Determines the overall priority
//   val totalLen  = UInt(20.W) // Total message length
//   val active    = Bool() // Whether this entry is valid
// 
//   // True means remove this entry, false means it stays
//   override def pop(fetchSize: UInt): PriorityQueable = {
//     // val current = Wire(new PrefetcherEntry)
//     // current := this
//     // when (this.fetchable < fetchSize) {
//     //   current.active := false.B
//     // }.otherwise {
//     //   // printf("DECREMENT FETCHABLE\n")
//     //   current.fetchable := this.fetchable - fetchSize
//     //   current.totalRem  := this.totalRem - fetchSize
//     // }
//     // current
//   }
// 
//   // LHS is "this" and RHS is "that"
//   override def <(that: PriorityQueable): Bool = that match {
//     // case that: PrefetcherEntry =>  {
//     //   val result = Wire(Bool())
//     //   when (that.active === false.B || this.totalRem < that.totalRem) {
//     //     result := true.B
//     //   }.otherwise {
//     //     result := false.B
//     //   }
//     //   result
//     // }
//     // case _ => throw new UnsupportedOperationException("Wrong comparison")
//   }
// }


/*
 * Fetch Flow: Delegete enqueues a single element of message length total length and cacheline bufferable size. Dispatches are made for that entry which cause bufferable to decrease. Those read requests are made and the outstanding queue entries are stored in the tag buffer. We can even stack entries going out. So if multiple of the same requests are the same RPC ID they are the same tag entry. When the read returns we can add a new entry to the priorty queue with the number of bytes remianing - one full cachline size, then we insert it at that priority but behind the current one in the queue?? But this is not the most exact spot?
 *  Use the tag module to store the "final output". The satisfaction of that could cause the reinsertion into the queue. But then this won't pipeline correctly? We want the first byte of receiver data to reopen the window. 
 */

// class PrefetcherEntry extends Bundle with PriorityQueable {
//   val localID   = UInt(16.W) // The RPC ID of this message
//   val dbuffID   = UInt(10.W) // Cache for this RPC
//   val fetchable = UInt(20.W) // How many bytes we are eligible to fetch
//   val totalRem  = UInt(20.W) // Determines the overall priority
//   val totalLen  = UInt(20.W) // Total message length
//   val active    = Bool() // Whether this entry is valid
// 
//   // True means remove this entry, false means it stays
//   override def pop(fetchSize: UInt): PriorityQueable = {
//     val current = Wire(new PrefetcherEntry)
//     current := this
//     when (this.fetchable < fetchSize) {
//       current.active := false.B
//     }.otherwise {
//       // printf("DECREMENT FETCHABLE\n")
//       current.fetchable := this.fetchable - fetchSize
//       current.totalRem  := this.totalRem - fetchSize
//     }
//     current
//   }
// 
//   // LHS is "this" and RHS is "that"
//   override def <(that: PriorityQueable): Bool = that match {
//     case that: PrefetcherEntry =>  {
//       val result = Wire(Bool())
//       when (that.active === false.B || this.totalRem < that.totalRem) {
//         result := true.B
//       }.otherwise {
//         result := false.B
//       }
//       result
//     }
//     case _ => throw new UnsupportedOperationException("Wrong comparison")
//   }
// }

class PrefetcherState extends Bundle {
  val localID   = UInt(16.W) // The RPC ID of this message
  val dbuffID   = UInt(10.W) // Cache for this RPC
  val fetchable = UInt(20.W) // How many bytes we are eligible to fetch
  val totalRem  = UInt(20.W) // Determines the overall priority
  val totalLen  = UInt(20.W) // Total message length
}

class PrefetcherEntry extends Bundle with PriorityQueable {
  val dbuffID  = UInt(5.W)  // Cache for this RPC
  val priority = UInt(10.W) // Priority of this entry
  val active   = Bool()     // Whether this entry is valid

  // True means remove this entry, false means it stays
  override def pop(): PriorityQueable = {
    val current = Wire(new PrefetcherEntry)
    current := this
    when (this.priority < 1.U) {
      current.active := false.B
    }.otherwise {
      current.priority := this.priority - 1.U 
    }
    current
  }

  // LHS is "this" and RHS is "that"
  override def <(that: PriorityQueable): Bool = that match {
    case that: PrefetcherEntry =>  {
      val result = Wire(Bool())
      when (that.active === false.B || this.priority < that.priority) {
        result := true.B
      }.otherwise {
        result := false.B
      }
      result
    }
    case _ => throw new UnsupportedOperationException("Wrong comparison")
  }
}


class PriorityQueue[T <: PriorityQueable] (entry: T, size: Int) extends Module {
  val io  = IO(new Bundle {
    val enqueue = Flipped(Decoupled(entry))
    val dequeue = Decoupled(entry)

    // val increment = Input(UInt(32.W))
  })

  io.enqueue.ready := true.B

  val queue = Seq.fill(size)(RegInit(entry, 0.U.asTypeOf(entry)))

  val outputQueue = Module(new Queue(new PrefetcherEntry, 1, true, false))

  outputQueue.io.deq <> io.dequeue
  outputQueue.io.enq.valid := false.B
  outputQueue.io.enq.bits := queue(0)

  when (io.enqueue.valid) {
    val matchMask = queue.map { elem => io.enqueue.bits < elem}
    val insertIndex = PriorityEncoder(matchMask)

    // could pipeline this
    for (entry <- 0 to size - 2) {
      when (entry.U === insertIndex) {
        queue(entry) := io.enqueue.bits
        queue(entry).active := true.B
      }

      when (matchMask(entry) === 1.U) {
        queue(entry+1) := queue(entry)
      }
    }
  }.elsewhen (queue(0).active && outputQueue.io.enq.ready) {
    outputQueue.io.enq.valid := true.B

    val pop = queue(0).pop()
    when (pop.active =/= true.B) {
      for (entry <- 0 to size - 2) {
        queue(entry) := queue(entry+1)
      }
    }.otherwise {
      queue(0) := pop
    }
  }
}
