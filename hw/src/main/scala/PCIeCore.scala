package gpnic

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

/* pcie_core - Groups pcie logic. The address map stores DMA
 * translation addressess. When DMA read and write operations are
 * inputed to pcie_core, the addr_map core translates them to absolute
 * DMA addresses and feeds them to the dma_ctrl core. This core stores
 * message state associated with that request and makes the actual
 * request to the pcie RTL core. The RPL interacts only with the
 * psdprams to read and write data.
 */
class pcie_core extends Module {
  val pcie_rx_p             = IO(Input(UInt(16.W)))
  val pcie_rx_n             = IO(Input(UInt(16.W)))
  val pcie_tx_p             = IO(Output(UInt(16.W)))
  val pcie_tx_n             = IO(Output(UInt(16.W)))
  val pcie_refclk_p         = IO(Input(Clock()))
  val pcie_refclk_n         = IO(Input(Clock()))
  val pcie_reset_n          = IO(Input(Reset()))

  val ram_read_desc         = IO(Flipped(Decoupled(new ram_read_desc_t)))
  val ram_read_desc_status  = IO(Decoupled(new ram_read_desc_status_t))
  val ram_read_data         = IO(Decoupled(new ram_read_data_t))

  val ram_write_data        = IO(Flipped(Decoupled(new RamWrite)))

  val dma_read_desc         = IO(Flipped(Decoupled(new dma_read_desc_t)))
  val dma_read_desc_status  = IO(Decoupled(new dma_read_desc_status_t))

  val dma_write_desc        = IO(Flipped(Decoupled(new dma_write_desc_t)))
  val dma_write_desc_status = IO(Decoupled(new dma_write_desc_status_t))

  val m_axi                 = IO(new axi(512, 26, true, 8, true, 4, true, 4))

  val pcie_user_clk         = IO(Output(Clock()))
  val pcie_user_reset       = IO(Output(Bool()))

  val pcie_core             = Module(new pcie_rtl)
  val dma_client_read       = Module(new SegmentedRAMRead)
  val dma_client_write      = Module(new SegmentedRAMWrite)
  val h2c_psdpram           = Module(new SegmentedRAM(262144))
  val c2h_psdpram           = Module(new SegmentedRAM(262144))

  m_axi <> pcie_core.io.m_axi

  pcie_user_clk   := pcie_core.io.pcie_user_clk
  pcie_user_reset := pcie_core.io.pcie_user_reset

  dma_read_desc <> pcie_core.io.dma_read_desc
  dma_read_desc_status <> pcie_core.io.dma_read_desc_status

  dma_write_desc <> pcie_core.io.dma_write_desc
  dma_write_desc_status <> pcie_core.io.dma_write_desc_status

  // Off chip pcie lanes, clock, and reset
  pcie_core.io.pcie_rx_p     <> pcie_rx_p
  pcie_core.io.pcie_rx_n     <> pcie_rx_n
  pcie_core.io.pcie_tx_p     <> pcie_tx_p
  pcie_core.io.pcie_tx_n     <> pcie_tx_n
  pcie_core.io.pcie_refclk_p <> pcie_refclk_p
  pcie_core.io.pcie_refclk_n <> pcie_refclk_n
  pcie_core.io.pcie_reset_n  <> pcie_reset_n

  // pcie_core writes to segmented ram to read from DMA
  h2c_psdpram.io.ram_wr <> pcie_core.io.ram_wr

  // pcie_core reads from segmented ram to write to DMA
  c2h_psdpram.io.ram_rd <> pcie_core.io.ram_rd

  dma_client_read.io.ram_read_desc <> ram_read_desc 
  dma_client_read.io.ram_read_data <> ram_read_data

  dma_client_read.io.ram_rd <> h2c_psdpram.io.ram_rd

  ram_read_desc_status := DontCare

  dma_client_write.io.ram_write_cmd <> ram_write_data
  dma_client_write.io.ram_wr        <> c2h_psdpram.io.ram_wr

  /* DEBUGGING ILAS */

  val h2c_psdpram_wr_ila = Module(new ILA(Flipped(new ram_wr_t)))
  h2c_psdpram_wr_ila.io.ila_data := pcie_core.io.ram_wr

  val h2c_psdpram_rd_ila = Module(new ILA(Flipped(new ram_rd_t)))
  h2c_psdpram_rd_ila.io.ila_data := pcie_core.io.ram_rd

  val c2h_psdpram_rd_ila = Module(new ILA(Flipped(new ram_rd_t)))
  c2h_psdpram_rd_ila.io.ila_data := pcie_core.io.ram_rd
}

