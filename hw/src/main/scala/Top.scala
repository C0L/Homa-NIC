package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

/* top - entry point to the design. Composes the pcie hierarchy and
 * core user logic together.
 */
class Top extends RawModule {
  /* pcie lanes */ 
  val pcie_rx_p     = IO(Input(UInt(16.W)))
  val pcie_rx_n     = IO(Input(UInt(16.W)))
  val pcie_tx_p     = IO(Output(UInt(16.W)))
  val pcie_tx_n     = IO(Output(UInt(16.W)))

  /* pcie reference clock and reset */
  val pcie_refclk_p = IO(Input(Clock()))
  val pcie_refclk_n = IO(Input(Clock()))
  val pcie_reset_n  = IO(Input(Bool()))

  /* 300 MHz Input Clock */
  val clk_in1_n = IO(Input(Clock()))
  val clk_in1_p = IO(Input(Clock()))
  val resetn    = IO(Input(Bool()))

  /* primary clock of user logic */
  val mainClk  = Module(new ClockWizard)
  mainClk.io.clk_in1_n := clk_in1_n
  mainClk.io.clk_in1_p := clk_in1_p
  mainClk.io.reset     := !resetn

  /* synchronizes resets to slowest clock */
  val ps_reset = Module(new ProcessorSystemReset)
  ps_reset.io.ext_reset_in         := !resetn
  ps_reset.io.aux_reset_in         := false.B
  ps_reset.io.slowest_sync_clk     := mainClk.io.clk_out1
  ps_reset.io.dcm_locked           := mainClk.io.locked
  ps_reset.io.mb_reset             := DontCare
  ps_reset.io.bus_struct_reset     := DontCare
  ps_reset.io.interconnect_aresetn := DontCare
  ps_reset.io.peripheral_aresetn   := DontCare
  ps_reset.io.mb_debug_sys_rst     := 0.U

  /* core logic operates at 250MHz output from clock generator */
  val core = withClockAndReset(mainClk.io.clk_out1, ps_reset.io.peripheral_reset) { Module(new MgmtCore) }

  val pcie_user_clk   = Wire(Clock())
  val pcie_user_reset = Wire(Bool())

  /* pcie logic operates at 250MHz output from internal pcie core */
  val pcie = withClockAndReset(pcie_user_clk, pcie_user_reset) { Module(new pcie_core) }

  pcie_user_clk   := pcie.pcie_user_clk
  pcie_user_reset := pcie.pcie_user_reset

  pcie.pcie_rx_p     := pcie_rx_p
  pcie.pcie_rx_n     := pcie_rx_n
  pcie_tx_p          := pcie.pcie_tx_p 
  pcie_tx_n          := pcie.pcie_tx_n
  pcie.pcie_refclk_p := pcie_refclk_p
  pcie.pcie_refclk_n := pcie_refclk_n
  pcie.pcie_reset_n  := pcie_reset_n

  /* Clock Convert core (200MHz) -> pcie (250MHz) */

  def core2pcie(source: DecoupledIO[Data], dest: DecoupledIO[Data]) = {
    val cc  = Module(new AXISClockConverter(chiselTypeOf((source: DecoupledIO[Data]).bits)))
    source <> cc.io.s_axis
    cc.io.s_axis_aresetn  := !ps_reset.io.peripheral_reset.asBool
    cc.io.s_axis_aclk     := mainClk.io.clk_out1
    cc.io.m_axis_aresetn  := !pcie.pcie_user_reset
    cc.io.m_axis_aclk     := pcie.pcie_user_clk
    cc.io.m_axis          <> dest
  }

  def pcie2core(source: DecoupledIO[Data], dest: DecoupledIO[Data]) = {
    val cc  = Module(new AXISClockConverter(chiselTypeOf((source: DecoupledIO[Data]).bits)))
    source <> cc.io.s_axis
    cc.io.s_axis_aresetn := !pcie.pcie_user_reset
    cc.io.s_axis_aclk    := pcie.pcie_user_clk
    cc.io.m_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
    cc.io.m_axis_aclk    := mainClk.io.clk_out1
    cc.io.m_axis         <> dest

  }

  core2pcie(core.io.h2cPayloadRamReq, pcie.h2cPayloadRamReq)
  core2pcie(core.io.c2hPayloadRamReq, pcie.c2hPayloadRamReq)
  core2pcie(core.io.c2hMetadataRamReq, pcie.c2hMetadataRamReq)
  core2pcie(core.io.h2cPayloadDmaReq, pcie.h2cPayloadDmaReq)
  core2pcie(core.io.c2hPayloadDmaReq, pcie.c2hPayloadDmaReq)
  core2pcie(core.io.c2hMetadataDmaReq, pcie.c2hMetadataDmaReq)

  pcie2core(pcie.h2cPayloadRamResp, core.io.h2cPayloadRamResp)
  pcie2core(pcie.h2cPayloadDmaStat, core.io.h2cPayloadDmaStat)

  /* Clock Convert pcie (250MHz) -> core (200MHz) */

  val axi_cc    = Module(new AXIClockConverter(512, 26, true, 8, true, 4, true, 4))

  axi_cc.io.s_axi         <> pcie.m_axi
  axi_cc.io.s_axi_aresetn := !pcie.pcie_user_reset
  axi_cc.io.s_axi_aclk    := pcie.pcie_user_clk
  axi_cc.io.m_axi_aresetn := !ps_reset.io.peripheral_reset.asBool
  axi_cc.io.m_axi_aclk    := mainClk.io.clk_out1
  axi_cc.io.m_axi         <> core.io.s_axi

 // withClockAndReset(mainClk.io.clk_out1, ps_reset.io.peripheral_reset) {
 //   val h2cPayloadRamReq_ila = Module(new ILA(new RamReadReq))
 //   h2cPayloadRamReq_ila.io.ila_data := core.io.h2cPayloadRamReq.bits

 //   val c2hPayloadRamReq_ila = Module(new ILA(new RamWriteReq))
 //   c2hPayloadRamReq_ila.io.ila_data := core.io.c2hPayloadRamReq.bits

 //   val c2hMetadataRamReq_ila = Module(new ILA(new RamWriteReq))
 //   c2hMetadataRamReq_ila.io.ila_data := core.io.c2hMetadataRamReq.bits

 //   val h2cPayloadDmaReq_ila = Module(new ILA(new DmaReq))
 //   h2cPayloadDmaReq_ila.io.ila_data := core.io.h2cPayloadDmaReq.bits

 //   val h2cPayloadDmaStat_ila = Module(new ILA(new DmaReadStat))
 //   h2cPayloadDmaStat_ila.io.ila_data := core.io.h2cPayloadDmaStat.bits

 //   val c2hPayloadDmaReq_ila = Module(new ILA(new DmaReq))
 //   c2hPayloadDmaReq_ila.io.ila_data := core.io.c2hPayloadDmaReq.bits

 //   val c2hMetadataDmaReq_ila = Module(new ILA(new DmaReq))
 //   c2hMetadataDmaReq_ila.io.ila_data := core.io.c2hMetadataDmaReq.bits
 // }
}

/* Main - generate the system verilog code of the entire design (top.sv)
 */
object Main extends App {
  ChiselStage.emitSystemVerilogFile(
    new Top,
    Array("--split-verilog", "--target-dir", "gen"),
    Array("--disable-all-randomization", "--strip-debug-info"),
  )
}
