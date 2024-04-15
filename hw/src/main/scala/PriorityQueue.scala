package gpnic

import chisel3._
import chisel3.util._

/* PriorityQueue - Performs parallel comparison of new elements will
 * all elements in the queue to determine insertion location. Pops
 * from the head of the queue to get the highest priority elements.
 * Relies on the entries to be "PriorityQueable" which means they
 * override the pop(), +, ===, and < operator. The + operator defines
 * update operations (how to combine a new entry with an entry that
 * matches). The === defines whether two entries should match and an
 * update should be performed. The < operator is used to compare
 * priority.
 */
class PriorityQueue[T <: PriorityQueable] (entry: T, size: Int) extends Module {
  val io  = IO(new Bundle {
    val enqueue = Flipped(Decoupled(entry)) // New entries to add to the queue
    val dequeue = Decoupled(entry)          // Entries poped from the head of queue
  })

  io.enqueue.ready := true.B

  val queue = Seq.fill(size)(RegInit(entry, 0.U.asTypeOf(entry)))

  val outputQueue = Module(new Queue(new PrefetcherEntry, 1, true, false))

  outputQueue.io.deq <> io.dequeue
  outputQueue.io.enq.valid := false.B
  outputQueue.io.enq.bits := queue(0)

  when (io.enqueue.valid) {
    val priorityMask  = queue.map {elem => io.enqueue.bits < elem}
    val priorityIndex = PriorityEncoder(priorityMask)

    for (entry <- 0 to size - 2) {
      when (entry.U === priorityIndex) {
        queue(entry) := io.enqueue.bits
        queue(entry).active := true.B
      }

      when (priorityMask(entry) === 1.U) {
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
