package gpnic

import chisel3._
import chisel3.util._

// TODO create wrapper here

class axi2axis extends BlackBox (
  Map("C_S_AXI_ID_WIDTH" -> 4,
      "C_S_AXI_DATA_WIDTH" -> 512,
      "C_S_AXI_ADDR_WIDTH" -> 26 
  )) {

  val io = IO(new Bundle {
    val s_axi_aclk    = Input(Clock())
    val s_axi_aresetn = Input(Reset())
    val s_axi         = Flipped(new axi(512, 26, true, 8, true, 4, true, 4))
    val m_axis        = new axis(512, false, 0, true, 32, false)
  })
}
