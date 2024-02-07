package gpnic

import chisel3._
import chisel3.util._

// TODO create wrapper here

class axi2axis extends BlackBox (
  Map("C_S_AXI_ID_WIDTH" -> "4",
      "C_S_AXI_DATA_WIDTH" -> "512",
      "C_S_AXI_ADDR_WIDTH" -> "32"
  )) {

  val io = IO(new Bundle {
    val s_axi   = Flipped(new axi(512, 8, 32))
    val m_axis  = new axis(512, false, 0, true, 32, false)
  })
  // val s_axis  = Flipped(new axis()) // TODO not used for now
}

// class axi2axis extends Module {
  // val 
// }
