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

class FetchQueue extends Module {
  val io = IO(new Bundle {
    val CACHE_BLOCK_SIZE = Input(UInt(16.W))
    val enqueue = Flipped(Decoupled(new QueueEntry))
    // TODO this is temporary
    val dequeue = Decoupled(new dma_read_t)
  })

  val fetch_queue_raw = Module(new srpt_queue("fetch"))

  fetch_queue_raw.io.clk := clock
  fetch_queue_raw.io.rst := reset.asUInt

  fetch_queue_raw.io.CACHE_BLOCK_SIZE <> io.CACHE_BLOCK_SIZE

  // val fetch_out_ila = Module(new ILA(Flipped(new axis(89, false, 0, false, 0, false, 0, false))))
  // fetch_out_ila.io.ila_data := fetch_queue_raw.io.m_axis

  // val fetch_in_ila = Module(new ILA(new axis(89, false, 0, false, 0, false, 0, false)))
  // fetch_in_ila.io.ila_data := fetch_queue_raw.io.s_axis

  fetch_queue_raw.io.s_axis.tdata  := io.enqueue.bits.asTypeOf(UInt(89.W))
  io.enqueue.ready                 := fetch_queue_raw.io.s_axis.tready
  fetch_queue_raw.io.s_axis.tvalid := io.enqueue.valid

  val fetch_queue_raw_out = Wire(new QueueEntry)

  fetch_queue_raw_out := fetch_queue_raw.io.m_axis.tdata.asTypeOf(new QueueEntry)
  val dma_read = Wire(new dma_read_t)

  dma_read.pcie_read_addr := fetch_queue_raw_out.dbuffered
  dma_read.cache_id       := fetch_queue_raw_out.dbuff_id // TODO bad name 
  dma_read.message_size   := fetch_queue_raw_out.granted
  dma_read.read_len       := 256.U // TODO placeholder
  dma_read.dest_ram_addr  := (16384.U * fetch_queue_raw_out.dbuff_id) + fetch_queue_raw_out.dbuffered
  dma_read.port           := 1.U // TODO placeholder

  io.dequeue.bits         := dma_read

  fetch_queue_raw.io.m_axis.tready := io.dequeue.ready
  io.dequeue.valid                 := fetch_queue_raw.io.m_axis.tvalid
}

class SendmsgQueue extends Module {
  val io = IO(new Bundle {
    val CACHE_BLOCK_SIZE = Input(UInt(16.W))

    val enqueue = Flipped(Decoupled(new QueueEntry))
    // TODO this is temporary
    val dequeue = Decoupled(new QueueEntry)
  })

  val send_queue_raw = Module(new srpt_queue("sendmsg"))

  send_queue_raw.io.CACHE_BLOCK_SIZE <> io.CACHE_BLOCK_SIZE

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
class SegmentedRAM (SIZE: Int) extends Module {
  val io = IO(new Bundle {
    val ram_wr = Flipped(new ram_wr_t)
    val ram_rd = Flipped(new ram_rd_t)
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

    val ram_wr = Flipped(new ram_wr_t)
    val ram_rd = Flipped(new ram_rd_t)
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
      "TAG_WIDTH" -> 8
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

    val dma_read_desc         = Flipped(Decoupled(new dma_read_desc_t))
    val dma_read_desc_status  = Decoupled(new dma_read_desc_status_t)
    val dma_write_desc        = Flipped(Decoupled(new dma_write_desc_t))
    val dma_write_desc_status = Decoupled(new dma_write_desc_status_t)

    val ram_rd                = new ram_rd_t
    val ram_wr                = new ram_wr_t
  })
}
