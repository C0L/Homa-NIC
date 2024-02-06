package gpnic

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

class pcie extends Module {
  val pcie_rx_p             = IO(Input(UInt(16.W)))
  val pcie_rx_n             = IO(Input(UInt(16.W)))
  val pcie_tx_p             = IO(Output(UInt(16.W)))
  val pcie_tx_n             = IO(Output(UInt(16.W)))
  val pcie_refclk_p         = IO(Input(Clock()))
  val pcie_refclk_n         = IO(Input(Clock()))
  val pcie_reset_n          = IO(Input(UInt(1.W)))

  val ram_read_desc         = IO(Flipped(Decoupled(new ram_read_desc_t)))
  val ram_read_desc_status  = IO(Decoupled(new ram_read_desc_status_t))
  val ram_read_data         = IO(Decoupled(new ram_read_data_t))

  val ram_write_desc        = IO(Flipped(Decoupled(new ram_write_desc_t)))
  val ram_write_data        = IO(Flipped(Decoupled(new ram_write_data_t)))
  val ram_write_desc_status = IO(Decoupled(new ram_write_desc_status_t))

  val dma_read_desc         = IO(Flipped(Decoupled(new dma_read_desc_t)))
  val dma_read_desc_status  = IO(Decoupled(new dma_read_desc_status_t))
  val dma_write_desc        = IO(Flipped(Decoupled(new dma_write_desc_t)))
  val dma_write_desc_status = IO(Decoupled(new dma_write_desc_status_t))

  val m_axi                 = IO(new axi4(512, 8, 26))

  val pcie_core             = Module(new pcie_core)
  val dma_client_read       = Module(new dma_client_axis_source)
  val dma_client_write      = Module(new dma_client_axis_sink)
  val h2c_psdpram           = Module(new dma_psdpram)
  val c2h_psdpram           = Module(new dma_psdpram)

  dma_client_read.io.clk  := pcie_core.io.pcie_user_clk
  dma_client_read.io.rst  := pcie_core.io.pcie_user_reset 

  dma_client_write.io.clk := pcie_core.io.pcie_user_clk
  dma_client_write.io.rst := pcie_core.io.pcie_user_reset

  h2c_psdpram.io.clk := pcie_core.io.pcie_user_clk
  h2c_psdpram.io.rst := pcie_core.io.pcie_user_reset

  c2h_psdpram.io.clk := pcie_core.io.pcie_user_clk
  c2h_psdpram.io.rst := pcie_core.io.pcie_user_reset

  // Off chip pcie lanes, clock, and reset
  pcie_core.io.pcie_rx_p     <> pcie_rx_p
  pcie_core.io.pcie_rx_n     <> pcie_rx_n
  pcie_core.io.pcie_tx_p     <> pcie_tx_p
  pcie_core.io.pcie_tx_n     <> pcie_tx_n
  pcie_core.io.pcie_refclk_p <> pcie_refclk_p
  pcie_core.io.pcie_refclk_n <> pcie_refclk_n
  pcie_core.io.pcie_reset_n  <> pcie_reset_n

  // AXI4 interface onto chip
  pcie_core.io.m_axi         <> m_axi

  // Pcie DMA descriptor interface
  pcie_core.io.dma_read_desc         <> dma_read_desc
  pcie_core.io.dma_read_desc_status  <> dma_read_desc_status
  pcie_core.io.dma_write_desc        <> dma_write_desc
  pcie_core.io.dma_write_desc_status <> dma_write_desc_status

  // pcie_core writes to segmented ram to read from DMA
  h2c_psdpram.io.wr <> pcie_core.io.ram_wr

  // pcie_core reads from segmented ram to write to DMA
  c2h_psdpram.io.rd <> pcie_core.io.ram_rd

  // DMA client accepts read descriptors from the packet egress logic
  dma_client_read.io.ram_read_desc        <> ram_read_desc 
  // DMA client returns read status to the packet egress logic
  dma_client_read.io.ram_read_desc_status <> ram_read_desc_status
  // DMA client returns read data to the packet egress logic
  dma_client_read.io.ram_read_data        <> ram_read_data

  dma_client_read.io.enable := 1.U(1.W)
  
  dma_client_read.io.rd <> h2c_psdpram.io.rd

  // DMA client accepts write descriptors from the packet ingress logic
  dma_client_write.io.ram_write_desc        <> ram_write_desc
  // DMA client accepts write data from the packet ingress logic
  dma_client_write.io.ram_write_data        <> ram_write_data
  // DMA client returns write status to the packet ingress logic
  dma_client_write.io.ram_write_desc_status <> ram_write_desc_status

  dma_client_write.io.enable := 1.U(1.W)

  dma_client_write.io.wr <> c2h_psdpram.io.wr
}

object Main extends App {
  ChiselStage.emitSystemVerilogFile(new pcie,
    firtoolOpts = Array("-disable-all-randomization", "-strip-debug-info")
  )

 ChiselStage.emitSystemVerilogFile(new addr_map,
    firtoolOpts = Array("-disable-all-randomization", "-strip-debug-info")
  )

 ChiselStage.emitSystemVerilogFile(new h2c_dma,
    firtoolOpts = Array("-disable-all-randomization", "-strip-debug-info")
  )
}
