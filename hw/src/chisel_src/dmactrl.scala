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

  // TODO this is unregistered

  // We are ready to accept dma reqs if there is room in the pcie core
  io.dma_read_req_i.ready := io.pcie_read_cmd_o.ready

  // We are ready to handle status outputs if dbuff queue is ready to handle notifs
  io.pcie_read_status_i.ready := io.dbuff_notif_o.ready

  // Expect not to send data downstream
  io.pcie_read_cmd_o.valid := false.B
  io.dbuff_notif_o.valid   := false.B

  // TODO should zero init?

  io.pcie_read_cmd_o.bits.pcie_addr := io.dma_read_req_i.bits.pcie_read_addr
  io.pcie_read_cmd_o.bits.ram_sel        := 0.U(2.W)
  io.pcie_read_cmd_o.bits.ram_addr       := io.dma_read_req_i.bits.dest_ram_addr
  io.pcie_read_cmd_o.bits.len            := io.dma_read_req_i.bits.read_len
  io.pcie_read_cmd_o.bits.tag            := tag

  val pending_read = tag_mem.read(io.pcie_read_status_i.bits.tag)

  io.dbuff_notif_o.bits.rpc_id     := 0.U
  io.dbuff_notif_o.bits.dbuff_id   := pending_read.cache_id
  io.dbuff_notif_o.bits.remaining  := pending_read.read_len - pending_read.dest_ram_addr - 256.U(20.W)
  io.dbuff_notif_o.bits.dbuffered  := 0.U
  io.dbuff_notif_o.bits.granted    := 0.U
  io.dbuff_notif_o.bits.priority   := 1.U  // TODO update

  // Did a dma request just arrive
  when(io.dma_read_req_i.valid) {
    io.pcie_read_cmd_o.valid := true.B
    
    tag_mem.write(tag, io.dma_read_req_i.bits)

    tag := tag + 1.U(8.W)
  }

  // Did a status completion from dma write just return
  when (io.pcie_read_status_i.valid) {
    io.dbuff_notif_o.valid := true.B
  }
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


  io.dma_w_meta_i.ready      := meta_data_maps.io.r_cmd_ready
  meta_data_maps.io.r_cmd_valid := io.dma_w_meta_i.valid
  meta_data_maps.io.r_cmd_addr  := io.dma_w_meta_i.bits.port
  meta_data_maps.io.track_in    := io.dma_w_meta_i.bits

  io.dma_w_data_i.ready      := c2h_data_maps.io.r_cmd_ready
  c2h_data_maps.io.r_cmd_valid  := io.dma_w_data_i.valid
  c2h_data_maps.io.r_cmd_addr   := io.dma_w_data_i.bits.port
  c2h_data_maps.io.track_in     := io.dma_w_data_i.bits

  io.dma_r_req_i.ready          := h2c_data_maps.io.r_cmd_ready
  h2c_data_maps.io.r_cmd_valid  := io.dma_r_req_i.valid
  h2c_data_maps.io.r_cmd_addr   := io.dma_r_req_i.bits.port
  h2c_data_maps.io.track_in     := io.dma_r_req_i.bits

  // TODO this can be better -- some mux??
  meta_data_maps.io.w_cmd_valid := false.B
  meta_data_maps.io.w_cmd_addr  := 0.U
  meta_data_maps.io.w_cmd_data  := 0.U.asTypeOf(new dma_map_t)

  c2h_data_maps.io.w_cmd_valid := false.B
  c2h_data_maps.io.w_cmd_addr  := 0.U
  c2h_data_maps.io.w_cmd_data  := 0.U.asTypeOf(new dma_map_t)

  h2c_data_maps.io.w_cmd_valid := false.B
  h2c_data_maps.io.w_cmd_addr  := 0.U
  h2c_data_maps.io.w_cmd_data  := 0.U.asTypeOf(new dma_map_t)

  io.dma_map_i.ready := h2c_data_maps.io.w_cmd_ready && c2h_data_maps.io.w_cmd_ready && meta_data_maps.io.w_cmd_ready

  switch (dma_map_type.safe(io.dma_map_i.bits.map_type)._1) {
    is (dma_map_type.H2C) {
      h2c_data_maps.io.w_cmd_valid := io.dma_map_i.valid
      h2c_data_maps.io.w_cmd_addr  := io.dma_map_i.bits.port
      h2c_data_maps.io.w_cmd_data  := io.dma_map_i.bits
    }
    is (dma_map_type.C2H) {
      c2h_data_maps.io.w_cmd_valid := io.dma_map_i.valid
      c2h_data_maps.io.w_cmd_addr  := io.dma_map_i.bits.port
      c2h_data_maps.io.w_cmd_data  := io.dma_map_i.bits
    }
    is (dma_map_type.META) {
      meta_data_maps.io.w_cmd_valid := io.dma_map_i.valid
      meta_data_maps.io.w_cmd_addr  := io.dma_map_i.bits.port
      meta_data_maps.io.w_cmd_data  := io.dma_map_i.bits
    }
  }
}
