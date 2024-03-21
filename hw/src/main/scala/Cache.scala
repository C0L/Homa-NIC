package gpnic

import chisel3._
import chisel3.util._

// TODO should have flow control on data fetch priority queue output
// TODO no flow control on DMA stat output!!!! 
// TODO move this outside of RTL wrappers
// TODO should move storage of message size outside?

// TODO this is more of a prefetcher?
/* PayloadCache - Responsible for prefetching packet payload data.
 * Issues DMA read requests, stores associated state while request is
 * outstanding, issues notifcations when requests are complette to the
 * rest of the system.
 */ 
class PayloadCache extends Module {
  val io = IO(new Bundle {
    val fetchSize    = Input(UInt(16.W)) // Number of bytes to fetch per cycle for an RPC
    val newFetchable = Flipped(Decoupled(new PrefetcherState)) // New RPCs to fetch data for
    val newFree      = Flipped(Decoupled(UInt(10.W))) // Opens cache window for refered dbuff ID
    val fetchRequest = Decoupled(new DmaReq) // DMA read requests to fetch data
    val fetchCmpl    = Flipped(Decoupled(new DmaReadStat)) // Completions from DMA core
    val fetchNotif   = Decoupled(new QueueEntry) // Notifies the rest of system of cache updates
  })

  // TODO temporary disabled
  io.newFree.bits := DontCare
  io.newFree.valid := DontCare
  io.newFree.ready := false.B

  /* PIFO style queue for fetch entries
   * TODO will need to expand
   */
  val fetchQueue = Module(new PriorityQueue(new PrefetcherEntry, 32))
  fetchQueue.io.increment := io.fetchSize

  /* Data associated with the entries in the priorty queue
   * TODO will need to expand
   */
  val dbuffState = Mem(32, new PrefetcherState)

  /* Create a new entry for the priority queue
   *   0   - 255 bytes, priority of 0, single fetch
   *   256 - 512 bytes, priority of 1, double fetch
   *   .....
   * TODO make >> 8 more general based on fetchSize
   */
  val newPrefetcherEntry = Wire(new PrefetcherEntry)
  newPrefetcherEntry.dbuffID  := io.newFetchable.bits.dbuffID
  newPrefetcherEntry.priority := (io.newFetchable.bits.totalLen >> 8.U) 
  newPrefetcherEntry.active   := 1.U

  // Tie new prefetcher entry to enqueue of the priority queue
  fetchQueue.io.enqueue.bits  := newPrefetcherEntry
  io.newFetchable.ready       := fetchQueue.io.enqueue.ready
  fetchQueue.io.enqueue.valid := io.newFetchable.valid

  // When a new priority queue entry is created store the metadata in memory
  when (fetchQueue.io.enqueue.fire) {
    dbuffState.write(io.newFetchable.bits.dbuffID, io.newFetchable.bits)
  }

  /* Read requests are stored in a memory while they are pending
   * completion. When a status message is received indicating that the
   * requested data has arrived we can lookup the metadata associated
   * with that message for upating our queue state.
   */
  class ReqMem extends Bundle {
    val state = new PrefetcherState
    val queue = new PrefetcherEntry
  }

  // Store data associated with oustanding requests to DMA 
  val tag     = RegInit(0.U(8.W))
  val tagMem  = Mem(512, new ReqMem)

  // Requests to DMA
  val dmaRead = Wire(new DmaReq)
  val ramAddr = Wire(UInt(32.W)) // Where in ram to write to 
  val msgOff  = Wire(UInt(32.W)) // Absolute address in message

  // Read metadata associated with priority queue entry
  val dbuffRead = dbuffState.read(fetchQueue.io.dequeue.bits.dbuffID)

  // When priority is max, msgAddr = 0
  // When priority is max-1, magAddr = 256

  val maxPriority = Wire(UInt(10.W))
  maxPriority := (dbuffRead.totalLen >> 8.U) 

  msgOff := (maxPriority - fetchQueue.io.dequeue.bits.priority) << 8.U
  ramAddr := (CacheCfg.lineSize.U * fetchQueue.io.dequeue.bits.dbuffID) + (msgOff % 16364.U)

  dmaRead.pcie_addr := msgOff
  dmaRead.ram_sel   := 0.U
  dmaRead.ram_addr  := ramAddr
  dmaRead.len       := io.fetchSize
  dmaRead.tag       := tag
  dmaRead.port      := 1.U // TODO placeholder

  io.fetchRequest.bits  := dmaRead
  io.fetchRequest.valid := fetchQueue.io.dequeue.valid
  fetchQueue.io.dequeue.ready := io.fetchRequest.ready

  val pendTag = tagMem.read(io.fetchCmpl.bits.tag)

  io.fetchNotif.bits.rpc_id     := 0.U
  io.fetchNotif.bits.dbuff_id   := pendTag.queue.dbuffID
  io.fetchNotif.bits.remaining  := 0.U

  val notifQueue = Module(new Queue(new ReqMem, 4, true, false))

  // TODO fetchCmpl does not respond to push back
  io.fetchCmpl.ready := notifQueue.io.enq.ready
  notifQueue.io.enq.valid := io.fetchCmpl.valid
  notifQueue.io.enq.bits := pendTag

  // val notif_offset = Wire(UInt(20.W))
  // TODO this will not work when windowing cache
  // notif_offset := notifQueue.io.deq.bits.queue.priority * io.fetchSize
  // when (notif_offset < io.fetchSize) {
  //   io.fetchNotif.bits.dbuffered := 0.U
  // }.otherwise {
  //   io.fetchNotif.bits.dbuffered := notif_offset - io.fetchSize

  io.fetchNotif.bits.dbuffered := notifQueue.io.deq.bits.queue.priority << 8.U
  io.fetchNotif.bits.granted   := 0.U
  io.fetchNotif.bits.priority  := queue_priority.DBUFF_UPDATE.asUInt
  io.fetchNotif.valid          := notifQueue.io.deq.valid
  notifQueue.io.deq.ready      := io.fetchNotif.ready
 
  // If a read request is lodged then we store it in the memory until completiton
  when(io.fetchRequest.fire) {
    val tagEntry = Wire(new ReqMem)
    tagEntry.state := dbuffRead
    tagEntry.queue := fetchQueue.io.dequeue.bits

    tagMem.write(tag, tagEntry)

    tag := tag + 1.U(8.W)
  }
}
