package dma

import chisel3._
import circt.stage.ChiselStage
import chisel3.util.Decoupled

/*
 * pcie_read_cmd_t - read request to pcie core with a destination in psdram for the result. This must match the instantiation of the pcie core. 
 */
class pcie_read_cmd_t extends Bundle {
  val pcie_read_addr = UInt(64.W)
  val dest_ram_sel   = UInt(2.W)
  val dest_ram_addr  = UInt(18.W)
  val read_length    = UInt(16.W)
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
  val read_length    = UInt(12.W)
  val dest_ram_addr  = UInt(20.W)
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
  io.pcie_read_cmd_o.bits.read_length    := io.dma_read_req_i.bits.read_length
  io.pcie_read_cmd_o.bits.tag            := tag

  val pending_read = tag_mem.read(io.pcie_read_status_i.bits.tag)

  io.dbuff_notif_o.bits.rpc_id           := 0.U
  io.dbuff_notif_o.bits.cache_id         := pending_read.cache_id
  io.dbuff_notif_o.bits.remaining_bytes  := pending_read.read_length - pending_read.dest_ram_addr - 256.U(20.W)
  io.dbuff_notif_o.bits.dbuffered_bytes  := 0.U
  io.dbuff_notif_o.bits.granted_bytes    := 0.U
  io.dbuff_notif_o.bits.priority         := 1.U  // TODO update

  // Did a dma request just arrive
  when(io.dma_read_req_i.valid) {
    io.pcie_read_cmd_o.valid             := true.B
    
    tag_mem.write(tag, io.dma_read_req_i.bits)

    tag := tag + 1.U(8.W)
  }

  when (io.pcie_read_status_i.valid) {
    io.dbuff_notif_o.valid := true.B
  }
}

object Main extends App {
  // These lines generate the Verilog output
  println(
    ChiselStage.emitSystemVerilog(
      new h2c_dma,
      firtoolOpts = Array("-disable-all-randomization", "-strip-debug-info")
    )
  )
}
