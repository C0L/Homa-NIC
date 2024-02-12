package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

class h2c_dma extends Module {
  val io = IO(new Bundle {
    val dma_read_req_i     = Flipped(Decoupled(new dma_read_t))
    val pcie_read_cmd_o    = Decoupled(new dma_read_desc_t)
    val pcie_read_status_i = Flipped(Decoupled(new dma_read_desc_status_t))
    val dbuff_notif_o      = Decoupled(new queue_entry_t)
  })

  // Memory of 256 tags
  // TODO maybe SyncReadMem?
  val tag = RegInit(0.U(8.W))
  val tag_mem = Mem(256, new dma_read_t)

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
  val h2c_data_maps = SyncReadMem(16384, new dma_map_t)
  val c2h_data_maps = SyncReadMem(16384, new dma_map_t)
  val metadata_maps = SyncReadMem(16384, new dma_map_t)

  // TODO this should not always be true because we cannot accept w meta and w data simultanously
  io.dma_map_i.ready := true.B
  io.dma_w_meta_i.ready  := true.B
  io.dma_w_data_i.ready  := true.B
  io.dma_r_req_i.ready   := true.B

  io.dma_w_req_o.valid   := false.B
  io.dma_r_req_o.valid   := false.B
  io.dma_r_req_o.valid   := false.B

  io.dma_w_req_o.bits := 0.U.asTypeOf(new dma_write_t)
  io.dma_r_req_o.bits := 0.U.asTypeOf(new dma_read_t)

  // TODO can we turn this into a enqueue function and completition function handler. Like async?
  val dma_w_meta_i_reg_0_bits  = RegNext(io.dma_w_meta_i.bits)
  val dma_w_meta_i_reg_1_bits  = RegNext(dma_w_meta_i_reg_0_bits)
  val dma_w_meta_i_reg_0_valid = RegNext(io.dma_w_meta_i.valid)
  val dma_w_meta_i_reg_1_valid = RegNext(dma_w_meta_i_reg_0_valid)

  val dma_w_data_i_reg_0_bits  = RegNext(io.dma_w_data_i.bits)
  val dma_w_data_i_reg_1_bits  = RegNext(dma_w_data_i_reg_0_bits)
  val dma_w_data_i_reg_0_valid = RegNext(io.dma_w_data_i.valid)
  val dma_w_data_i_reg_1_valid = RegNext(dma_w_data_i_reg_0_valid)

  val dma_r_req_i_reg_0_bits  = RegNext(io.dma_r_req_i.bits)
  val dma_r_req_i_reg_1_bits  = RegNext(dma_r_req_i_reg_0_bits)
  val dma_r_req_i_reg_0_valid = RegNext(io.dma_r_req_i.valid)
  val dma_r_req_i_reg_1_valid = RegNext(dma_r_req_i_reg_0_valid)

  val dma_map_i_reg_0_bits  = RegNext(io.dma_map_i.bits)
  val dma_map_i_reg_1_bits  = RegNext(dma_map_i_reg_0_bits)

  val dma_map_i_reg_0_valid = RegNext(io.dma_map_i.valid)
  val dma_map_i_reg_1_valid = RegNext(dma_map_i_reg_0_valid)

  val meta_maps_read           = metadata_maps.read(dma_w_meta_i_reg_0_bits.port)
  val c2h_data_maps_read       = c2h_data_maps.read(dma_w_data_i_reg_0_bits.port)
  val h2c_data_maps_read       = h2c_data_maps.read(dma_r_req_i_reg_0_bits.port)

  when (dma_w_meta_i_reg_1_valid) {
    io.dma_w_req_o.bits := dma_w_meta_i_reg_1_bits
    io.dma_w_req_o.bits.pcie_write_addr := meta_maps_read.pcie_addr + dma_w_meta_i_reg_1_bits.pcie_write_addr
    io.dma_w_req_o.valid := true.B
  }.elsewhen(dma_w_data_i_reg_1_valid) {
    io.dma_w_req_o.bits := dma_w_data_i_reg_1_bits
    io.dma_w_req_o.bits.pcie_write_addr := c2h_data_maps_read.pcie_addr + dma_w_data_i_reg_1_bits.pcie_write_addr
    io.dma_w_req_o.valid := true.B
  }

  when(dma_r_req_i_reg_1_valid) {
    io.dma_r_req_o.bits := dma_r_req_i_reg_1_bits
    io.dma_r_req_o.bits.pcie_read_addr := h2c_data_maps_read.pcie_addr + dma_r_req_i_reg_1_bits.pcie_read_addr
    io.dma_r_req_o.valid := true.B
  }

  // TODO this needlessly stores the map_type
  when(dma_map_i_reg_0_valid) {
    switch (dma_map_type(dma_map_i_reg_0_bits.map_type)) {
      is (dma_map_type.H2C) {
        h2c_data_maps.write(dma_map_i_reg_0_bits.port, dma_map_i_reg_0_bits)
      }
      is (dma_map_type.C2H) {
        c2h_data_maps.write(dma_map_i_reg_0_bits.port, dma_map_i_reg_0_bits)
      }
      is (dma_map_type.META) {
        metadata_maps.write(dma_map_i_reg_0_bits.port, dma_map_i_reg_0_bits)
      }
    }
  }
}
