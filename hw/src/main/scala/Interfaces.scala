package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._
import scala.collection.immutable.ListMap

class dma_read_desc_t (ports: Int) extends Bundle {
  val pcie_addr = Output(UInt((ports * 64).W))
  val ram_sel   = Output(UInt((ports * 2).W))
  val ram_addr  = Output(UInt((ports * 18).W))
  val len       = Output(UInt((ports * 16).W))
  val tag       = Output(UInt((ports * 8).W))
}

class dma_write_desc_t (ports: Int) extends Bundle {
  val pcie_addr = Output(UInt((ports * 64).W))
  val ram_sel   = Output(UInt((ports * 2).W))
  val ram_addr  = Output(UInt((ports * 18).W))
  val len       = Output(UInt((ports * 16).W))
  val tag       = Output(UInt((ports * 8).W))
}

class DmaReq extends Bundle {
  val pcie_addr = UInt(64.W)
  val ram_sel   = UInt(2.W)
  val ram_addr  = UInt(18.W)
  val len       = UInt(16.W)
  val tag       = UInt(8.W)
  val port      = UInt(16.W)
}

class DmaReadStat extends Bundle {
  val tag   = UInt(8.W)
  val error = UInt(4.W)
}

class DmaWriteStat extends Bundle {
  val tag   = UInt(8.W)
  val error = UInt(4.W)
}

class RamReadReq extends Bundle {
  val addr = UInt(18.W)
  val len  = UInt(8.W)
}

class RamReadResp extends Bundle {
  val data = UInt(512.W)
}

class RamWriteReq extends Bundle {
  val addr = UInt(18.W)
  val len  = UInt(8.W)
  val data = UInt(512.W)
}

class DmaMap extends Bundle {
  val map_type  = UInt(8.W)
  val port      = UInt(16.W)
  val pcie_addr = UInt(64.W)
}

class ram_wr_t (ports: Int) extends Bundle {
  val cmd_be     = Output(UInt((ports * 128).W))
  val cmd_addr   = Output(UInt((ports * 22).W))
  val cmd_data   = Output(UInt((ports * 1024).W))
  val cmd_valid  = Output(UInt((ports * 2).W))
  val cmd_ready  = Input(UInt((ports * 2).W))
  val done       = Input(UInt((ports * 2).W))
}

class ram_rd_t (ports: Int) extends Bundle {
  val cmd_addr   = Output(UInt((ports * 22).W))
  val cmd_valid  = Output(UInt((ports * 2).W))
  val cmd_ready  = Input(UInt((ports * 2).W))
  val resp_data  = Input(UInt((ports * 1024).W))
  val resp_valid = Input(UInt((ports * 2).W))
  val resp_ready = Output(UInt((ports * 2).W))
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
// TODO this name is not great as it goes from being a packet factory to a frame factory
class PacketFactory extends Bundle {
  val eth       = new EthHeader
  val ipv6      = new Ipv6Header
  val common    = new HomaCommonHeader
  val data      = new HomaDataHeader
  val payload   = UInt(512.W)
  val cb        = new msghdr_t
  val trigger   = new QueueEntry
  val frame_off = UInt(32.W)

  def wireFrame(): UInt = {
    val frame = Wire(UInt(512.W))

    when (frame_off === 0.U) {
      // First 64 bytes
      frame := this.asUInt(this.getWidth - 1, this.getWidth - 512) 
    }.elsewhen (frame_off === 64.U) {
      // Second 64 bytes
      frame := this.asUInt(this.getWidth - 512 - 1, this.getWidth - 1024)
    }.otherwise {
      frame := this.payload
    }

    frame
  }

  /* packetBytes - The total number of bytes in the packet
   * corressponding to this packet factory.
   */
  def packetBytes(): UInt = {
    val packet_len = Wire(UInt(32.W))
    packet_len := data.data.seg_len + NetworkCfg.headerBytes.U
    packet_len
  }


  /* lastFrame - bit indicating whether this packet factory generate
   * the last frame in a packet.
   */
  def lastFrame(): UInt = {
    val last = Wire(UInt(1.W))
    // If the next 64 bytes from frame offset exceeds the size then set last signal
    when (frame_off + 64.U > NetworkCfg.headerBytes.U + data.data.seg_len) {
      last := 1.U
    }.otherwise {
      last := 0.U
    }
    last
  }

  /* payloadBytes - The number of bytes (0-64) which this packet
   * factory carries.
   */
  def payloadBytes(): UInt = {
    val bytes = Wire(UInt(32.W))

    when (frame_off === 0.U) {
      bytes := 0.U
    }.elsewhen (frame_off === 64.U) {
      bytes := (128 - NetworkCfg.headerBytes).U
    }.otherwise {
      bytes := 64.U
    }

    val truncbytes = Wire(UInt(32.W))
    truncbytes := bytes

    when (this.payloadOffset() + bytes > data.data.seg_len) {
      truncbytes := data.data.seg_len - this.payloadOffset()
    }
    truncbytes
  }

  /* payloadOffset - Offset of the 0-64 Bytes of payload of this packet
   * factory within a single PacketFrameFactory
   */
  def payloadOffset(): UInt = {
    val offset = Wire(UInt(32.W))

    /* The first frame has no payload, so leave offset as 0. The
     * second frame has payload which starts at offset 0 from the
     * segment offset. Sucessive frames have an offset that begins
     * after the first partial chunk.
     */
    when (frame_off <= 64.U) {
      offset := 0.U
    }.otherwise {
      offset := frame_off - NetworkCfg.headerBytes.U
    }
    offset
  }

  /* keepBytes - Generate a corresponding keep signal for the ethernet
   * core identifying the size of the frame passed to it.
   */
  def keepBytes(): UInt = {
    val keep = Wire(UInt(64.W))
    // If the next 64 bytes from frame offset exceeds the size then set last signal
    when (frame_off + 64.U > NetworkCfg.headerBytes.U + data.data.seg_len) {
      keep := ("hffffffffffffffff".U(64.W) << (frame_off + 64.U - (NetworkCfg.headerBytes.U + data.data.seg_len))(5,0))
    }.otherwise {
      keep := "hffffffffffffffff".U(64.W)
    }

    keep
  }
}

class EthHeader extends Bundle {
  val mac_dest    = UInt(48.W)
  val mac_src     = UInt(48.W)
  val ethertype   = UInt(16.W)
}

class Ipv6Header extends Bundle {
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
  val unused1   = UInt(32.W)
  val unused2   = UInt(32.W) 
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

class CAMEntry (KEY_WIDTH: Int, VALUE_WIDTH: Int) extends Bundle {
  val key   = UInt(KEY_WIDTH.W)
  val value = UInt(VALUE_WIDTH.W)
  val set   = UInt(1.W)
  // val tag   = UInt(32.W)

  val seeds = List("h7BF6BF21", "h9FA91FE9", "hD0C8FBDF", "hE6D0851C")

  // TODO Murmur3 is probably better here
  def hash(TABLE: Int): UInt = {
    val result = Wire(UInt(32.W))
    val vec = key.asTypeOf(Vec(KEY_WIDTH/32, UInt(32.W)))

    result := vec.reduce(_ + _)
    result ^ seeds(TABLE).U
    result % 30133.U
  }
}
