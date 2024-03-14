package gpnic

import chisel3._
import chisel3.util._

// trait PriorityQueable {
//   // Can the entry be popped from the head of the queue
//   // def popable(): Bool
// 
//   // Pop from this entry, and return whether it is now empty
//   // def pop(): Bool
// 
//   def <(that: Any): Bool
// }

// https://stackoverflow.com/questions/54792791/scala-abstract-comparison-method-in-trait

// class PrefetcherEntry extends Bundle with PriorityQuable {
//   val priority  = UInt(3.W)
//   val granted   = UInt(20.W)
//   val dbuffered = UInt(20.W)
//   val remaining = UInt(20.W)
//   val dbuff_id  = UInt(10.W)
//   val rpc_id    = UInt(16.W)
// }

// class SendPacketEntry extends Bundle with PriorityQueable {
//   val priority  = UInt(3.W)
//   val granted   = UInt(20.W)
//   val dbuffered = UInt(20.W)
//   val remaining = UInt(20.W)
//   val dbuff_id  = UInt(10.W)
//   val rpc_id    = UInt(16.W)
// 
//   // override def popable(): Bool = {
// 
//   // }
// 
//   // overwride def pop(): Bool = {
// 
//   // }
// 
//   // LHS is "this" and RHS is "that"
//   override def <(that: SendPacketEntry): Bool = {
//     val swap = Wire(Bool())
//     when (this.remaining < that.remaining) {
//       swap := true.B
//     }.otherwise {
//       swap := false.B
//     }
//     swap
//     // TODO just compare remainint
//     // If the priorities do not match we will swap regardless
//     // when (this.priority =/= that.priority && this.priority < that.priority) {
//     //   // Do swap
//     //   when (this.dbuff_id === that.dbuff_id && this.priority === DBUFF_UPDATE) {
//     //     that.dbuffered := this.dbuffered
//     //     that.priority  := ACTIVE
//     //     true.B
//     //   }.elsewhen (this.rpc_id === that.rpc_id && this.priority === GRANT_UPDATE) {
//     //     that.granted := this.granted
//     //     that.priority := ACTIVE
//     //     true.B
//     // // If the priorities do match we will swap only if the remaining bytes varies
//     // }.elsewhen (this.priority == that.priority && this.remaining < that.remaining) {
//     //   true.B
//     // // Otherwise we will not swap
//     // }.otherwise {
//     //   // Don't swap
//     //   false.B
//     //   }
//     // }
//   }
// }

// TODO overide comparison operator?

// trait PriorityQueable[A <: PriorityQueable[A]] extends Bundle { this: A =>
//   def <(that: A): Bool 
// }

trait PriorityQueable extends Bundle {
  def active: Bool

  def pop(fetchSize: UInt): PriorityQueable
  def <(that: PriorityQueable): Bool 
}

class PrefetcherEntry extends Bundle with PriorityQueable {
  val localID   = UInt(16.W) // The RPC ID of this message
  val dbuffID   = UInt(10.W) // Cache for this RPC
  val fetchable = UInt(16.W) // How many bytes we are eligible to fetch
  val totalRem  = UInt(20.W) // Determines the overall priority
  val totalLen  = UInt(20.W) // Total message length
  val active    = Bool() // Whether this entry is valid

  // True means remove this entry, false means it stays
  override def pop(fetchSize: UInt): PriorityQueable = {
    val current = Wire(new PrefetcherEntry)
    current := this
    when (this.fetchable < fetchSize) {
      current.active := false.B
    }.otherwise {
      // printf("DECREMENT FETCHABLE\n")
      current.fetchable := this.fetchable - fetchSize
      current.totalRem  := this.totalRem - fetchSize
    }
    current
  }

  // LHS is "this" and RHS is "that"
  override def <(that: PriorityQueable): Bool = that match {
    case that: PrefetcherEntry =>  {
      val result = Wire(Bool())
      when (that.active === false.B || this.totalRem < that.totalRem) {
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

    val increment = Input(UInt(32.W))
  })

  io.enqueue.ready := true.B

  val queue = Seq.fill(size)(RegInit(entry, 0.U.asTypeOf(entry)))

  val outputQueue = Module(new Queue(new PrefetcherEntry, 1, true, false))

  outputQueue.io.deq <> io.dequeue
  outputQueue.io.enq.valid := false.B
  outputQueue.io.enq.bits  := 0.U.asTypeOf(new PrefetcherEntry)

  outputQueue.io.enq.valid := false.B
  outputQueue.io.enq.bits := queue(0)

  when (io.enqueue.valid) {
    val matchMask = queue.map { elem => io.enqueue.bits < elem}
    val insertIndex = PriorityEncoder(matchMask)

    // could pipeline this
    // TODO This is unsafe
    for (entry <- 0 to size - 2) {
      when (entry.U === insertIndex) {
        // printf("SET ENTRY %d\n", entry.U)
        queue(entry) := io.enqueue.bits
        queue(entry).active := true.B
      }

      // TODO overriding entry

      when (matchMask(entry) === 1.U) {
        // printf("SHIFT %d\n", entry.U)
        queue(entry+1) := queue(entry)
      }
    }
  }.elsewhen (queue(0).active && outputQueue.io.enq.ready) {
    // printf("DEQUEUE\n")
    outputQueue.io.enq.valid := true.B

    val pop = queue(0).pop(io.increment)
    when (pop.active =/= true.B) {
      for (entry <- 0 to size - 2) {
        queue(entry) := queue(entry+1)
      }
    }.otherwise {
      // printf("UPDATE HEAD\n")
      queue(0) := pop
    }
  }
  
}
