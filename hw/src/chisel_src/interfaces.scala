package packetproc

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

/*
 * pcie_read_cmd_t - read request to pcie core with a destination in psdram for the result. This must match the instantiation of the pcie core. 
 */
// TODO this should probably be parameterized from the top
class dma_read_desc_t extends Bundle {
  val dma_read_desc_pcie_addr = UInt(64.W)
  val dma_read_desc_ram_sel   = UInt(2.W)
  val dma_read_desc_ram_addr  = UInt(18.W)
  val dma_read_desc_len       = UInt(16.W)
  val dma_read_desc_tag       = UInt(8.W)
}

class dma_write_desc_t extends Bundle {
  val dma_write_desc_pcie_addr = UInt(64.W)
  val dma_write_desc_ram_sel   = UInt(2.W)
  val dma_write_desc_ram_addr  = UInt(18.W)
  val dma_write_desc_len       = UInt(16.W)
  val dma_write_desc_tag       = UInt(8.W)
}

class dma_read_desc_status_t extends Bundle {
  val dma_read_desc_status_tag   = UInt(8.W)
  val dma_read_desc_status_error = UInt(4.W)
}

class dma_write_desc_status_t extends Bundle {
  val dma_write_desc_status_tag   = UInt(8.W)
  val dma_write_desc_status_error = UInt(4.W)
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

class dma_map_t extends Bundle {
  val pcie_addr = UInt(64.W)
  val port      = UInt(16.W)
  val map_type  = dma_map_type()
}

class pcie_raw extends Bundle {
  val pcie_rx_p     = Input(UInt(1.W))
  val pcie_rx_n     = Input(UInt(1.W))
  val pcie_tx_p     = Output(UInt(1.W))
  val pcie_tx_n     = Output(UInt(1.W))
  val pcie_refclk_p = Input(UInt(1.W))
  val pcie_refclk_n = Input(UInt(1.W))
  val pcie_reset_n  = Input(UInt(1.W))
}

class axi4(AXI_DATA_WIDTH: Int, AXI_ID_WIDTH: Int, AXI_ADDR_WIDTH: Int) extends Bundle {
  val m_axi_awid    = Output(UInt(AXI_ID_WIDTH.W))
  val m_axi_awaddr  = Output(UInt(AXI_ADDR_WIDTH.W))
  val m_axi_awlen   = Output(UInt(8.W))
  val m_axi_awsize  = Output(UInt(3.W))
  val m_axi_awburst = Output(UInt(2.W))
  val m_axi_awlock  = Output(UInt(1.W))
  val m_axi_awcache = Output(UInt(4.W))
  val m_axi_awprot  = Output(UInt(3.W))
  val m_axi_awvalid = Output(UInt(1.W))
  val m_axi_awready = Input(UInt(1.W))
  val m_axi_wdata   = Output(UInt(AXI_DATA_WIDTH.W))
  val m_axi_wstrb   = Output(UInt((AXI_DATA_WIDTH/8).W))
  val m_axi_wlast   = Output(UInt(1.W))
  val m_axi_wvalid  = Output(UInt(1.W))
  val m_axi_wready  = Input(UInt(1.W))
  val m_axi_bid     = Input(UInt(AXI_ID_WIDTH.W))
  val m_axi_bresp   = Input(UInt(2.W))
  val m_axi_bvalid  = Input(UInt(1.W))
  val m_axi_bready  = Output(UInt(1.W))
  val m_axi_arid    = Output(UInt(AXI_ID_WIDTH.W))
  val m_axi_araddr  = Output(UInt(AXI_ADDR_WIDTH.W))
  val m_axi_arlen   = Output(UInt(8.W))
  val m_axi_arsize  = Output(UInt(4.W)) 
  val m_axi_arburst = Output(UInt(2.W))
  val m_axi_arlock  = Output(UInt(1.W))
  val m_axi_arcache = Output(UInt(4.W))
  val m_axi_arprot  = Output(UInt(3.W))
  val m_axi_arvalid = Output(UInt(1.W))
  val m_axi_arready = Input(UInt(1.W))
  val m_axi_rid     = Input(UInt(AXI_ID_WIDTH.W))
  val m_axi_rdata   = Input(UInt(AXI_DATA_WIDTH.W))
  val m_axi_rresp   = Input(UInt(2.W))
  val m_axi_rlast   = Input(UInt(1.W))
  val m_axi_rvalid  = Input(UInt(1.W))
  val m_axi_rready  = Output(UInt(1.W))
}

object dma_map_type extends ChiselEnum {
  val h2c_map, c2h_map, meta_map = Value
}

