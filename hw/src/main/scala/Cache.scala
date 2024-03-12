package gpnic

import chisel3._
import chisel3.util._

// TODO should have flow control on data fetch priority queue output
// TODO no flow control on DMA stat output!!!! 
// TODO move this outside of RTL wrappers
// TODO should move storage of message size outside?

/* PayloadCache - Responsible for prefetching packet payload data.
 * Issues DMA read requests, stores associated state while request is
 * outstanding, issues notifcations when requests are complette to the
 * rest of the system.
 */ 
class PayloadCache extends Module {
  val io = IO(new Bundle {
    val fetchSize    = Input(UInt(16.W)) // Number of bytes to fetch per cycle for an RPC
    val newFetchable = Flipped(Decoupled(new QueueEntry)) // New RPCs to fetch data for
    val newFree      = Flipped(Decoupled(UInt(10.W))) // Opens cache window for refered dbuff ID
    val fetchRequest = Decoupled(new DmaReq) // DMA read requests to fetch data
    val fetchCmpl    = Flipped(Decoupled(new DmaReadStat)) // Completions from DMA core
    val fetchNotif   = Decoupled(new QueueEntry) // Notifies the rest of system of cache updates
  })

  // Systolic array queue
  val fetchQueueRaw = Module(new srpt_queue("fetch"))

  val freeQueue = Module(new Queue(UInt(10.W), 20, true, false))
  freeQueue.io.enq <> io.newFree

  fetchQueueRaw.io.clk := clock
  fetchQueueRaw.io.rst := reset.asUInt

  // Feed the number of bytes per request to the systolic array
  fetchQueueRaw.io.CACHE_BLOCK_SIZE := io.fetchSize

  /* Read requests are stored in a memory while they are pending
   * completion. When a status message is received indicating that the
   * requested data has arrived we can lookup the metadata associated
   * with that message for upating our queue state.
   */
  class ReqMem extends Bundle {
    val dma   = new DmaReq
    val queue = new QueueEntry
  }

  val tag     = RegInit(0.U(8.W))
  val tagMem  = Mem(256, new ReqMem)

  val newFreeEntry = Wire(new QueueEntry)
  newFreeEntry := 0.U.asTypeOf(new QueueEntry)
  newFreeEntry.dbuff_id := io.newFree.bits
  newFreeEntry.priority := 1.U // TODO enum

  val arbiter = Module(new RRArbiter(new QueueEntry, 2))
  arbiter.io.in(0) <> io.newFetchable
  arbiter.io.in(1).bits  := newFreeEntry
  arbiter.io.in(1).valid := freeQueue.io.deq.valid // io.newFree.valid
  freeQueue.io.deq.ready := arbiter.io.in(1).ready

  fetchQueueRaw.io.s_axis.tdata  := arbiter.io.out.bits.asTypeOf(UInt(89.W))
  arbiter.io.out.ready := fetchQueueRaw.io.s_axis.tready
  fetchQueueRaw.io.s_axis.tvalid := arbiter.io.out.valid

  // Cast the bitvector output if the fetch queue to a Chisel native type
  val fetchQueueRawOut = Wire(new QueueEntry)
  fetchQueueRawOut := fetchQueueRaw.io.m_axis.tdata.asTypeOf(new QueueEntry)

  val dmaRead = Wire(new DmaReq)

  dmaRead.pcie_addr := fetchQueueRawOut.dbuffered
  dmaRead.ram_sel   := 0.U
  dmaRead.ram_addr  := (CacheCfg.lineSize.U * fetchQueueRawOut.dbuff_id) + fetchQueueRawOut.dbuffered
  dmaRead.len       := io.fetchSize
  dmaRead.tag       := tag
  dmaRead.port      := 1.U // TODO placeholder

  io.fetchRequest.bits := dmaRead

  fetchQueueRaw.io.m_axis.tready := io.fetchRequest.ready
  io.fetchRequest.valid          := fetchQueueRaw.io.m_axis.tvalid

  io.fetchCmpl.ready  := io.fetchNotif.ready
  io.fetchNotif.valid := io.fetchCmpl.valid

  val pendTag = tagMem.read(io.fetchCmpl.bits.tag)

  io.fetchNotif.bits.rpc_id     := 0.U
  io.fetchNotif.bits.dbuff_id   := pendTag.queue.dbuff_id
  io.fetchNotif.bits.remaining  := 0.U

  val notif_offset = Wire(UInt(20.W))
  // TODO granted is total number of bytes in fetch queue, bad naming
  notif_offset := pendTag.queue.granted - (pendTag.dma.ram_addr - (CacheCfg.lineSize.U * pendTag.queue.dbuff_id)) 
  when (notif_offset < io.fetchSize) {
    io.fetchNotif.bits.dbuffered := 0.U
  }.otherwise {
    io.fetchNotif.bits.dbuffered := notif_offset - io.fetchSize
  }
 
  io.fetchNotif.bits.granted  := 0.U
  io.fetchNotif.bits.priority := queue_priority.DBUFF_UPDATE.asUInt
 
  // If a read request is lodged then we store it in the memory until completiton
  when(io.fetchRequest.fire) {
    val tagEntry = Wire(new ReqMem)
    tagEntry.dma   := io.fetchRequest.bits
    tagEntry.queue := fetchQueueRawOut

    tagMem.write(tag, tagEntry)

    tag := tag + 1.U(8.W)
  }
}
