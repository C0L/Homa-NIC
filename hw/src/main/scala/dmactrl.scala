package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

/* h2c_dma - Generates tag identifers to store read requests as they
 * are outstanding in the pcie engine.
 */
class h2c_dma extends Module {
  val io = IO(new Bundle {
    val dma_r_req       = Flipped(Decoupled(new dma_read_t)) // New DMA read requests
    val dma_read_desc   = Decoupled(new dma_read_desc_t) // Descriptors to pcie to init request
    val dma_read_status = Flipped(Decoupled(new dma_read_desc_status_t)) // Status of read
    val dbuff_notif_o   = Decoupled(new QueueEntry) // Alert the fetch queue of data arrival
  })

  /* Read requests are stored in a memory while they are pending
   * completion. When a status message is received indicating that the
   * requested data has arrived we can lookup the metadata associated
   * with that message for upating our queue state.
   */
  val tag = RegInit(0.U(8.W))
  val tag_mem = Mem(256, new dma_read_t)

  // Add a register stage to this module for timing 
  val dma_read_queue  = Module(new Queue(new dma_read_t, 1, true, false))
  val dma_read_status = Module(new Queue(new dma_read_desc_status_t, 1, true, false))

  // Add data to the queues if there are inputs from status or requests
  dma_read_status.io.enq <> io.dma_read_status
  dma_read_queue.io.enq  <> io.dma_r_req

  // Dequeue data from queues if there is room in the downstream receiver
  dma_read_status.io.deq.ready := io.dbuff_notif_o.ready
  dma_read_queue.io.deq.ready  := io.dma_read_desc.ready
  io.dma_read_desc.valid       := dma_read_queue.io.deq.valid
  io.dbuff_notif_o.valid       := dma_read_status.io.deq.valid

  // TODO just embed the read_desc in the dma_read_t?
  // Create a read descriptor from input request
  io.dma_read_desc.bits.pcie_addr := dma_read_queue.io.deq.bits.pcie_read_addr
  io.dma_read_desc.bits.ram_sel   := 0.U(2.W)
  io.dma_read_desc.bits.ram_addr  := dma_read_queue.io.deq.bits.dest_ram_addr
  io.dma_read_desc.bits.len       := dma_read_queue.io.deq.bits.read_len
  io.dma_read_desc.bits.tag       := tag

  // Create a data buffer notification from a status response from pcie
  val pending_read = tag_mem.read(dma_read_status.io.deq.bits.tag)

  io.dbuff_notif_o.bits.rpc_id     := 0.U
  io.dbuff_notif_o.bits.dbuff_id   := pending_read.cache_id
  io.dbuff_notif_o.bits.remaining  := 0.U
  val notif_offset = Wire(UInt(20.W))
  notif_offset := pending_read.message_size - pending_read.dest_ram_addr 
  when (notif_offset < 256.U) {
    io.dbuff_notif_o.bits.dbuffered  := 0.U
  }.otherwise {
    io.dbuff_notif_o.bits.dbuffered  := notif_offset - 256.U
  }

  io.dbuff_notif_o.bits.granted    := 0.U
  io.dbuff_notif_o.bits.priority   := queue_priority.DBUFF_UPDATE.asUInt

  // If a read request is lodged then we store it in the memory until completiton
  when(dma_read_queue.io.deq.fire) {
    tag_mem.write(tag, dma_read_queue.io.deq.bits)

    tag := tag + 1.U(8.W)
  }
}

/* c2h_dma - state machine which transforms input write requests into
 * writes to ram and after receipt of completion writes to pcie.
 *
 * WARNING: This makes the assumption that the write request to the
 * BRAM will complete before the pcie core has fetched that same data
 * from the BRAM. I think this is generally a safe assumption.
 */
class c2h_dma extends Module {
  val io = IO(new Bundle {
    val dma_write_req         = Flipped(Decoupled(new dma_write_t)) // New DMA write requests
    val ram_write_desc        = Decoupled(new ram_write_desc_t) // Request to write data to BRAM
    val ram_write_data        = Decoupled(new ram_write_data_t) // Data to write to BRAM
    val ram_write_desc_status = Flipped(Decoupled(new ram_write_desc_status_t)) // Status from BRAM write
    val dma_write_desc        = Decoupled(new dma_write_desc_t) // Request for DMA write from BRAM
    val dma_write_desc_status = Flipped(Decoupled(new dma_write_desc_status_t)) // Response status of DMA write
  })

  // Maintain the last used tag ID and address in BRAM circular buffer
  val tag = RegInit(0.U(8.W))
  val ram_head = RegInit(0.U(14.W))

  // Add a register stage to improve timing
  val ram_write_req_queue = Module(new Queue(new dma_write_t, 1, true, true))
  ram_write_req_queue.io.enq <> io.dma_write_req

  // Dequeue validity determines validity to all receivers
  io.ram_write_desc.valid := ram_write_req_queue.io.deq.valid
  io.ram_write_data.valid := ram_write_req_queue.io.deq.valid
  io.dma_write_desc.valid := ram_write_req_queue.io.deq.valid

  // All receivers need to be ready for a dequeue transaction
  ram_write_req_queue.io.deq.ready := io.ram_write_desc.ready && io.ram_write_data.ready && io.dma_write_desc.ready

  // Construct a write to BRAM at the circular buffer head
  io.ram_write_desc.bits.ram_addr  := ram_head
  io.ram_write_desc.bits.len       := ram_write_req_queue.io.deq.bits.length
  io.ram_write_desc.bits.tag       := tag

  // Format data to write to BRAM
  io.ram_write_data.bits      := 0.U.asTypeOf(new ram_write_data_t)
  io.ram_write_data.bits.data := ram_write_req_queue.io.deq.bits.data
  io.ram_write_data.bits.last := 1.U
  io.ram_write_data.bits.keep := ("hFFFFFFFFFFFFFFFF".U >> (64.U - ram_write_req_queue.io.deq.bits.length))

  // Construct a DMA write request to read from specified place in BRAM
  val dma_write = Wire(new dma_write_desc_t)
  dma_write.pcie_addr := ram_write_req_queue.io.deq.bits.pcie_write_addr
  dma_write.ram_sel   := 0.U
  dma_write.ram_addr  := ram_head
  dma_write.len       := ram_write_req_queue.io.deq.bits.length 
  dma_write.tag       := tag
  io.dma_write_desc.bits := dma_write

  // When a dequeue transaction occurs, move to the next tag and ram head
  when(ram_write_req_queue.io.deq.fire) {
    tag := tag + 1.U(8.W)
    ram_head := ram_head + 64.U(14.W)
  }

  // TODO unused
  io.ram_write_desc_status.ready := true.B
  io.ram_write_desc_status.bits  := DontCare 
  io.ram_write_desc_status.valid := DontCare

  io.dma_write_desc_status.ready := true.B
  io.dma_write_desc_status.bits  := DontCare 
  io.dma_write_desc_status.valid := DontCare
}


// This can probably be more fixed function
// class addr_store[T <: Data](size: Int, aw: Int, gen: T, carry: T) extends Module {
//   val io = IO (new Bundle {
//     val w_cmd_addr   = Input(UInt(aw.W))
//     val w_cmd_data   = Input(gen)
//     val w_cmd_valid  = Input(Bool())
//     val w_cmd_ready  = Output(Bool())
// 
//     val r_cmd_ready  = Output(Bool())
//     val r_cmd_valid  = Input(Bool())
//     val r_cmd_addr   = Input(UInt(aw.W))
//     val r_data_ready = Input(Bool())
//     val r_data_valid = Output(Bool())
//     val r_data_data  = Output(gen)
// 
//     val track_in     = Input(carry)
//     val track_out    = Output(carry)
//   })
// 
//   io.w_cmd_ready := true.B
// 
//   val memory = SyncReadMem(size, gen)
// 
//   // We are holding data if a transaction took place
//   val r_cmd_output_hold  = RegInit(false.B)
// 
//   // We cannot accept more data if we are holding and it cannot progress
//   val r_cmd_output_ready = !(r_cmd_output_hold && !io.r_data_ready)
// 
//   // We are holding data if a transaction took place
//   val r_cmd_input_hold = RegInit(false.B)
// 
//   // We cannot accept more data if we are holding and it cannot progress
//   val r_cmd_input_ready = (r_cmd_input_hold && r_cmd_output_ready) || !r_cmd_input_hold
// 
//     // Grab the addr if a transaction occurs
//   val r_cmd_input_addr  = RegEnable(io.r_cmd_addr, r_cmd_input_ready && io.r_cmd_valid)
// 
//   io.r_data_valid   := r_cmd_output_hold
//   io.r_data_data    := memory.read(r_cmd_input_addr)
// 
//   r_cmd_input_hold  := r_cmd_input_ready & io.r_cmd_valid
//   r_cmd_output_hold := r_cmd_output_ready && r_cmd_input_hold
//   io.r_cmd_ready    := r_cmd_input_ready
// 
//   val w_cmd_addr_reg = RegNext(io.w_cmd_addr)
//   val w_cmd_data_reg = RegNext(io.w_cmd_data)
// 
//   when (RegNext(io.w_cmd_valid)) {
//     memory.write(w_cmd_addr_reg, w_cmd_data_reg)
//   }
// 
//   // Grab side data if transaction occurs
//   val r_cmd_input_track = RegEnable(io.track_in, r_cmd_input_ready && io.r_cmd_valid)
// 
//   io.track_out := RegEnable(r_cmd_input_track, r_cmd_output_ready && r_cmd_input_hold)
// }


/* addr_map - performs DMA address translations for read and write
 * requests before they are passed the pcie core.
 */
class addr_map extends Module {
  val io = IO(new Bundle {
    val dma_map_i    = Flipped(Decoupled(new dma_map_t))   // New DMA address translations
    val dma_w_meta_i = Flipped(Decoupled(new dma_write_t)) // Requests to metadata DMA buffer
    val dma_w_data_i = Flipped(Decoupled(new dma_write_t)) // Requests to c2h DMA buffer
    val dma_r_req_i  = Flipped(Decoupled(new dma_read_t))  // Requests to h2c DMA buffer
    val dma_w_req_o  = Decoupled(new dma_write_t)          // Outgoing write request 
    val dma_r_req_o  = Decoupled(new dma_read_t)           // Outgoing read request 
  })

  // Map port -> pcie address of DMA buffer
  val h2c_data_maps  = Module(new BRAM(16384, 14, new dma_map_t, new dma_read_t))
  val c2h_data_maps  = Module(new BRAM(16384, 14, new dma_map_t, new dma_write_t))
  val meta_data_maps = Module(new BRAM(16384, 14, new dma_map_t, new dma_write_t))

  val dma_w_meta_aug = Wire(new dma_write_t)
  val dma_r_req_aug  = Wire(new dma_read_t)
  val dma_w_data_aug  = Wire(new dma_write_t)

  dma_w_meta_aug := meta_data_maps.io.track_out
  dma_r_req_aug  := h2c_data_maps.io.track_out
  dma_w_data_aug := c2h_data_maps.io.track_out

  // TODO this is a little sloppy
  dma_w_meta_aug.pcie_write_addr := meta_data_maps.io.r_data_data.asTypeOf(new dma_map_t).pcie_addr + meta_data_maps.io.track_out.asTypeOf(new dma_write_t).pcie_write_addr
  dma_r_req_aug.pcie_read_addr   := h2c_data_maps.io.r_data_data.asTypeOf(new dma_map_t).pcie_addr  + h2c_data_maps.io.track_out.asTypeOf(new dma_read_t).pcie_read_addr
  dma_w_data_aug.pcie_write_addr := c2h_data_maps.io.r_data_data.asTypeOf(new dma_map_t).pcie_addr  + c2h_data_maps.io.track_out.asTypeOf(new dma_write_t).pcie_write_addr

  // There is a 2:1 ratio of write requests that we collapse into a single stream
  val write_arbiter = Module(new Arbiter(new dma_write_t, 2))
  write_arbiter.io.in(0).valid   := meta_data_maps.io.r_data_valid
  meta_data_maps.io.r_data_ready := write_arbiter.io.in(0).ready
  write_arbiter.io.in(0).bits    := dma_w_meta_aug

  write_arbiter.io.in(1).valid   := c2h_data_maps.io.r_data_valid
  c2h_data_maps.io.r_data_ready  := write_arbiter.io.in(1).ready
  write_arbiter.io.in(1).bits    := dma_w_data_aug

  write_arbiter.io.out.ready     := io.dma_w_req_o.ready
  io.dma_w_req_o.valid           := write_arbiter.io.out.valid
  io.dma_w_req_o.bits            := write_arbiter.io.out.bits

  io.dma_r_req_o.valid          := h2c_data_maps.io.r_data_valid
  h2c_data_maps.io.r_data_ready := io.dma_r_req_o.ready
  io.dma_r_req_o.bits           := dma_r_req_aug

  // TODO create some wrapper for requests of this type?
  io.dma_w_meta_i.ready         := meta_data_maps.io.r_cmd_ready
  meta_data_maps.io.r_cmd_valid := io.dma_w_meta_i.valid
  meta_data_maps.io.r_cmd_addr  := io.dma_w_meta_i.bits.port
  meta_data_maps.io.track_in    := io.dma_w_meta_i.bits

  io.dma_w_data_i.ready        := c2h_data_maps.io.r_cmd_ready
  c2h_data_maps.io.r_cmd_valid := io.dma_w_data_i.valid
  c2h_data_maps.io.r_cmd_addr  := io.dma_w_data_i.bits.port
  c2h_data_maps.io.track_in    := io.dma_w_data_i.bits

  io.dma_r_req_i.ready          := h2c_data_maps.io.r_cmd_ready
  h2c_data_maps.io.r_cmd_valid  := io.dma_r_req_i.valid
  h2c_data_maps.io.r_cmd_addr   := io.dma_r_req_i.bits.port
  h2c_data_maps.io.track_in     := io.dma_r_req_i.bits

  // TODO this can be better -- some mux??
  meta_data_maps.io.w_cmd_valid := false.B
  c2h_data_maps.io.w_cmd_valid  := false.B
  h2c_data_maps.io.w_cmd_valid  := false.B

  meta_data_maps.io.w_cmd_addr  := io.dma_map_i.bits.port
  meta_data_maps.io.w_cmd_data  := io.dma_map_i.bits

  c2h_data_maps.io.w_cmd_addr  := io.dma_map_i.bits.port
  c2h_data_maps.io.w_cmd_data  := io.dma_map_i.bits

  h2c_data_maps.io.w_cmd_addr  := io.dma_map_i.bits.port
  h2c_data_maps.io.w_cmd_data  := io.dma_map_i.bits

  io.dma_map_i.ready := true.B

   switch (dma_map_type.safe(io.dma_map_i.bits.map_type)._1) {
     is (dma_map_type.H2C) {
       h2c_data_maps.io.w_cmd_valid := io.dma_map_i.valid
     }
     is (dma_map_type.C2H) {
       c2h_data_maps.io.w_cmd_valid := io.dma_map_i.valid
     }
     is (dma_map_type.META) {
       meta_data_maps.io.w_cmd_valid := io.dma_map_i.valid
     }
   }
}
