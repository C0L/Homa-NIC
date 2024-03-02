package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

// TODO not flow control in addr map core!!!!

// TODO logic moving to cache core

/* H2cDMA- Generates tag identifers to store read requests as they
 * are outstanding in the pcie engine.
 */
// class H2cDMA extends Module {
//   val io = IO(new Bundle {
//     val dma_r_req       = Flipped(Decoupled(new DmaReadReq)) // New DMA read requests
//     val dma_read_desc   = Decoupled(new dma_read_desc_t(1)) // Descriptors to pcie to init request
//     val dma_read_status = Flipped(Decoupled(new dma_read_desc_status_t(1))) // Status of read
//     val dbuff_notif_o   = Decoupled(new QueueEntry) // Alert the fetch queue of data arrival
// 
//     val dynamicConfiguration = Input(new DynamicConfiguration)
//   })
// 
//   /* Read requests are stored in a memory while they are pending
//    * completion. When a status message is received indicating that the
//    * requested data has arrived we can lookup the metadata associated
//    * with that message for upating our queue state.
//    */
//   val tag = RegInit(0.U(8.W))
//   val tag_mem = Mem(256, new DmaReadReq)
// 
//   // Add a register stage to this module for timing 
//   val dma_read_queue  = Module(new Queue(new DmaReadReq, 1, true, false))
//   val dma_read_status = Module(new Queue(new dma_read_desc_status_t(1), 1, true, false))
// 
//   // Add data to the queues if there are inputs from status or requests
//   dma_read_status.io.enq <> io.dma_read_status
//   dma_read_queue.io.enq  <> io.dma_r_req
// 
//   // Dequeue data from queues if there is room in the downstream receiver
//   dma_read_status.io.deq.ready := io.dbuff_notif_o.ready
//   dma_read_queue.io.deq.ready  := io.dma_read_desc.ready
//   io.dma_read_desc.valid       := dma_read_queue.io.deq.valid
//   io.dbuff_notif_o.valid       := dma_read_status.io.deq.valid
// 
//   // TODO just embed the read_desc in the dma_read_t?
//   // Create a read descriptor from input request
//   io.dma_read_desc.bits.pcie_addr := dma_read_queue.io.deq.bits.pcie_addr
//   io.dma_read_desc.bits.ram_sel   := 0.U(2.W)
//   io.dma_read_desc.bits.ram_addr  := dma_read_queue.io.deq.bits.ram_addr
//   io.dma_read_desc.bits.len       := dma_read_queue.io.deq.bits.len
//   io.dma_read_desc.bits.tag       := tag
// 
//   // Create a data buffer notification from a status response from pcie
//   val pending_read = tag_mem.read(dma_read_status.io.deq.bits.tag)
// 
//   // TODO 
//   // io.dbuff_notif_o.bits.rpc_id     := 0.U
//   // io.dbuff_notif_o.bits.dbuff_id   := pending_read.cache_id
//   // io.dbuff_notif_o.bits.remaining  := 0.U
//   // val notif_offset = Wire(UInt(20.W))
//   // notif_offset := pending_read.message_size - (pending_read.dest_ram_addr - (16384.U * pending_read.cache_id)) // TODO not great
//   // when (notif_offset < io.dynamicConfiguration.fetchRequestSize) {
//   //   io.dbuff_notif_o.bits.dbuffered  := 0.U
//   // }.otherwise {
//   //   io.dbuff_notif_o.bits.dbuffered  := notif_offset - io.dynamicConfiguration.fetchRequestSize
//   // }
// 
//   io.dbuff_notif_o.bits.granted    := 0.U
//   io.dbuff_notif_o.bits.priority   := queue_priority.DBUFF_UPDATE.asUInt
// 
//   // If a read request is lodged then we store it in the memory until completiton
//   when(dma_read_queue.io.deq.fire) {
//     tag_mem.write(tag, dma_read_queue.io.deq.bits)
// 
//     tag := tag + 1.U(8.W)
//   }
// }

/* C2hDMA- state machine which transforms input write requests into
 * writes to ram and after receipt of completion writes to pcie.
 *
 * WARNING: This makes the assumption that the write request to the
 * BRAM will complete before the pcie core has fetched that same data
 * from the BRAM. I think this is generally a safe assumption.
 */
// class C2hDMA extends Module {
//   val io = IO(new Bundle {
//     val dma_write_req         = Flipped(Decoupled(new DmaWriteReq)) // New DMA write requests
//     val ram_write_data        = Decoupled(new RamWriteReq) // Data to write to BRAM
//     val dma_write_desc        = Decoupled(new dma_write_desc_t(1)) // Request for DMA write from BRAM
//     val dma_write_desc_status = Flipped(Decoupled(new dma_write_desc_status_t(1))) // Response status of DMA write
//   })
// 
//   // Maintain the last used tag ID and address in BRAM circular buffer
//   val tag = RegInit(0.U(8.W))
//   val ram_head = RegInit(0.U(14.W))
// 
//   // Add a register stage to improve timing
//   val ram_write_req_queue = Module(new Queue(new RamWriteReq, 1, true, false)) // TODO just changed this
//   ram_write_req_queue.io.enq <> io.dma_write_req
// 
//   // Dequeue validity determines validity to all receivers
//   io.ram_write_data.valid := ram_write_req_queue.io.deq.valid
//   io.dma_write_desc.valid := ram_write_req_queue.io.deq.valid
// 
//   // All receivers need to be ready for a dequeue transaction
//   ram_write_req_queue.io.deq.ready := io.ram_write_data.ready && io.dma_write_desc.ready
// 
//   // Construct the ram write request at the circular buffer head
//   io.ram_write_data.bits.addr := ram_head
//   io.ram_write_data.bits.len  := ram_write_req_queue.io.deq.bits.len
//   io.ram_write_data.bits.data := ram_write_req_queue.io.deq.bits.data
// 
//   // Construct a DMA write request to read from specified place in BRAM
//   val dma_write = Wire(new dma_write_desc_t(1))
//   // dma_write.pcie_addr    := ram_write_req_queue.io.deq.bits.pcie_addr TODO??
//   dma_write.ram_sel      := 0.U
//   dma_write.ram_addr     := ram_head
//   dma_write.len          := ram_write_req_queue.io.deq.bits.len
//   dma_write.tag          := tag
//   io.dma_write_desc.bits := dma_write
// 
//   // When a dequeue transaction occurs, move to the next tag and ram head
//   when(ram_write_req_queue.io.deq.fire) {
//     tag := tag + 1.U(8.W)
//     ram_head := ram_head + 64.U(14.W)
//   }
// 
//   // TODO unused
//   io.dma_write_desc_status.ready := true.B
//   io.dma_write_desc_status.bits  := DontCare 
//   io.dma_write_desc_status.valid := DontCare
// }

/* AddressMap - performs DMA address translations for read and write
 * requests before they are passed the pcie core.
 */
class AddressMap extends Module {
  val io = IO(new Bundle {
    val dmaMap              = Flipped(Decoupled(new DmaMap))   // New DMA address translations

    val c2hMetadataDmaReqIn = Flipped(Decoupled(new DmaWriteReq)) // Requests to metadata DMA buffer
    val c2hPayloadDmaReqIn  = Flipped(Decoupled(new DmaWriteReq)) // Requests to c2h DMA buffer
    val h2cPayloadDmaReqIn  = Flipped(Decoupled(new DmaReadReq))  // Requests to h2c DMA buffer

    val c2hMetadataDmaReqOut = Decoupled(new DmaWriteReq) // Requests to metadata DMA buffer with DMA addr
    val c2hPayloadDmaReqOut  = Decoupled(new DmaWriteReq) // Requests to c2h DMA buffer with DMA addr
    val h2cPayloadDmaReqOut  = Decoupled(new DmaReadReq)  // Requests to h2c DMA buffer with DMA addr
  })

  // Map port -> pcie address of DMA buffer
  val metadataMaps = Module(new BRAM(16384, 14, new DmaMap, new DmaWriteReq))
  val c2hdataMaps  = Module(new BRAM(16384, 14, new DmaMap, new DmaWriteReq))
  val h2cdataMaps  = Module(new BRAM(16384, 14, new DmaMap, new DmaReadReq))
 
  val metadataMapsAddr = metadataMaps.io.r_data_data.asTypeOf(new DmaMap).pcie_addr
  val c2hdataMapsAddr  = c2hdataMaps.io.r_data_data.asTypeOf(new DmaMap).pcie_addr 
  val h2cdataMapsAddr  = h2cdataMaps.io.r_data_data.asTypeOf(new DmaMap).pcie_addr

  // TODO ugly 
  io.c2hMetadataDmaReqOut.bits           := metadataMaps.io.track_out
  io.c2hMetadataDmaReqOut.bits.pcie_addr := metadataMaps.io.track_out.asTypeOf(new DmaWriteReq).pcie_addr + metadataMapsAddr
  metadataMaps.io.r_data_ready           := io.c2hMetadataDmaReqOut.ready
  io.c2hMetadataDmaReqOut.valid          := metadataMaps.io.r_data_valid

  io.c2hPayloadDmaReqOut.bits           := c2hdataMaps.io.track_out
  io.c2hPayloadDmaReqOut.bits.pcie_addr := c2hdataMaps.io.track_out.asTypeOf(new DmaWriteReq).pcie_addr + c2hdataMapsAddr
  c2hdataMaps.io.r_data_ready           := io.c2hMetadataDmaReqOut.ready
  io.c2hPayloadDmaReqOut.valid          := c2hdataMaps.io.r_data_valid

  io.h2cPayloadDmaReqOut.bits           := h2cdataMaps.io.track_out
  io.h2cPayloadDmaReqOut.bits.pcie_addr := h2cdataMaps.io.track_out.asTypeOf(new DmaReadReq).pcie_addr + h2cdataMapsAddr
  h2cdataMaps.io.r_data_ready           := io.h2cPayloadDmaReqOut.ready
  io.h2cPayloadDmaReqOut.valid          := h2cdataMaps.io.r_data_valid

  // TODO request out ready valid 

  io.c2hMetadataDmaReqIn.ready := metadataMaps.io.r_cmd_ready
  metadataMaps.io.r_cmd_valid  := io.c2hMetadataDmaReqIn.valid
  metadataMaps.io.r_cmd_addr   := io.c2hMetadataDmaReqIn.bits.port
  metadataMaps.io.track_in     := io.c2hMetadataDmaReqIn.bits

  io.c2hPayloadDmaReqIn.ready := c2hdataMaps.io.r_cmd_ready
  c2hdataMaps.io.r_cmd_valid  := io.c2hPayloadDmaReqIn.valid
  c2hdataMaps.io.r_cmd_addr   := io.c2hPayloadDmaReqIn.bits.port
  c2hdataMaps.io.track_in     := io.c2hPayloadDmaReqIn.bits

  io.h2cPayloadDmaReqIn.ready := h2cdataMaps.io.r_cmd_ready
  h2cdataMaps.io.r_cmd_valid  := io.h2cPayloadDmaReqIn.valid
  h2cdataMaps.io.r_cmd_addr   := io.h2cPayloadDmaReqIn.bits.port
  h2cdataMaps.io.track_in     := io.h2cPayloadDmaReqIn.bits

  metadataMaps.io.w_cmd_valid := false.B
  c2hdataMaps.io.w_cmd_valid  := false.B
  h2cdataMaps.io.w_cmd_valid  := false.B

  metadataMaps.io.w_cmd_addr  := io.dmaMap.bits.port
  metadataMaps.io.w_cmd_data  := io.dmaMap.bits

  c2hdataMaps.io.w_cmd_addr  := io.dmaMap.bits.port
  c2hdataMaps.io.w_cmd_data  := io.dmaMap.bits

  h2cdataMaps.io.w_cmd_addr  := io.dmaMap.bits.port
  h2cdataMaps.io.w_cmd_data  := io.dmaMap.bits

  io.dmaMap.ready := true.B

  // TODO this can be a lot lower weight
  switch (dma_map_type.safe(io.dmaMap.bits.map_type)._1) {
    is (dma_map_type.H2C) {
      h2cdataMaps.io.w_cmd_valid := io.dmaMap.valid
    }
    is (dma_map_type.C2H) {
      c2hdataMaps.io.w_cmd_valid := io.dmaMap.valid
    }
    is (dma_map_type.META) {
      metadataMaps.io.w_cmd_valid := io.dmaMap.valid
    }
  }
}
