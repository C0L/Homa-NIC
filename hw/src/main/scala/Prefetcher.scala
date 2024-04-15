package gpnic

import chisel3._
import chisel3.util._

/* PayloadCache - Responsible for prefetching packet payload data.
 * Issues DMA read requests, stores associated state while request is
 * outstanding, issues notifcations when requests are complette to the
 * rest of the system.
 */ 
class Prefetcher extends Module {
  val io = IO(new Bundle {
    val logReadSize  = Input(UInt(16.W)) // Number of bytes to fetch per cycle for an RPC
    val newFetchable = Flipped(Decoupled(new PrefetcherState)) // New RPCs to fetch data for
    val newFree      = Flipped(Decoupled(UInt(10.W))) // Opens cache window for refered dbuff ID
    val fetchRequest = Decoupled(new DmaReq) // DMA read requests to fetch data
    val fetchCmpl    = Flipped(Decoupled(new DmaReadStat)) // Completions from DMA core
    val fetchNotif   = Decoupled(new QueueEntry) // Notifies the rest of system of cache updates
  })
/*
  // Parallel compare priority queue for fetch entries
  val fetchQueue = Module(new PriorityQueue(new PrefetcherEntry, 32))

  // Data associated with the entries in the priorty queue
  val state = SyncMem(32, new PrefetcherState)

  // Tie new prefetcher entry to enqueue of the priority queue
  fetchQueue.io.enqueue.bits  := io.newFetchable.entry(io.logReadSize) 
  io.newFetchable.ready       := fetchQueue.io.enqueue.ready
  fetchQueue.io.enqueue.valid := io.newFetchable.valid

  // When a new priority queue entry is created store the metadata in memory
  when (fetchQueue.io.enqueue.fire) {
    state.write(io.newFetchable.bits.dbuffID, io.newFetchable.bits)
  }

  // Read metadata associated with priority queue entry
  val dbuffRead = state.read(fetchQueue.io.dequeue.bits.dbuffID)

  // Requests to DMA
  io.fetchRequest.bits  := ....dmaRead
  io.fetchRequest.valid := fetchQueue.io.dequeue.valid
  fetchQueue.io.dequeue.ready := io.fetchRequest.ready

  // TODO the send queue should always be ready?
  val notifQueue = Module(new Queue(new ReqMem, 16, true, false))

  io.fetchCmpl.ready      := notifQueue.io.enq.ready // TODO meaningless
  notifQueue.io.enq.valid := io.fetchCmpl.valid
  notifQueue.io.enq.bits  := pendTag

  io.fetchNotif.valid          := notifQueue.io.deq.valid
 notifQueue.io.deq.ready      := io.fetchNotif.ready
 */
}



  // TODO need to simplify this and reintroduce the window
  // TODO temporary disabled
  // io.newFree.bits := DontCare
  // io.newFree.valid := DontCare
  // io.newFree.ready := false.B
