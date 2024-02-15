package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

/* h2c_dma - Generates tag identifers to store read requests as they
 * are outstanding in the pcie engine.
 */
class h2c_dma extends Module {
  val io = IO(new Bundle {
    val dma_read_req_i     = Flipped(Decoupled(new dma_read_t))
    val pcie_read_cmd_o    = Decoupled(new dma_read_desc_t)
    val pcie_read_status_i = Flipped(Decoupled(new dma_read_desc_status_t))
    val dbuff_notif_o      = Decoupled(new queue_entry_t)
  })

  // Memory of 256 tags
  val tag = RegInit(0.U(8.W))
  val tag_mem = Mem(256, new dma_read_t)

  val dma_read_queue  = Module(new Queue(new dma_read_t, 1, true, true))
  val dma_read_status = Module(new Queue(new dma_read_desc_status_t, 1, true, true))

  dma_read_status.io.enq <> io.pcie_read_status_i
  dma_read_queue.io.enq  <> io.dma_read_req_i

  dma_read_queue.io.deq.ready  := io.pcie_read_cmd_o.ready
  dma_read_status.io.deq.ready := io.dbuff_notif_o.ready
  io.pcie_read_cmd_o.valid     := dma_read_queue.io.deq.valid
  io.dbuff_notif_o.valid       := dma_read_status.io.deq.valid

  io.pcie_read_cmd_o.bits.pcie_addr := dma_read_queue.io.deq.bits.pcie_read_addr
  io.pcie_read_cmd_o.bits.ram_sel   := 0.U(2.W)
  io.pcie_read_cmd_o.bits.ram_addr  := dma_read_queue.io.deq.bits.dest_ram_addr
  io.pcie_read_cmd_o.bits.len       := dma_read_queue.io.deq.bits.read_len
  io.pcie_read_cmd_o.bits.tag       := tag

  val pending_read = tag_mem.read(dma_read_status.io.deq.bits.tag)

  io.dbuff_notif_o.bits.rpc_id     := 0.U
  io.dbuff_notif_o.bits.dbuff_id   := pending_read.cache_id
  io.dbuff_notif_o.bits.remaining  := pending_read.read_len - pending_read.dest_ram_addr - 256.U(20.W)
  io.dbuff_notif_o.bits.dbuffered  := 0.U
  io.dbuff_notif_o.bits.granted    := 0.U
  io.dbuff_notif_o.bits.priority   := queue_priority.DBUFF_UPDATE.asUInt

  // Did a dma request just arrive
  when(dma_read_queue.io.deq.fire) {
    tag_mem.write(tag, dma_read_queue.io.deq.bits)

    tag := tag + 1.U(8.W)
  }
}

/* c2h_dma - state machine which transforms input write requests into
 * writes to ram and after receipt of completion writes to pcie.
 */
class c2h_dma extends Module {
  val io = IO(new Bundle {
    val dma_write_req         = Flipped(Decoupled(new dma_write_t))

    val ram_write_desc        = Decoupled(new ram_write_desc_t)
    val ram_write_data        = Decoupled(new ram_write_data_t)
    val ram_write_desc_status = Flipped(Decoupled(new ram_write_desc_status_t))

    val dma_write_desc        = Decoupled(new dma_write_desc_t)
    val dma_write_desc_status = Flipped(Decoupled(new dma_write_desc_status_t))
  })

  // Memory of 256 tags
  val tag = RegInit(0.U(8.W))
  val tag_mem = Mem(256, new dma_write_desc_t)

  val ram_head = RegInit(0.U(14.W))

  val ram_write_req_queue = Module(new Queue(new dma_write_t, 1, true, true))
  val dma_write_req_queue = Module(new Queue(new ram_write_desc_status_t, 1, true, true))

  ram_write_req_queue.io.enq <> io.dma_write_req
  dma_write_req_queue.io.enq <> io.ram_write_desc_status

  io.ram_write_desc.valid := ram_write_req_queue.io.deq.valid
  io.ram_write_data.valid := ram_write_req_queue.io.deq.valid
  io.dma_write_desc.valid := dma_write_req_queue.io.deq.valid
  ram_write_req_queue.io.deq.ready := io.ram_write_desc.ready && io.ram_write_data.ready
  dma_write_req_queue.io.deq.ready := io.dma_write_desc.ready

  io.ram_write_desc.bits.ram_addr  := ram_head
  io.ram_write_desc.bits.len       := ram_write_req_queue.io.deq.bits.length
  io.ram_write_desc.bits.tag       := tag

  io.ram_write_data.bits      := 0.U.asTypeOf(new ram_write_data_t)
  io.ram_write_data.bits.data := ram_write_req_queue.io.deq.bits.data
  io.ram_write_data.bits.last := 1.U
  io.ram_write_data.bits.keep := ("hFFFFFFFFFFFFFFFF".U >> (64.U - ram_write_req_queue.io.deq.bits.length))

  val dma_write = Wire(new dma_write_desc_t)
  dma_write.pcie_addr := ram_write_req_queue.io.deq.bits.pcie_write_addr
  dma_write.ram_sel   := 0.U
  dma_write.ram_addr  := ram_head
  dma_write.len       := ram_write_req_queue.io.deq.bits.pcie_write_addr
  dma_write.tag       := tag

  when(ram_write_req_queue.io.deq.fire) {

    tag_mem.write(tag, dma_write)

    tag      := tag + 1.U(8.W)
    ram_head := ram_head + 64.U(14.W)
  }

  io.dma_write_desc.bits  := tag_mem.read(dma_write_req_queue.io.deq.bits.tag)

  // TODO unused
  io.dma_write_desc_status.ready := true.B
  io.dma_write_desc_status.bits  := DontCare 
  io.dma_write_desc_status.valid := DontCare
}

/* addr_map - performs DMA address translations for read and write
 * requests before they are passed the pcie core.
 */
class addr_map extends Module {
  val io = IO(new Bundle {
    val dma_map_i    = Flipped(Decoupled(new dma_map_t))
    val dma_w_meta_i = Flipped(Decoupled(new dma_write_t))
    val dma_w_data_i = Flipped(Decoupled(new dma_write_t))
    val dma_r_req_i  = Flipped(Decoupled(new dma_read_t))
    val dma_w_req_o  = Decoupled(new dma_write_t)
    val dma_r_req_o  = Decoupled(new dma_read_t)
  })

  // Map port -> pcie address of DMA buffer
  val h2c_data_maps  = Module(new BRAM(16384, 14, new dma_map_t, new dma_read_t))
  val c2h_data_maps  = Module(new BRAM(16384, 14, new dma_map_t, new dma_write_t))
  val meta_data_maps = Module(new BRAM(16384, 14, new dma_map_t, new dma_write_t))

  // TODO need to keep track of original requests

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

  val write_arbiter = Module(new Arbiter(new dma_write_t, 2))
  write_arbiter.io.in(0).valid      := meta_data_maps.io.r_data_valid
  meta_data_maps.io.r_data_ready    := write_arbiter.io.in(0).ready
  write_arbiter.io.in(0).bits       := dma_w_meta_aug

  write_arbiter.io.in(1).valid := c2h_data_maps.io.r_data_valid
  c2h_data_maps.io.r_data_ready := write_arbiter.io.in(1).ready
  write_arbiter.io.in(1).bits := dma_w_data_aug

  write_arbiter.io.out.ready := io.dma_w_req_o.ready
  io.dma_w_req_o.valid := write_arbiter.io.out.valid
  io.dma_w_req_o.bits        := write_arbiter.io.out.bits

  io.dma_r_req_o.valid          := h2c_data_maps.io.r_data_valid
  h2c_data_maps.io.r_data_ready := io.dma_r_req_o.ready
  io.dma_r_req_o.bits           := dma_r_req_aug

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
