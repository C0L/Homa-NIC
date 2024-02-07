package gpnic

import chisel3._
import chisel3.util._

class pcie_core extends BlackBox (
  Map("AXIS_PCIE_DATA_WIDTH" -> "512",
      "AXIS_PCIE_KEEP_WIDTH" -> "16",
      "AXIS_PCIE_RC_USER_WIDTH" -> "161",
      "AXIS_PCIE_RQ_USER_WIDTH" -> "137",
      "AXIS_PCIE_CQ_USER_WIDTH" -> "183",
      "AXIS_PCIE_CC_USER_WIDTH" -> "81",
      "RC_STRADDLE" -> "1",
      "RQ_STRADDLE" -> "1",
      "CQ_STRADDLE" -> "1",
      "CC_STRADDLE" -> "1",
      "RQ_SEQ_NUM_WIDTH" -> "6",
      "RQ_SEQ_NUM_ENABLE" -> "1",
      "IMM_ENABLE" -> "0",
      "IMM_WIDTH" -> "32",
      "PCIE_TAG_COUNT" -> "256",
      "READ_OP_TABLE_SIZE" -> "256",
      "READ_TX_LIMIT" -> "32",
      "READ_CPLH_FC_LIMIT" -> "256",
      "READ_CPLD_FC_LIMIT" -> "2048-256",
      "WRITE_OP_TABLE_SIZE" -> "32",
      "WRITE_TX_LIMIT" -> "32",
      "TLP_DATA_WIDTH" -> "512",
      "TLP_STRB_WIDTH" -> "16",
      "TLP_HDR_WIDTH" -> "128",
      "TLP_SEG_COUNT" -> "1",
      "TX_SEQ_NUM_COUNT" -> "2",
      "TX_SEQ_NUM_WIDTH" -> "5",
      "TX_SEQ_NUM_ENABLE" -> "1",
      "PF_COUNT" -> "1",
      "VF_COUNT" -> "0",
      "F_COUNT" -> "1",
      "TLP_FORCE_64_BIT_ADDR" -> "0",
      "CHECK_BUS_NUMBER" -> "0",
      "RAM_SEL_WIDTH" -> "2",
      "RAM_ADDR_WIDTH" -> "18",
      "RAM_SEG_COUNT" -> "2",
      "RAM_SEG_DATA_WIDTH" -> "512",
      "RAM_SEG_BE_WIDTH" -> "64",
      "RAM_SEG_ADDR_WIDTH" -> "11",
      "AXI_DATA_WIDTH" -> "512",
      "AXI_STRB_WIDTH" -> "64",
      "AXI_ADDR_WIDTH" -> "26",
      "AXI_ID_WIDTH" -> "8",
      "PCIE_ADDR_WIDTH" -> "64",
      "LEN_WIDTH" -> "16",
      "TAG_WIDTH" -> "8"
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
			  
    val m_axi                 = new axi(512, 8, 26)
    val dma_read_desc         = Flipped(Decoupled(new dma_read_desc_t))
    val dma_read_desc_status  = Decoupled(new dma_read_desc_status_t)
    val dma_write_desc        = Flipped(Decoupled(new dma_write_desc_t))
    val dma_write_desc_status = Decoupled(new dma_write_desc_status_t)

    val ram_rd                = new ram_rd_t
    val ram_wr                = new ram_wr_t
  })
}
