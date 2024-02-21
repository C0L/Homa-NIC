package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._
import scala.collection.immutable.ListMap

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
  val pcie_read_addr = UInt(64.W)
  val cache_id       = UInt(10.W)
  val message_size   = UInt(20.W)
  val read_len       = UInt(12.W)
  val dest_ram_addr  = UInt(20.W)
  val port           = UInt(16.W)
}

class dma_write_t extends Bundle {
  val pcie_write_addr = UInt(64.W)
  val data            = UInt(512.W)
  val length          = UInt(8.W)
  val port            = UInt(16.W)
}

class dma_map_t extends Bundle {
  val map_type  = UInt(8.W)
  val port      = UInt(16.W)
  val pcie_addr = UInt(64.W)
}

class ram_wr_t extends Bundle {
  val cmd_be     = Output(UInt(128.W))
  val cmd_addr   = Output(UInt(22.W))
  val cmd_data   = Output(UInt(1024.W))
  val cmd_valid  = Output(UInt(2.W))
  val cmd_ready  = Input(UInt(2.W))
  val done       = Input(UInt(2.W))
}

class ram_rd_t extends Bundle {
  val cmd_addr   = Output(UInt(22.W))
  val cmd_valid  = Output(UInt(2.W))
  val cmd_ready  = Input(UInt(2.W))
  val resp_data  = Input(UInt(1024.W))
  val resp_valid = Input(UInt(2.W))
  val resp_ready = Output(UInt(2.W))
}

// Default axi_m
class axi(
  AXI_DATA_WIDTH: Int,
  AXI_ADDR_WIDTH: Int,
  ENABLE_ID: Boolean,
  AXI_ID_WIDTH: Int,
  ENABLE_REGION: Boolean,
  AXI_REGION_WIDTH: Int,
  ENABLE_QOS: Boolean,
  AXI_QOS_WIDTH: Int
  ) extends Bundle {
  val awid     = Output(UInt(AXI_ID_WIDTH.W))
  val awaddr   = Output(UInt(AXI_ADDR_WIDTH.W))
  val awlen    = Output(UInt(8.W))
  val awsize   = Output(UInt(3.W))
  val awburst  = Output(UInt(2.W))
  val awlock   = Output(UInt(1.W))
  val awcache  = Output(UInt(4.W))
  val awprot   = Output(UInt(3.W))
  val awvalid  = Output(UInt(1.W))
  val awready  = Input(UInt(1.W))
  val awqos    = Output(UInt(AXI_QOS_WIDTH.W))
  val awregion = Output(UInt(AXI_REGION_WIDTH.W))
  val wdata    = Output(UInt(AXI_DATA_WIDTH.W))
  val wstrb    = Output(UInt((AXI_DATA_WIDTH/8).W))
  val wlast    = Output(UInt(1.W))
  val wvalid   = Output(UInt(1.W))
  val wready   = Input(UInt(1.W))
  val bid      = Input(UInt(AXI_ID_WIDTH.W))
  val bresp    = Input(UInt(2.W))
  val bvalid   = Input(UInt(1.W))
  val bready   = Output(UInt(1.W))
  val arid     = Output(UInt(AXI_ID_WIDTH.W))
  val araddr   = Output(UInt(AXI_ADDR_WIDTH.W))
  val arlen    = Output(UInt(8.W))
  val arsize   = Output(UInt(4.W)) 
  val arburst  = Output(UInt(2.W))
  val arlock   = Output(UInt(1.W))
  val arcache  = Output(UInt(4.W))
  val arprot   = Output(UInt(3.W))
  val arvalid  = Output(UInt(1.W))
  val arready  = Input(UInt(1.W))
  val arqos    = Output(UInt(AXI_QOS_WIDTH.W))
  val arregion = Output(UInt(AXI_REGION_WIDTH.W))
  val rid      = Input(UInt(AXI_ID_WIDTH.W))
  val rdata    = Input(UInt(AXI_DATA_WIDTH.W))
  val rresp    = Input(UInt(2.W))
  val rlast    = Input(UInt(1.W))
  val rvalid   = Input(UInt(1.W))
  val rready   = Output(UInt(1.W))
}

// Default axis_m
// TODO make parameterizable
class axis(
  DATA_WIDTH: Int,
  ENABLE_ID: Boolean,
  ID_WIDTH: Int,
  ENABLE_USER: Boolean,
  USER_WIDTH: Int,
  ENABLE_KEEP: Boolean,
  KEEP_WIDTH: Int,
  ENABLE_LAST: Boolean) extends Bundle {
  val tvalid = Output(Bool())
  val tready = Input(Bool())
  val tdata  = Output(UInt(DATA_WIDTH.W))
  val tid    = if (ENABLE_ID) Some(Output(UInt(ID_WIDTH.W))) else None     // Optional 
  val tuser  = if (ENABLE_USER) Some(Output(UInt(USER_WIDTH.W))) else None // Optional
  val tkeep  = if (ENABLE_KEEP) Some(Output(UInt(KEEP_WIDTH.W))) else None // Optional
  val tlast  = if (ENABLE_LAST) Some(Output(Bool())) else None             // Optional
}

/* Offsets within the sendmsg and recvmsg bitvector for sendmsg and
 * recvmsg requests that form the msghdr
 */
class msghdr_t extends Bundle {
  val cc        = UInt(64.W)
  val id        = UInt(64.W)
  val buff_size = UInt(20.W)
  val ret       = UInt(12.W)
  val buff_addr = UInt(32.W)
  val dport     = UInt(16.W)
  val sport     = UInt(16.W)
  val daddr     = UInt(128.W)
  val saddr     = UInt(128.W)
  val unused    = UInt(32.W)
}

// TODO this can be specilized now
class QueueEntry extends Bundle {
  val priority  = UInt(3.W)
  val granted   = UInt(20.W)
  val dbuffered = UInt(20.W)
  val remaining = UInt(20.W)
  val dbuff_id  = UInt(10.W)
  val rpc_id    = UInt(16.W)
}

object dma_map_type extends ChiselEnum {
  val H2C   = Value(0x00.U)
  val C2H   = Value(0x01.U)
  val META  = Value(0x02.U)
  val DUMMY = Value(0xFF.U)
}

object queue_priority extends ChiselEnum {
  val INVALIDATE   = Value(0x0.U) 
  val DBUFF_UPDATE = Value(0x1.U)
  val GRANT_UPDATE = Value(0x2.U)
  val EMPTY        = Value(0x3.U)
  val BLOCKED      = Value(0x4.U)
  val ACTIVE       = Value(0x5.U)
}

// TODO move this stuff to somewhere else?

object IPV6 {
  val version       = 6.U
  val traffic       = 0.U
  val flow          = "hFFFF".U
  val ethertype     = "h86DD".U
  val hop_limit     = 0.U
  val ipproto_homa  = "hFD".U
  val header_length = 40.U
}

object MAC {
  val dst = "hFFFFFFFFF".U
  val src = "hDEADBEEFD".U
}

object HOMA {
  val doff = 160.U
  val DATA = "h10".U
}

// TODO make this parmeterizable
// TODO this only generates on chunk
class PacketFactory extends Bundle {
  val eth     = new EthHeader
  val ipv6    = new IPV6Header
  val common  = new HomaCommonHeader
  val data    = new HomaDataHeader
  val payload = UInt(512.W)
  val cb      = new msghdr_t
  val trigger = new QueueEntry
  val frame   = UInt(32.W)
}

class EthHeader extends Bundle {
  val mac_dest    = UInt(36.W)
  val mac_src     = UInt(36.W)
  val ethertype   = UInt(16.W)
}

class IPV6Header extends Bundle {
  val version     = UInt(4.W)  
  val traffic     = UInt(8.W)  
  val flow        = UInt(20.W) 
  val payload_len = UInt(16.W) 
  val next_header = UInt(8.W)  
  val hop_limit   = UInt(8.W)  
  val saddr       = UInt(128.W)
  val daddr       = UInt(128.W)
}

class HomaCommonHeader extends Bundle {
  val sport     = UInt(16.W)
  val dport     = UInt(16.W)
  val unused0   = UInt(48.W)
  val unused1   = UInt(8.W) 
  val doff      = UInt(8.W) 
  val homatype  = UInt(8.W)
  val unused3   = UInt(16.W)
  val checksum  = UInt(16.W)
  val unused4   = UInt(16.W)
  val sender_id = UInt(64.W)
}

class HomaDataSegment extends Bundle {
  val offset  = UInt(32.W)
  val seg_len = UInt(32.W)
  val ack     = new HomaAck()
}

class HomaDataHeader extends Bundle {
  val msg_len  = UInt(32.W)
  val incoming = UInt(32.W) 
  val cutoff   = UInt(16.W)
  val rexmit   = UInt(8.W)
  val pad      = UInt(8.W)
  val data     = new HomaDataSegment
}

class HomaAck extends Bundle {
  val id    = UInt(64.W)
  val sport = UInt(16.W)
  val dport = UInt(16.W)
}
