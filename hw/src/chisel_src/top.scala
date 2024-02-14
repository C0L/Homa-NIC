package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

/* top - entry point to the design. Composes the pcie hierarchy and
 * core user logic together.
 */
class top extends RawModule {
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

  /* core logic operates at 200MHz output from clock generator */
  val core = withClockAndReset(mainClk.io.clk_out1, ps_reset.io.peripheral_reset) { Module(new mgmt_core) }

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

  /* Eventually from packet processing pipeline */
  pcie.ram_read_desc         := DontCare
  pcie.ram_read_desc_status  := DontCare
  pcie.ram_read_data         := DontCare

  /* Eventually from c2h_dma */
  // pcie.dma_write_desc        := DontCare
  // pcie.dma_write_desc_status := DontCare

  // TODO 
  pcie.dma_w_meta_i := DontCare
  pcie.dma_w_data_i := DontCare

  /* Clock Convert core (200MHz) -> pcie (250MHz) */
  val fetch_cc  = Module(new AXISClockConverter(new dma_read_t))

  core.io.fetch_dequeue      <> fetch_cc.io.s_axis
  fetch_cc.io.s_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  fetch_cc.io.s_axis_aclk    := mainClk.io.clk_out1
  fetch_cc.io.m_axis_aresetn := !pcie.pcie_user_reset
  fetch_cc.io.m_axis_aclk    := pcie.pcie_user_clk
  fetch_cc.io.m_axis         <> pcie.dma_r_req_i

  /* Clock Convert pcie (200MHz) -> core (250MHz) */
  val dbnotif_cc = Module(new AXISClockConverter(new queue_entry_t))

  pcie.dbuff_notif_o        <> dbnotif_cc.io.s_axis
  dbnotif_cc.io.s_axis_aresetn := !pcie.pcie_user_reset
  dbnotif_cc.io.s_axis_aclk    := pcie.pcie_user_clk
  dbnotif_cc.io.m_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  dbnotif_cc.io.m_axis_aclk    := mainClk.io.clk_out1
  dbnotif_cc.io.m_axis         <> core.io.dbuff_notif_i

  /* Clock Convert pcie (250MHz) -> pcie (200MHz) */
  val dmamap_cc = Module(new AXISClockConverter(new dma_map_t))

  core.io.dma_map_o           <> dmamap_cc.io.s_axis
  dmamap_cc.io.s_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  dmamap_cc.io.s_axis_aclk    := mainClk.io.clk_out1
  dmamap_cc.io.m_axis_aresetn := !pcie.pcie_user_reset
  dmamap_cc.io.m_axis_aclk    := pcie.pcie_user_clk
  dmamap_cc.io.m_axis         <> pcie.dma_map_i

  /* Clock Convert pcie (250MHz) -> core (200MHz) */
  val axi_cc    = Module(new AXIClockConverter(512, 26, true, 8, true, 4, true, 4))

  axi_cc.io.s_axi <> pcie.m_axi
  axi_cc.io.s_axi_aresetn := !pcie.pcie_user_reset
  axi_cc.io.s_axi_aclk    := pcie.pcie_user_clk
  axi_cc.io.m_axi_aresetn := !ps_reset.io.peripheral_reset.asBool
  axi_cc.io.m_axi_aclk    := mainClk.io.clk_out1
  axi_cc.io.m_axi         <> core.io.s_axi
}

/* Main - generate the system verilog code of the entire design (top.sv)
 */
object Main extends App {
  ChiselStage.emitSystemVerilogFile(new top,
    firtoolOpts = Array("-disable-all-randomization", "-strip-debug-info")
  )
}
