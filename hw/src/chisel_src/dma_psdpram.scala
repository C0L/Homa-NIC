package gpnic

import chisel3._
import chisel3.util._

class dma_psdpram extends BlackBox(
  Map("SIZE" -> 262144,
      "SEG_COUNT" -> 2,
      "SEG_DATA_WIDTH" -> 512,
      "SEG_BE_WIDTH" -> 64,
      "SEG_ADDR_WIDTH" -> 11,
      "PIPELINE" -> 2
  )) {

  // TODO should parameterize ram interfaces
  val io = IO(new Bundle {
    val clk = Input(Clock())
    val rst = Input(UInt(1.W))

    val ram_wr = Flipped(new ram_wr_t)
    val ram_rd = Flipped(new ram_rd_t)
  })
}
