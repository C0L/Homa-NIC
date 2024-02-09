package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

class dma_client_axis_source extends BlackBox(
  Map("RAM_ADDR_WIDTH" -> 18,
      "SEG_COUNT" -> 2,
      "SEG_DATA_WIDTH" -> 512,
      "SEG_BE_WIDTH" -> 64,
      "SEG_ADDR_WIDTH" -> 11,
      "AXIS_DATA_WIDTH" -> 512,
      "AXIS_KEEP_ENABLE" -> 1,
      "AXIS_KEEP_WIDTH" -> 64,
      "AXIS_LAST_ENABLE" -> 1,
      "AXIS_ID_ENABLE" -> 0,
      "AXIS_ID_WIDTH" -> 1,
      "AXIS_DEST_ENABLE" -> 0,
      "AXIS_DEST_WIDTH" -> 8,
      "AXIS_USER_ENABLE" -> 1,
      "AXIS_USER_WIDTH" -> 1,
      "LEN_WIDTH" -> 64,
      "TAG_WIDTH" -> 8
  )) {

  // TODO should parameterize ram interfaces
  val io = IO(new Bundle {
    val clk = Input(Clock())
    val rst = Input(UInt(1.W))

    val ram_read_desc        = Flipped(Decoupled(new ram_read_desc_t))
    val ram_read_desc_status = Decoupled(new ram_read_desc_status_t)
    val ram_read_data        = Decoupled(new ram_read_data_t)

    val ram_rd = new ram_rd_t

    val enable = Input(UInt(1.W))
  })
}

class dma_client_axis_sink extends BlackBox(
  Map("RAM_ADDR_WIDTH" -> 18,
      "SEG_COUNT" -> 2,
      "SEG_DATA_WIDTH" -> 512,
      "SEG_BE_WIDTH" -> 64,
      "SEG_ADDR_WIDTH" -> 11,
      "AXIS_DATA_WIDTH" -> 512,
      "AXIS_KEEP_ENABLE" -> 1,
      "AXIS_KEEP_WIDTH" -> 64,
      "AXIS_LAST_ENABLE" -> 1,
      "AXIS_ID_ENABLE" -> 0,
      "AXIS_ID_WIDTH" -> 1,
      "AXIS_DEST_ENABLE" -> 0,
      "AXIS_DEST_WIDTH" -> 8,
      "AXIS_USER_ENABLE" -> 1,
      "AXIS_USER_WIDTH" -> 1,
      "LEN_WIDTH" -> 64,
      "TAG_WIDTH" -> 8
  )) {

  // TODO should parameterize ram interfaces
  val io = IO(new Bundle {
    val clk = Input(Clock())
    val rst = Input(UInt(1.W))

    val ram_write_desc        = Flipped(Decoupled(new ram_write_desc_t))
    val ram_write_data        = Flipped(Decoupled(new ram_write_data_t))
    val ram_write_desc_status = Decoupled(new ram_write_desc_status_t)

    val ram_wr = new ram_wr_t

    val enable = Input(UInt(1.W))
  })
}

