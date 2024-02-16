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
  val send_cc   = UInt(64.W)
  val send_id   = UInt(64.W)
  val buff_size = UInt(20.W)
  val ret       = UInt(12.W)
  val buff_addr = UInt(32.W)
  val dport     = UInt(16.W)
  val sport     = UInt(16.W)
  val daddr     = UInt(128.W)
  val saddr     = UInt(128.W)
}

// TODO this can be specilized now
class queue_entry_t extends Bundle {
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

// #define IPV6_VERSION       6
// #define IPV6_TRAFFIC       0
// #define IPV6_FLOW          0xFFFF
// #define IPV6_ETHERTYPE     0x86DD
// #define IPV6_HOP_LIMIT     0x00
// #define IPPROTO_HOMA       0xFD
// #define IPV6_HEADER_LENGTH 40
// 
// // TODO clean this up
// #define HDR_IPV6_MAC_DEST         511+512,464+512 
// #define HDR_IPV6_MAC_SRC          463+512,416+512
// #define HDR_IPV6_ETHERTYPE        415+512,400+512
// #define HDR_IPV6_VERSION          399+512,396+512
// #define HDR_IPV6_TRAFFIC          395+512,388+512
// #define HDR_IPV6_FLOW             387+512,368+512
// #define HDR_IPV6_PAYLOAD_LEN      367+512,352+512
// #define HDR_IPV6_NEXT_HEADER      351+512,344+512
// #define HDR_IPV6_HOP_LIMIT        343+512,336+512
// #define HDR_IPV6_SADDR            335+512,208+512
// #define HDR_IPV6_DADDR            207+512,80+512
// #define HDR_HOMA_COMMON_SPORT     79+512,64+512 
// #define HDR_HOMA_COMMON_DPORT     63+512,48+512
// #define HDR_HOMA_COMMON_UNUSED0   47+512,0+512
// #define HDR_HOMA_COMMON_UNUSED1   511,496
// #define HDR_HOMA_COMMON_DOFF      495,488
// #define HDR_HOMA_COMMON_TYPE      487,480
// #define HDR_HOMA_COMMON_UNUSED3   479,464
// #define HDR_HOMA_COMMON_CHECKSUM  463,448
// #define HDR_HOMA_COMMON_UNUSED4   447,432
// #define HDR_HOMA_COMMON_SENDER_ID 431,368
// #define HDR_HOMA_DATA_MSG_LEN     367,336
// #define HDR_HOMA_DATA_INCOMING    335,304
// #define HDR_HOMA_DATA_CUTOFF      303,288
// #define HDR_HOMA_DATA_REXMIT      287,280
// #define HDR_HOMA_DATA_PAD         279,272
// #define HDR_HOMA_DATA_OFFSET      271,240
// #define HDR_HOMA_DATA_SEG_LEN     239,208
// #define HDR_HOMA_ACK_ID           207,144
// #define HDR_HOMA_ACK_SPORT        143,128
// #define HDR_HOMA_ACK_DPORT        127,112
// 
// #define OUT_OF_BAND               111,0
// 
// 
// /* Ethernet Header Constants
//  *
//  * TODO: Who is responsible for setting the MAX_DST and MAX_SRC? The
//  * PHY?
//  */
// #define MAC_DST 0xFFFFFFFFFFFF
// #define MAC_SRC 0xDEADBEEFDEAD
// 
// /* Homa Header Constants: Used for computing packet chunk offsets
//  */
// #define DOFF             160 // Number of 4 byte chunks in the data header (<< 2)
// #define DATA_PKT_HEADER  114 // How many bytes the Homa DATA packet header takes
// #define GRANT_PKT_HEADER 87  // Number of bytes in ethernet + ipv6 + common + data 
// #define PREFACE_HEADER   54  // Number of bytes in ethernet + ipv6 header 
// #define HOMA_DATA_HEADER 60  // Number of bytes in homa data ethernet header
