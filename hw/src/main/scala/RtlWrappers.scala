package gpnic

import chisel3._
import chisel3.util._

/* srpt_queue - BlackBox module for the srpt queue Verilog
 * implementation. This name must match that of the Verilog.
 */
class srpt_queue (qtype: String) extends BlackBox (
    Map("MAX_RPCS" -> 64,
      "TYPE" -> qtype,
  )) with HasBlackBoxResource {

  val io = IO(new Bundle {
    val clk = Input(Clock())
    val rst = Input(UInt(1.W))

    val CACHE_BLOCK_SIZE = Input(UInt(16.W))

    val s_axis = Flipped(new axis(89, false, 0, false, 0, false, 0, false))
    val m_axis = new axis(89, false, 0, false, 0, false, 0, false)
  })

  addResource("/verilog_src/srpt_queue.v")
}

// TODO no flow control on DMA stat output!!!! 
// TODO move this outside of RTL wrappers
// TODO should move storage of message size outside?
class FetchQueue extends Module {
  val io = IO(new Bundle {
    val fetchSize = Input(UInt(16.W))
    val enqueue   = Flipped(Decoupled(new QueueEntry))
    val dequeue   = Decoupled(new DmaReadReq)
    val readcmpl  = Flipped(Decoupled(new DmaReadStat))
    val notifout  = Decoupled(new QueueEntry)
  })

  val fetch_queue_raw = Module(new srpt_queue("fetch"))

  fetch_queue_raw.io.clk := clock
  fetch_queue_raw.io.rst := reset.asUInt

  fetch_queue_raw.io.CACHE_BLOCK_SIZE := io.fetchSize

  /* Read requests are stored in a memory while they are pending
   * completion. When a status message is received indicating that the
   * requested data has arrived we can lookup the metadata associated
   * with that message for upating our queue state.
   */
  class ReqMem extends Bundle {
    val dma   = new DmaReadReq
    val queue = new QueueEntry
  }

  val tag = RegInit(0.U(8.W))
  val tagMem  = Mem(256, new ReqMem)

  fetch_queue_raw.io.s_axis.tdata  := io.enqueue.bits.asTypeOf(UInt(89.W))
  io.enqueue.ready                 := fetch_queue_raw.io.s_axis.tready
  fetch_queue_raw.io.s_axis.tvalid := io.enqueue.valid

  val fetch_queue_raw_out = Wire(new QueueEntry)

  fetch_queue_raw_out := fetch_queue_raw.io.m_axis.tdata.asTypeOf(new QueueEntry)
  val dma_read = Wire(new DmaReadReq)

  dma_read.pcie_addr := fetch_queue_raw_out.dbuffered
  dma_read.ram_sel   := 0.U
  dma_read.ram_addr  := (CACHE.line_size * fetch_queue_raw_out.dbuff_id) + fetch_queue_raw_out.dbuffered
  dma_read.len       := io.fetchSize
  dma_read.tag       := tag // TODO
  dma_read.port      := 1.U // TODO placeholder

  io.dequeue.bits    := dma_read

  fetch_queue_raw.io.m_axis.tready := io.dequeue.ready
  io.dequeue.valid                 := fetch_queue_raw.io.m_axis.tvalid

  io.readcmpl.ready := io.notifout.ready
  io.notifout.valid := io.readcmpl.valid

  val pendTag = tagMem.read(io.readcmpl.bits.tag)

  io.notifout.bits.rpc_id     := 0.U
  io.notifout.bits.dbuff_id   := pendTag.queue.dbuff_id
  io.notifout.bits.remaining  := 0.U
  val notif_offset = Wire(UInt(20.W))
  // TODO granted is total number of bytes in fetch queue, bad naming
  notif_offset := pendTag.queue.granted - (pendTag.dma.ram_addr - (16384.U * pendTag.queue.dbuff_id)) 
  when (notif_offset < io.fetchSize) {
    io.notifout.bits.dbuffered  := 0.U
  }.otherwise {
    io.notifout.bits.dbuffered  := notif_offset - io.fetchSize
  }
 
  io.notifout.bits.granted    := 0.U
  io.notifout.bits.priority   := queue_priority.DBUFF_UPDATE.asUInt
 
  // If a read request is lodged then we store it in the memory until completiton
  when(io.dequeue.fire) {
    val tagEntry = Wire(new ReqMem)
    tagEntry.dma   := io.dequeue.bits
    tagEntry.queue := fetch_queue_raw_out

    tagMem.write(tag, tagEntry)

    tag := tag + 1.U(8.W)
  }
}

class SendmsgQueue extends Module {
  val io = IO(new Bundle {
    val fetchSize = Input(UInt(16.W))

    val enqueue = Flipped(Decoupled(new QueueEntry))
    // TODO this is temporary
    val dequeue = Decoupled(new QueueEntry)
  })

  val send_queue_raw = Module(new srpt_queue("sendmsg"))

  send_queue_raw.io.CACHE_BLOCK_SIZE <> io.fetchSize

  // val sendmsg_out_ila = Module(new ILA(Flipped(new axis(89, false, 0, false, 0, false, 0, false))))
  // sendmsg_out_ila.io.ila_data := send_queue_raw.io.m_axis

  // val sendmsg_in_ila = Module(new ILA(new axis(89, false, 0, false, 0, false, 0, false)))
  // sendmsg_in_ila.io.ila_data  := send_queue_raw.io.s_axis

  send_queue_raw.io.clk := clock
  send_queue_raw.io.rst := reset.asUInt

  val raw_in  = io.enqueue.bits.asTypeOf(UInt(89.W))
  val raw_out = Wire(new QueueEntry)

  raw_out := send_queue_raw.io.m_axis.tdata.asTypeOf(new QueueEntry)

  io.enqueue.ready                := send_queue_raw.io.s_axis.tready
  send_queue_raw.io.s_axis.tvalid := io.enqueue.valid
  send_queue_raw.io.s_axis.tdata  := raw_in

  send_queue_raw.io.m_axis.tready := io.dequeue.ready
  io.dequeue.valid                := send_queue_raw.io.m_axis.tvalid
  io.dequeue.bits                 := raw_out
}

/* SegmentedRAM - interface over black box for automatic clock and
 * reset inference.
 *   SIZE - Number of bytes of storage
 */
class SegmentedRam (SIZE: Int) extends Module {
  val io = IO(new Bundle {
    val ram_wr = Flipped(new ram_wr_t(1))
    val ram_rd = Flipped(new ram_rd_t(1))
  })

  val psdpram = Module(new dma_psdpram(SIZE))

  psdpram.io.clk := clock
  psdpram.io.rst := reset.asUInt

  psdpram.io.ram_wr <> io.ram_wr
  psdpram.io.ram_rd <> io.ram_rd
}

/* dma_psdpram - blackbox for segmented memory RTL library
 *   SIZE - Number of bytes of storage
 */
class dma_psdpram (SIZE: Int) extends BlackBox(
  Map("SIZE" -> SIZE,
      "SEG_COUNT" -> 2,
      "SEG_DATA_WIDTH" -> 512,
      "SEG_BE_WIDTH" -> 64,
      "SEG_ADDR_WIDTH" -> 11,
      "PIPELINE" -> 2
  )) with HasBlackBoxResource {
  val io = IO(new Bundle {
    val clk = Input(Clock())
    val rst = Input(UInt(1.W))

    val ram_wr = Flipped(new ram_wr_t(1))
    val ram_rd = Flipped(new ram_rd_t(1))
  })
  addResource("/verilog_libs/pcie/dma_psdpram.v")
}

/* axi2axis_rtl - converts AXI requests into AXI stream requests
 */
class axi2axis extends BlackBox (
  Map("C_S_AXI_ID_WIDTH" -> 4,
      "C_S_AXI_DATA_WIDTH" -> 512,
      "C_S_AXI_ADDR_WIDTH" -> 26 
  )) {

  val io = IO(new Bundle {
    val s_axi_aclk    = Input(Clock())
    val s_axi_aresetn = Input(Reset())
    val s_axi         = Flipped(new axi(512, 26, true, 8, true, 4, true, 4))
    val m_axis        = new axis(512, false, 0, true, 32, false, 0, false)
  })
}


// TODO move me
class dma_read_desc_raw (ports: Int) extends Bundle {
  val pcie_addr = Output(UInt((ports * 64).W))
  val ram_sel   = Output(UInt((ports * 2).W))
  val ram_addr  = Output(UInt((ports * 18).W))
  val len       = Output(UInt((ports * 16).W))
  val tag       = Output(UInt((ports * 8).W))
  val valid     = Output(UInt((ports * 1).W))
  val ready     = Input(UInt((ports * 1).W))
}

class dma_write_desc_raw (ports: Int) extends Bundle {
  val pcie_addr = Output(UInt((ports * 64).W))
  val ram_sel   = Output(UInt((ports * 2).W))
  val ram_addr  = Output(UInt((ports * 18).W))
  val len       = Output(UInt((ports * 16).W))
  val tag       = Output(UInt((ports * 8).W))
  val valid     = Output(UInt((ports * 1).W))
  val ready     = Input(UInt((ports * 1).W))
}

class dma_read_desc_status_raw (ports: Int) extends Bundle {
  val tag   = UInt((ports * 8).W)
  val error = UInt((ports * 4).W)
  val valid = Output(UInt((ports * 1).W))
  val ready = Input(UInt((ports * 1).W))
}

class dma_write_desc_status_raw (ports: Int) extends Bundle {
  val tag   = UInt((ports * 8).W)
  val error = UInt((ports * 4).W)
  val valid = Output(UInt((ports * 1).W))
  val ready = Input(UInt((ports * 1).W))
}

/* pcie_rtl - third party IP to manage the AMD Ultrascale+ block PCIe
 */
class pcie_rtl extends BlackBox (
  Map("AXIS_PCIE_DATA_WIDTH" -> 512,
      "AXIS_PCIE_KEEP_WIDTH" -> 16,
      "AXIS_PCIE_RC_USER_WIDTH" -> 161,
      "AXIS_PCIE_RQ_USER_WIDTH" -> 137,
      "AXIS_PCIE_CQ_USER_WIDTH" -> 183,
      "AXIS_PCIE_CC_USER_WIDTH" -> 81,
      "RC_STRADDLE" -> 1,
      "RQ_STRADDLE" -> 1,
      "CQ_STRADDLE" -> 1,
      "CC_STRADDLE" -> 1,
      "RQ_SEQ_NUM_WIDTH" -> 6,
      "RQ_SEQ_NUM_ENABLE" -> 1,
      "IMM_ENABLE" -> 0,
      "IMM_WIDTH" -> 32,
      "PCIE_TAG_COUNT" -> 256,
      "READ_OP_TABLE_SIZE" -> 256,
      "READ_TX_LIMIT" -> 32,
      "READ_CPLH_FC_LIMIT" -> 256,
      "READ_CPLD_FC_LIMIT" -> 1792,
      "WRITE_OP_TABLE_SIZE" -> 32,
      "WRITE_TX_LIMIT" -> 32,
      "TLP_DATA_WIDTH" -> 512,
      "TLP_STRB_WIDTH" -> 16,
      "TLP_HDR_WIDTH" -> 128,
      "TLP_SEG_COUNT" -> 1,
      "TX_SEQ_NUM_COUNT" -> 2,
      "TX_SEQ_NUM_WIDTH" -> 5,
      "TX_SEQ_NUM_ENABLE" -> 1,
      "PF_COUNT" -> 1,
      "VF_COUNT" -> 0,
      "F_COUNT" -> 1,
      "TLP_FORCE_64_BIT_ADDR" -> 0,
      "CHECK_BUS_NUMBER" -> 0,
      "RAM_SEL_WIDTH" -> 2,
      "RAM_ADDR_WIDTH" -> 18,
      "RAM_SEG_COUNT" -> 2,
      "RAM_SEG_DATA_WIDTH" -> 512,
      "RAM_SEG_BE_WIDTH" -> 64,
      "RAM_SEG_ADDR_WIDTH" -> 11,
      "AXI_DATA_WIDTH" -> 512,
      "AXI_STRB_WIDTH" -> 64,
      "AXI_ADDR_WIDTH" -> 26,
      "AXI_ID_WIDTH" -> 8,
      "PCIE_ADDR_WIDTH" -> 64,
      "LEN_WIDTH" -> 16,
      "TAG_WIDTH" -> 8,
      "PORTS" -> 2,
      "S_TAG_WIDTH" -> 8,
      "M_TAG_WIDTH" -> 9,
      "S_RAM_SEL_WIDTH" -> 2,
      "M_RAM_SEL_WIDTH" -> 3 
  )) {

  val io = IO(new Bundle {
    val pcie_rx_p             = Input(UInt(16.W))
    val pcie_rx_n             = Input(UInt(16.W))
    val pcie_tx_p             = Output(UInt(16.W))
    val pcie_tx_n             = Output(UInt(16.W))
    val pcie_refclk_p         = Input(Clock())
    val pcie_refclk_n         = Input(Clock())
    val pcie_reset_n          = Input(UInt(1.W))

    val pcie_user_clk         = Output(Clock())
    val pcie_user_reset       = Output(UInt(1.W))
			  
    val m_axi                 = new axi(512, 26, true, 8, true, 4, true, 4)

    val dma_read_desc         = Flipped(new dma_read_desc_raw(2))
    val dma_read_desc_status  = new dma_read_desc_status_raw(2)
    val dma_write_desc        = Flipped(new dma_write_desc_raw(2))
    val dma_write_desc_status = new dma_write_desc_status_raw(2)

    val ram_rd                = new ram_rd_t(2)
    val ram_wr                = new ram_wr_t(2)
  })
}

// TODO ports should be passed down all the way to RTL core. In general the RTL core should be more parameterizable from the top level chisel
class PCIe (PORTS: Int) extends Module {
  val io = IO(new Bundle {
    val pcie_rx_p       = Input(UInt(16.W))
    val pcie_rx_n       = Input(UInt(16.W))
    val pcie_tx_p       = Output(UInt(16.W))
    val pcie_tx_n       = Output(UInt(16.W))
    val pcie_refclk_p   = Input(Clock())
    val pcie_refclk_n   = Input(Clock())
    val pcie_reset_n    = Input(UInt(1.W))

    val pcie_user_clk   = Output(Clock())
    val pcie_user_reset = Output(UInt(1.W))
			  
    val m_axi        = new axi(512, 26, true, 8, true, 4, true, 4)

    val dmaReadReq   = Vec(PORTS, Flipped(Decoupled(new DmaReadReq)))
    val dmaReadStat  = Vec(PORTS, Decoupled(new DmaReadStat))

    val dmaWriteReq  = Vec(PORTS, Flipped(Decoupled(new DmaWriteReq)))

    val ram_rd = Vec(PORTS, new ram_rd_t(1))
    val ram_wr = Vec(PORTS, new ram_wr_t(1))
  })

  val pcie_core = Module(new pcie_rtl)

  pcie_core.io.pcie_rx_p := io.pcie_rx_p
  pcie_core.io.pcie_rx_n := io.pcie_rx_n
  io.pcie_tx_p := pcie_core.io.pcie_tx_p
  io.pcie_tx_n := pcie_core.io.pcie_tx_n

  pcie_core.io.pcie_refclk_p := io.pcie_refclk_p
  pcie_core.io.pcie_refclk_n := io.pcie_refclk_n
  pcie_core.io.pcie_reset_n  := io.pcie_reset_n

  io.pcie_user_clk   := pcie_core.io.pcie_user_clk
  io.pcie_user_reset := pcie_core.io.pcie_user_reset

  io.m_axi <> pcie_core.io.m_axi

  def vec2dblwidth[T <: Bundle](source: Vec[T], dest: T) = {
    // Iterate through each interface tagging it with an index
    source.zipWithIndex.foreach { case (interface, index) =>
      // Iterate through each bus in each interface
      interface.elements.foreach { case (name, data) =>
        data <> dest.elements(name).asTypeOf(Vec(PORTS, UInt(data.getWidth.W)))(index)
      }
    }

    // Iterating through double width buses
    dest.elements.foreach { case (name, data) =>
      // Map to each interface
      val inter = source.toSeq.map(interface => {
        interface.elements(name)
      })
      data <> VecInit(inter).asTypeOf(chiselTypeOf(data))
    }
  }

  // TODO functionize this

  vec2dblwidth(io.ram_rd, pcie_core.io.ram_rd)
  vec2dblwidth(io.ram_wr, pcie_core.io.ram_wr)

  io.dmaReadReq.zipWithIndex.foreach { case (interface, index) =>
    interface.ready := pcie_core.io.dma_read_desc.ready.asTypeOf(Vec(PORTS, UInt(interface.ready.getWidth.W)))(index)
  }

  pcie_core.io.dma_read_desc.elements.foreach { case (name, data) =>
    if (name != "valid" && name != "ready") {
      // Map to each interface
      val inter = io.dmaReadReq.toSeq.map(interface => {
        interface.bits.elements(name)
      })
      data := VecInit(inter).asTypeOf(chiselTypeOf(data))
    }
  }

  val readValids = io.dmaReadReq.toSeq.map(interface => {
    interface.elements("valid")
  })
  pcie_core.io.dma_read_desc.valid := VecInit(readValids).asTypeOf(chiselTypeOf(pcie_core.io.dma_read_desc.valid))



  io.dmaWriteReq.zipWithIndex.foreach { case (interface, index) =>
    interface.ready := pcie_core.io.dma_write_desc.ready.asTypeOf(Vec(PORTS, UInt(interface.ready.getWidth.W)))(index)
  }

  pcie_core.io.dma_write_desc.elements.foreach { case (name, data) =>
    if (name != "valid" && name != "ready") {
      // Map to each interface
      val inter = io.dmaWriteReq.toSeq.map(interface => {
        interface.bits.elements(name)
      })
      data := VecInit(inter).asTypeOf(chiselTypeOf(data))
    }
  }

  val writeValids = io.dmaWriteReq.toSeq.map(interface => {
    interface.elements("valid")
  })
  pcie_core.io.dma_write_desc.valid := VecInit(writeValids).asTypeOf(chiselTypeOf(pcie_core.io.dma_write_desc.valid))





  // Iterate through each interface tagging it with an index
  io.dmaReadStat.zipWithIndex.foreach { case (interface, index) =>
    // Iterate through each bus in each interface
    interface.bits.elements.foreach { case (name, data) =>
      if (name != "valid" && name != "ready") {
        data := pcie_core.io.dma_read_desc_status.elements(name).asTypeOf(Vec(PORTS, UInt(data.getWidth.W)))(index)
      }
    }
  }

  io.dmaReadStat.zipWithIndex.foreach { case (interface, index) =>
    interface.valid:= pcie_core.io.dma_read_desc_status.valid.asTypeOf(Vec(PORTS, UInt(interface.ready.getWidth.W)))(index)
  }

  val readStatReadys = io.dmaReadStat.toSeq.map(interface => {
    interface.elements("ready")
  })

  pcie_core.io.dma_read_desc_status.ready := VecInit(readStatReadys).asTypeOf(chiselTypeOf(pcie_core.io.dma_read_desc_status.ready))

  pcie_core.io.dma_write_desc_status := DontCare
  // pcie_core.io.dma_read_desc_status := DontCare
}
