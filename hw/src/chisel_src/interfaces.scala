package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._
import scala.collection.immutable.ListMap

/*
 * pcie_read_cmd_t - read request to pcie core with a destination in psdram for the result. This must match the instantiation of the pcie core. 
 */
// TODO this should probably be parameterized from the top
class dma_read_desc_t extends Bundle {
  val pcie_addr = UInt(64.W)
  val ram_sel   = UInt(2.W)
  val ram_addr  = UInt(18.W)
  val len       = UInt(16.W)
  val tag       = UInt(8.W)
}

class dma_write_desc_t extends Bundle {
  val pcie_addr = UInt(64.W)
  val ram_sel   = UInt(2.W)
  val ram_addr  = UInt(18.W)
  val len       = UInt(16.W)
  val tag       = UInt(8.W)
}

class ram_read_desc_t extends Bundle {
 val ram_addr = UInt(18.W)
 val len      = UInt(64.W)
 val tag      = UInt(8.W)
 val id       = UInt(1.W)
 val dest     = UInt(8.W)
 val user     = UInt(1.W)
}

class ram_read_data_t extends Bundle {
 val data = UInt(512.W)
 val keep = UInt(64.W)
 val last = UInt(1.W)
 val id   = UInt(1.W)
 val dest = UInt(8.W)
 val user = UInt(1.W)
}

class ram_read_desc_status_t extends Bundle {
 val tag   = UInt(8.W)
 val error = UInt(4.W)
}

class ram_write_desc_t extends Bundle {
  val ram_addr = UInt(18.W)
  val len      = UInt(64.W)
  val tag      = UInt(8.W)
}

class ram_write_data_t extends Bundle {
  val data = UInt(512.W)
  val keep = UInt(64.W)
  val last = UInt(1.W)
  val id   = UInt(8.W)
  val dest = UInt(8.W)
  val user = UInt(1.W)
}

class ram_write_desc_status_t extends Bundle {
  val len   = UInt(64.W)
  val tag   = UInt(8.W)
  val id    = UInt(1.W)
  val dest  = UInt(8.W)
  val user  = UInt(1.W)
  val error = UInt(4.W)
}

class dma_read_desc_status_t extends Bundle {
  val tag   = UInt(8.W)
  val error = UInt(4.W)
}

class dma_write_desc_status_t extends Bundle {
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

class dma_map_t extends Bundle {
  val pcie_addr = UInt(64.W)
  val port      = UInt(16.W)
  val map_type  = dma_map_type()
}

// TODO these should be parmeterizable
class ram_wr_t extends Bundle {
  val cmd_be     = Output(UInt(128.W))
  val cmd_addr   = Output(UInt(22.W))
  val cmd_data   = Output(UInt(1024.W))
  val cmd_valid  = Output(UInt(2.W))
  val cmd_ready  = Input(UInt(2.W))
  val done    = Input(UInt(2.W))
}

class ram_rd_t extends Bundle {
  // val cmd_sel    = Output(UInt(2.W))
  val cmd_addr   = Output(UInt(22.W))
  val cmd_valid  = Output(UInt(2.W))
  val cmd_ready  = Input(UInt(2.W))
  val resp_data  = Input(UInt(1024.W))
  val resp_valid = Input(UInt(2.W))
  val resp_ready = Output(UInt(2.W))
}

//class pcie_raw extends Bundle {
//  val pcie_rx_p     = Input(UInt(1.W))
//  val pcie_rx_n     = Input(UInt(1.W))
//  val pcie_tx_p     = Output(UInt(1.W))
//  val pcie_tx_n     = Output(UInt(1.W))
//  val pcie_refclk_p = Input(Clock())
//  val pcie_refclk_n = Input(Clock())
//  val pcie_reset_n  = Input(UInt(1.W))
//}

// Default axi_m
class axi(AXI_DATA_WIDTH: Int, AXI_ID_WIDTH: Int, AXI_ADDR_WIDTH: Int) extends Bundle {
  val awid    = Output(UInt(AXI_ID_WIDTH.W))
  val awaddr  = Output(UInt(AXI_ADDR_WIDTH.W))
  val awlen   = Output(UInt(8.W))
  val awsize  = Output(UInt(3.W))
  val awburst = Output(UInt(2.W))
  val awlock  = Output(UInt(1.W))
  val awcache = Output(UInt(4.W))
  val awprot  = Output(UInt(3.W))
  val awvalid = Output(UInt(1.W))
  val awready = Input(UInt(1.W))
  val wdata   = Output(UInt(AXI_DATA_WIDTH.W))
  val wstrb   = Output(UInt((AXI_DATA_WIDTH/8).W))
  val wlast   = Output(UInt(1.W))
  val wvalid  = Output(UInt(1.W))
  val wready  = Input(UInt(1.W))
  val bid     = Input(UInt(AXI_ID_WIDTH.W))
  val bresp   = Input(UInt(2.W))
  val bvalid  = Input(UInt(1.W))
  val bready  = Output(UInt(1.W))
  val arid    = Output(UInt(AXI_ID_WIDTH.W))
  val araddr  = Output(UInt(AXI_ADDR_WIDTH.W))
  val arlen   = Output(UInt(8.W))
  val arsize  = Output(UInt(4.W)) 
  val arburst = Output(UInt(2.W))
  val arlock  = Output(UInt(1.W))
  val arcache = Output(UInt(4.W))
  val arprot  = Output(UInt(3.W))
  val arvalid = Output(UInt(1.W))
  val arready = Input(UInt(1.W))
  val rid     = Input(UInt(AXI_ID_WIDTH.W))
  val rdata   = Input(UInt(AXI_DATA_WIDTH.W))
  val rresp   = Input(UInt(2.W))
  val rlast   = Input(UInt(1.W))
  val rvalid  = Input(UInt(1.W))
  val rready  = Output(UInt(1.W))
}


// Default axis_m
// TODO make parameterizable
class axis(
  DATA_WIDTH: Int,
  ENABLE_ID: Boolean,
  ID_WIDTH: Int,
  ENABLE_USER: Boolean,
  USER_WIDTH: Int,
  ENABLE_LAST: Boolean) extends Bundle {
  val tvalid = Output(Bool())
  val tready = Input(Bool())
  val tdata  = Output(UInt(DATA_WIDTH.W))
  val tid    = if (ENABLE_ID) Some(Output(UInt(ID_WIDTH.W))) else None     // Optional 
  val tuser  = if (ENABLE_USER) Some(Output(UInt(USER_WIDTH.W))) else None // Optional
  val tlast  = if (ENABLE_LAST) Some(Output(Bool())) else None             // Optional
}

/* Offsets within the sendmsg and recvmsg bitvector for sendmsg and
 * recvmsg requests that form the msghdr
 */
class msghdr_send_t extends Bundle {
  val saddr     = UInt(128.W)
  val daddr     = UInt(128.W)
  val sport     = UInt(16.W)
  val dport     = UInt(16.W)
  val buff_addr = UInt(32.W)
  val ret       = UInt(12.W)
  val buff_size = UInt(20.W)
  val send_id   = UInt(64.W)
  val send_cc   = UInt(64.W)
}

// TODO this can be specilized now
class queue_entry_t extends Bundle {
  val rpc_id    = UInt(16.W)
  val dbuff_id  = UInt(16.W)
  val remaining = UInt(20.W)
  val dbuffered = UInt(20.W)
  val granted   = UInt(20.W)
  val priority  = UInt(3.W)
}


//class dbuff_notif_t extends Bundle {
//  val rpc_id          = UInt(16.W) // TODO make this the same across all
//  val cache_id        = UInt(10.W)
//  val remaining_bytes = UInt(20.W)
//  val dbuffered_bytes = UInt(20.W)
//  val granted_bytes   = UInt(20.W)
//  val priority        = UInt(3.W)
//}


object dma_map_type extends ChiselEnum {
  val h2c_map, c2h_map, meta_map = Value
}

class MyBundle extends Record {
  val elements = ListMap(Seq.tabulate(10) { i =>
    s"foo$i" -> Input(UInt(i.W))
  }:_*)

  val clk    = Input(Clock())
  val resetn = Input(Reset())
  // override def cloneType: this.type = (new MyBundle).asInstanceOf[this.type]
}
