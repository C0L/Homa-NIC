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

  val h2cPayloadRamReq  = IO(Flipped(Decoupled(new RamReadReq))) // Read request for payload data out of RAM
  val h2cPayloadRamResp = IO(Decoupled(new RamReadResp)) // Read response data from payload RAM read

  // Write data from h2cPayloadRam into DMA
  val c2hPayloadRamReq = IO(Flipped(Decoupled(new RamWriteReq))) // Write payload data to RAM
  val c2hMetadataRamReq = IO(Flipped(Decoupled(new RamWriteReq))) // Write metadata data to RAM

  // TODO change these all to DmaWrite/DmaRead. Ignore all status

  // Read data from DMA into h2cPayloadRam
  val h2cPayloadDmaReq  = IO(Flipped(Decoupled(new DmaReadReq)))
  // val h2cPayloadDmaStat = IO(Decoupled(new dma_read_desc_status_t(1)))

  // Write data from c2hPayloadRam into DMA
  val c2hPayloadDmaReq  = IO(Flipped(Decoupled(new DmaWriteReq)))
  // val c2hPayloadDmaStat = IO(Decoupled(new dma_write_desc_status_t(1)))

  // Write data from c2hMetadataRam into DMA
  val c2hMetadataDmaReq  = IO(Flipped(Decoupled(new DmaWriteReq)))
  // val c2hMetadataDmaStat = IO(Decoupled(new dma_write_desc_status_t(1)))

  val m_axi                 = IO(new axi(512, 26, true, 8, true, 4, true, 4))

  val pcie_user_clk         = IO(Output(Clock()))
  val pcie_user_reset       = IO(Output(Bool()))

  val pcie_core = Module(new PCIe(2))

  // Read data from payload RAM
  val h2cPayloadRd  = Module(new SegmentedRamRead)
  // Write metadata to payload RAM
  val c2hMetadataWr = Module(new SegmentedRamWrite)
  // Write payload data to RAM
  val c2hPayloadWr  = Module(new SegmentedRamWrite)
  // Host to card payload RAM
  val h2cPayloadRam = Module(new SegmentedRam(262144))
  // Card to host payload RAM
  val c2hPayloadRam  = Module(new SegmentedRam(262144))
  // Card to host metadata
  val c2hMetadataRam = Module(new SegmentedRam(262144))

  m_axi <> pcie_core.io.m_axi

  pcie_user_clk   := pcie_core.io.pcie_user_clk
  pcie_user_reset := pcie_core.io.pcie_user_reset

  h2cPayloadRamReq  <> h2cPayloadRd.io.ramReadReq
  h2cPayloadRamResp <> h2cPayloadRd.io.ramReadResp

  c2hPayloadRamReq  <> c2hPayloadWr.io.ramWriteReq
  c2hMetadataRamReq <> c2hMetadataWr.io.ramWriteReq

  h2cPayloadDmaReq  <> pcie_core.io.dmaReadReq(0)
  // h2cPayloadDmaStat <> pcie_core.io.dmaReadStat(0)

  c2hPayloadDmaReq  <> pcie_core.io.dmaWriteReq(0)
  // c2hPayloadDmaStat <> pcie_core.io.dmaWriteStat(0)

  c2hMetadataDmaReq  <> pcie_core.io.dmaWriteReq(1)
  // c2hMetadataDmaStat <> pcie_core.io.dmaWriteStat(1)

  pcie_core.io.dmaReadReq(1) := DontCare
  // pcie_core.io.dmaReadStat(1) := DontCare

  pcie_core.io.ram_wr(1) := DontCare

  h2cPayloadRam.io.ram_wr <> pcie_core.io.ram_wr(0)
  h2cPayloadRam.io.ram_rd <> h2cPayloadRd.io.ram_rd 

  c2hPayloadRam.io.ram_wr <> c2hPayloadWr.io.ram_wr
  c2hPayloadRam.io.ram_rd <> pcie_core.io.ram_rd(0)

  c2hMetadataRam.io.ram_wr <> c2hMetadataWr.io.ram_wr
  c2hMetadataRam.io.ram_rd <> pcie_core.io.ram_rd(1)

  // Off chip pcie lanes, clock, and reset
  pcie_core.io.pcie_rx_p     <> pcie_rx_p
  pcie_core.io.pcie_rx_n     <> pcie_rx_n
  pcie_core.io.pcie_tx_p     <> pcie_tx_p
  pcie_core.io.pcie_tx_n     <> pcie_tx_n
  pcie_core.io.pcie_refclk_p <> pcie_refclk_p
  pcie_core.io.pcie_refclk_n <> pcie_refclk_n
  pcie_core.io.pcie_reset_n  <> pcie_reset_n

  /* DEBUGGING ILAS */

  val h2c_psdpram_wr_ila = Module(new ILA(Flipped(new ram_wr_t(1))))
  h2c_psdpram_wr_ila.io.ila_data := pcie_core.io.ram_wr(1)

  val h2c_psdpram_rd_ila = Module(new ILA(Flipped(new ram_rd_t(1))))
  h2c_psdpram_rd_ila.io.ila_data := pcie_core.io.ram_rd(1)

  val c2h_psdpram_rd_ila = Module(new ILA(Flipped(new ram_rd_t(1))))
  c2h_psdpram_rd_ila.io.ila_data := pcie_core.io.ram_rd(1)
}

