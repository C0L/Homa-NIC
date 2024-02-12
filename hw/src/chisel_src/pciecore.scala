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

  val ram_write_desc        = IO(Flipped(Decoupled(new ram_write_desc_t)))
  val ram_write_data        = IO(Flipped(Decoupled(new ram_write_data_t)))
  val ram_write_desc_status = IO(Decoupled(new ram_write_desc_status_t))

  val dma_write_desc        = IO(Flipped(Decoupled(new dma_write_desc_t)))
  val dma_write_desc_status = IO(Decoupled(new dma_write_desc_status_t))

  val m_axi                 = IO(new axi(512, 26, true, 8, true, 4, true, 4))

  val pcie_user_clk         = IO(Output(Clock()))
  val pcie_user_reset       = IO(Output(Bool()))

  val dma_r_req_i           = IO(Flipped(Decoupled(new dma_read_t)))
  val dbuff_notif_o         = IO(Decoupled(new queue_entry_t))
  val dma_map_i             = IO(Flipped(Decoupled(new dma_map_t)))

  val pcie_core             = Module(new pcie_rtl)
  val dma_client_read       = Module(new dma_client_axis_source)
  val dma_client_write      = Module(new dma_client_axis_sink)
  val h2c_psdpram           = Module(new dma_psdpram)
  val c2h_psdpram           = Module(new dma_psdpram)
  val addr_map              = Module(new addr_map) // Map read and write requests to DMA address
  val h2c_dma               = Module(new h2c_dma)  // Manage dma reads and writes TODO maybe redundant now

  addr_map.io.dma_map_i         <> dma_map_i
  h2c_dma.io.pcie_read_cmd_o    <> pcie_core.io.dma_read_desc
  h2c_dma.io.pcie_read_status_i <> pcie_core.io.dma_read_desc_status
  m_axi <> pcie_core.io.m_axi
  h2c_dma.io.dbuff_notif_o <> dbuff_notif_o 

  addr_map.io.dma_w_meta_i  := DontCare
  addr_map.io.dma_w_data_i  := DontCare
  addr_map.io.dma_w_req_o   := DontCare

  addr_map.io.dma_r_req_i <> dma_r_req_i
  addr_map.io.dma_r_req_o <> h2c_dma.io.dma_read_req_i //TODO inconsistent names

  pcie_user_clk   := pcie_core.io.pcie_user_clk
  pcie_user_reset := pcie_core.io.pcie_user_reset

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

  // Pcie DMA descriptor interface
  pcie_core.io.dma_write_desc        <> dma_write_desc
  pcie_core.io.dma_write_desc_status <> dma_write_desc_status

  // pcie_core writes to segmented ram to read from DMA
  h2c_psdpram.io.ram_wr <> pcie_core.io.ram_wr

  // pcie_core reads from segmented ram to write to DMA
  c2h_psdpram.io.ram_rd <> pcie_core.io.ram_rd

  // DMA client accepts read descriptors from the packet egress logic
  dma_client_read.io.ram_read_desc        <> ram_read_desc 
  // DMA client returns read status to the packet egress logic
  dma_client_read.io.ram_read_desc_status <> ram_read_desc_status
  // DMA client returns read data to the packet egress logic
  dma_client_read.io.ram_read_data        <> ram_read_data

  dma_client_read.io.enable := 1.U(1.W)
  
  dma_client_read.io.ram_rd <> h2c_psdpram.io.ram_rd

  // DMA client accepts write descriptors from the packet ingress logic
  dma_client_write.io.ram_write_desc        <> ram_write_desc
  // DMA client accepts write data from the packet ingress logic
  dma_client_write.io.ram_write_data        <> ram_write_data
  // DMA client returns write status to the packet ingress logic
  dma_client_write.io.ram_write_desc_status <> ram_write_desc_status

  dma_client_write.io.enable := 1.U(1.W)

  dma_client_write.io.ram_wr <> c2h_psdpram.io.ram_wr

  // TODO maybe for recursive def?
  // override def typeName = s"MyBundle_${gen.typeName}_${intParam}"
}

