package packetproc

import chisel3._
import circt.stage.ChiselStage
// import chisel3.util.Decoupled
import chisel3.util._

/*
 * pcie_read_cmd_t - read request to pcie core with a destination in psdram for the result. This must match the instantiation of the pcie core. 
 */
class pcie_read_cmd_t extends Bundle {
  val pcie_read_addr = UInt(64.W)
  val dest_ram_sel   = UInt(2.W)
  val dest_ram_addr  = UInt(18.W)
  val read_len       = UInt(16.W)
  val tag            = UInt(8.W)
}

class pcie_read_status_t extends Bundle {
  val tag   = UInt(8.W)
  val error = UInt(4.W)
}

class dma_read_t extends Bundle {
  val pcie_read_addr = UInt(64.W) // TODO make this the same across all
  val cache_id       = UInt(10.W)
  val message_size   = UInt(20.W)
  val read_len       = UInt(12.W)
  val dest_ram_addr  = UInt(20.W)
  val port           = UInt(16.W)
}

class dma_write_t extends Bundle {
  val pcie_write_addr = UInt(64.W) // TODO make this the same across all
  val data            = UInt(512.W)
  val length          = UInt(8.W)
  val port            = UInt(16.W)
}

class dbuff_notif_t extends Bundle {
  val rpc_id          = UInt(16.W) // TODO make this the same across all
  val cache_id        = UInt(10.W)
  val remaining_bytes = UInt(20.W)
  val dbuffered_bytes = UInt(20.W)
  val granted_bytes   = UInt(20.W)
  val priority        = UInt(3.W)
}

class h2c_dma extends Module {
  val io = IO(new Bundle {
    val dma_read_req_i     = Flipped(Decoupled(new dma_read_t))
    val pcie_read_cmd_o    = Decoupled(new pcie_read_cmd_t)
    val pcie_read_status_i = Flipped(Decoupled(new pcie_read_status_t))
    val dbuff_notif_o      = Decoupled(new dbuff_notif_t)
  })

  // Memory of 256 tags
  // TODO maybe SyncReadMem?
  val tag = RegInit(0.U(8.W))
  val tag_mem = Mem(256, new dma_read_t)

  // We are ready to accept dma reqs if there is room in the pcie core
  io.dma_read_req_i.ready <> io.pcie_read_cmd_o.ready

  // We are ready to handle status outputs if dbuff queue is ready to handle notifs
  io.pcie_read_status_i.ready <> io.dbuff_notif_o.ready

  // Expect not to send data downstream
  io.pcie_read_cmd_o.valid := false.B
  io.dbuff_notif_o.valid   := false.B

  // TODO should zero init?

  io.pcie_read_cmd_o.bits.pcie_read_addr := io.dma_read_req_i.bits.pcie_read_addr
  io.pcie_read_cmd_o.bits.dest_ram_sel   := 0.U(2.W)
  io.pcie_read_cmd_o.bits.dest_ram_addr  := io.dma_read_req_i.bits.dest_ram_addr
  io.pcie_read_cmd_o.bits.read_len       := io.dma_read_req_i.bits.read_len
  io.pcie_read_cmd_o.bits.tag            := tag

  val pending_read = tag_mem.read(io.pcie_read_status_i.bits.tag)

  io.dbuff_notif_o.bits.rpc_id           := 0.U
  io.dbuff_notif_o.bits.cache_id         := pending_read.cache_id
  io.dbuff_notif_o.bits.remaining_bytes  := pending_read.read_len - pending_read.dest_ram_addr - 256.U(20.W)
  io.dbuff_notif_o.bits.dbuffered_bytes  := 0.U
  io.dbuff_notif_o.bits.granted_bytes    := 0.U
  io.dbuff_notif_o.bits.priority         := 1.U  // TODO update

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


object dma_map_type extends ChiselEnum {
  val h2c_map, c2h_map, meta_map = Value
}

class dma_map_t extends Bundle {
  val pcie_addr = UInt(64.W)
  val port      = UInt(16.W)
  val map_type  = dma_map_type()
}

class addr_map extends Module {
  val io = IO(new Bundle {
    val new_dma_map_i   = Flipped(Decoupled(new dma_map_t))
    val dma_w_meta_i    = Flipped(Decoupled(new dma_write_t))
    val dma_w_data_i    = Flipped(Decoupled(new dma_write_t))
    val dma_r_req_i     = Flipped(Decoupled(new dma_read_t))
    val dma_w_req_o     = Decoupled(new dma_write_t)
    val dma_r_req_o     = Decoupled(new dma_read_t)
  })

  // Map port -> pcie address of DMA buffer
  val h2c_data_maps = SyncReadMem(16384, new dma_map_t)
  val c2h_data_maps = SyncReadMem(16384, new dma_map_t)
  val metadata_maps = SyncReadMem(16384, new dma_map_t)

  // TODO this should not always be true because we cannot accept w meta and w data simultanously
  io.new_dma_map_i.ready := true.B
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

  val new_dma_map_i_reg_0_bits  = RegNext(io.new_dma_map_i.bits)
  val new_dma_map_i_reg_1_bits  = RegNext(new_dma_map_i_reg_0_bits)

  val new_dma_map_i_reg_0_valid = RegNext(io.new_dma_map_i.valid)
  val new_dma_map_i_reg_1_valid = RegNext(new_dma_map_i_reg_0_valid)

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
  when(new_dma_map_i_reg_0_valid) {
    switch (new_dma_map_i_reg_0_bits.map_type) {
      is (dma_map_type.h2c_map) {
        h2c_data_maps.write(new_dma_map_i_reg_0_bits.port, new_dma_map_i_reg_0_bits)
      }
      is (dma_map_type.c2h_map) {
        c2h_data_maps.write(new_dma_map_i_reg_0_bits.port, new_dma_map_i_reg_0_bits)
      }
      is (dma_map_type.meta_map) {
        metadata_maps.write(new_dma_map_i_reg_0_bits.port, new_dma_map_i_reg_0_bits)
      }
    }
  }
}

class dma_ctrl extends Module {
  // TODO this just connects addr_map to c2h_dma and h2c_dma
  val io = IO(new Bundle {
    val new_dma_map_i   = Flipped(Decoupled(new dma_map_t))
    val dma_w_meta_i    = Flipped(Decoupled(new dma_write_t))
    val dma_w_data_i    = Flipped(Decoupled(new dma_write_t))
    val dma_r_req_i     = Flipped(Decoupled(new dma_read_t))
    val dma_w_req_o     = Decoupled(new dma_write_t)
    val dma_r_req_o     = Decoupled(new dma_read_t)

    // Also need the ports for interacting with DMA
  })

  val addr_map = Module(new addr_map())
  addr_map.dma_w_meta_i.

  // TODO connect these two together

  val h2c_dma = Module(new h2c_dma())

  // Tie the outputs of this to dma_r_req_o

}

object Main extends App {
  // These lines generate the Verilog output
  ChiselStage.emitSystemVerilogFile(new h2c_dma,
    firtoolOpts = Array("-disable-all-randomization", "-strip-debug-info")
  )

  ChiselStage.emitSystemVerilogFile(new addr_map,
    firtoolOpts = Array("-disable-all-randomization", "-strip-debug-info")
  )
}
