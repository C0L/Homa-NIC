package gpnic

import chisel3._
import chisel3.util._

class axi2axis extends BlackBox (
  Map("C_S_AXI_ID_WIDTH" -> "4",
      "C_S_AXI_DATA_WIDTH" -> "512",
      "C_S_AXI_ADDR_WIDTH" -> "32"
  )) {

  val io = IO(new Bundle {
    val s_axi = new axi4(512, 8, 32) //  TODO This needs to be re-parameterized
    val m_axis  = new axis()
    // val s_axis  = Flipped(new axis()) // TODO not used for now
  })
}
