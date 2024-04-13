package gpnic

import chisel3._
import chisel3.util._

class PriorityQueue[T <: PriorityQueable] (entry: T, size: Int) extends Module {
  val io  = IO(new Bundle {
    val enqueue = Flipped(Decoupled(entry))
    val dequeue = Decoupled(entry)
  })

  io.enqueue.ready := true.B

  val queue = Seq.fill(size)(RegInit(entry, 0.U.asTypeOf(entry)))

  val outputQueue = Module(new Queue(new PrefetcherEntry, 1, true, false))

  outputQueue.io.deq <> io.dequeue
  outputQueue.io.enq.valid := false.B
  outputQueue.io.enq.bits := queue(0)

  when (io.enqueue.valid) {
    val matchMask = queue.map {elem => io.enqueue.bits < elem}
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
