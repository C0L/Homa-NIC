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

  /* Clock Convert core (200MHz) -> pcie (250MHz) */
  val ram_read_desc_cc  = Module(new AXISClockConverter(new ram_read_desc_t))
  val ram_write_desc_cc = Module(new AXISClockConverter(new ram_write_desc_t))
  val ram_write_data_cc = Module(new AXISClockConverter(new ram_write_data_t))
  val dma_read_desc_cc  = Module(new AXISClockConverter(new dma_read_desc_t))
  val dma_write_desc_cc = Module(new AXISClockConverter(new dma_write_desc_t))

  core.io.ram_read_desc              <> ram_read_desc_cc.io.s_axis
  ram_read_desc_cc.io.s_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  ram_read_desc_cc.io.s_axis_aclk    := mainClk.io.clk_out1
  ram_read_desc_cc.io.m_axis_aresetn := !pcie.pcie_user_reset
  ram_read_desc_cc.io.m_axis_aclk    := pcie.pcie_user_clk
  ram_read_desc_cc.io.m_axis         <> pcie.ram_read_desc

  core.io.ram_write_desc              <> ram_write_desc_cc.io.s_axis
  ram_write_desc_cc.io.s_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  ram_write_desc_cc.io.s_axis_aclk    := mainClk.io.clk_out1
  ram_write_desc_cc.io.m_axis_aresetn := !pcie.pcie_user_reset
  ram_write_desc_cc.io.m_axis_aclk    := pcie.pcie_user_clk
  ram_write_desc_cc.io.m_axis         <> pcie.ram_write_desc

  core.io.ram_write_data                 <> ram_write_data_cc.io.s_axis
  ram_write_data_cc.io.s_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  ram_write_data_cc.io.s_axis_aclk    := mainClk.io.clk_out1
  ram_write_data_cc.io.m_axis_aresetn := !pcie.pcie_user_reset
  ram_write_data_cc.io.m_axis_aclk    := pcie.pcie_user_clk
  ram_write_data_cc.io.m_axis         <> pcie.ram_write_data

  core.io.dma_read_desc              <> dma_read_desc_cc.io.s_axis
  dma_read_desc_cc.io.s_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  dma_read_desc_cc.io.s_axis_aclk    := mainClk.io.clk_out1
  dma_read_desc_cc.io.m_axis_aresetn := !pcie.pcie_user_reset
  dma_read_desc_cc.io.m_axis_aclk    := pcie.pcie_user_clk
  dma_read_desc_cc.io.m_axis         <> pcie.dma_read_desc

  core.io.dma_write_desc             <> dma_write_desc_cc.io.s_axis
  dma_write_desc_cc.io.s_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  dma_write_desc_cc.io.s_axis_aclk    := mainClk.io.clk_out1
  dma_write_desc_cc.io.m_axis_aresetn := !pcie.pcie_user_reset
  dma_write_desc_cc.io.m_axis_aclk    := pcie.pcie_user_clk
  dma_write_desc_cc.io.m_axis         <> pcie.dma_write_desc


  /* Clock Convert pcie (250MHz) -> core (200MHz) */
  val ram_read_desc_status_cc  = Module(new AXISClockConverter(new ram_read_desc_status_t))
  val ram_read_data_cc         = Module(new AXISClockConverter(new ram_read_data_t))
  val ram_write_desc_status_cc = Module(new AXISClockConverter(new ram_write_desc_status_t))
  val dma_read_desc_status_cc  = Module(new AXISClockConverter(new dma_read_desc_status_t))
  val dma_write_desc_status_cc = Module(new AXISClockConverter(new dma_write_desc_status_t))

  pcie.ram_read_desc_status                 <> ram_read_desc_status_cc.io.s_axis
  ram_read_desc_status_cc.io.s_axis_aresetn := !pcie.pcie_user_reset
  ram_read_desc_status_cc.io.s_axis_aclk    := pcie.pcie_user_clk
  ram_read_desc_status_cc.io.m_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  ram_read_desc_status_cc.io.m_axis_aclk    := mainClk.io.clk_out1
  ram_read_desc_status_cc.io.m_axis         <> core.io.ram_read_desc_status


  pcie.ram_read_data <> ram_read_data_cc.io.s_axis
  ram_read_data_cc.io.s_axis_aresetn := !pcie.pcie_user_reset
  ram_read_data_cc.io.s_axis_aclk    := pcie.pcie_user_clk
  ram_read_data_cc.io.m_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  ram_read_data_cc.io.m_axis_aclk    := mainClk.io.clk_out1
  ram_read_data_cc.io.m_axis         <> core.io.ram_read_data


  pcie.ram_write_desc_status                 <> ram_write_desc_status_cc.io.s_axis
  ram_write_desc_status_cc.io.s_axis_aresetn := !pcie.pcie_user_reset
  ram_write_desc_status_cc.io.s_axis_aclk    := pcie.pcie_user_clk
  ram_write_desc_status_cc.io.m_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  ram_write_desc_status_cc.io.m_axis_aclk    := mainClk.io.clk_out1
  ram_write_desc_status_cc.io.m_axis         <> core.io.ram_write_desc_status


  pcie.dma_read_desc_status                 <> dma_read_desc_status_cc.io.s_axis
  dma_read_desc_status_cc.io.s_axis_aresetn := !pcie.pcie_user_reset
  dma_read_desc_status_cc.io.s_axis_aclk    := pcie.pcie_user_clk
  dma_read_desc_status_cc.io.m_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  dma_read_desc_status_cc.io.m_axis_aclk    := mainClk.io.clk_out1
  dma_read_desc_status_cc.io.m_axis         <> core.io.dma_read_desc_status


  pcie.dma_write_desc_status                 <> dma_write_desc_status_cc.io.s_axis
  dma_write_desc_status_cc.io.s_axis_aresetn := !pcie.pcie_user_reset
  dma_write_desc_status_cc.io.s_axis_aclk    := pcie.pcie_user_clk
  dma_write_desc_status_cc.io.m_axis_aresetn := !ps_reset.io.peripheral_reset.asBool
  dma_write_desc_status_cc.io.m_axis_aclk    := mainClk.io.clk_out1
  dma_write_desc_status_cc.io.m_axis         <> core.io.dma_write_desc_status

  /* Clock Convert pcie (250MHz) -> core (200MHz) */
  val axi_cc    = Module(new AXIClockConverter(512, 26, true, 8, true, 4, true, 4))

  axi_cc.io.s_axi         <> pcie.m_axi
  axi_cc.io.s_axi_aresetn := !pcie.pcie_user_reset
  axi_cc.io.s_axi_aclk    := pcie.pcie_user_clk
  axi_cc.io.m_axi_aresetn := !ps_reset.io.peripheral_reset.asBool
  axi_cc.io.m_axi_aclk    := mainClk.io.clk_out1
  axi_cc.io.m_axi         <> core.io.s_axi

  withClockAndReset(mainClk.io.clk_out1, ps_reset.io.peripheral_reset) {
  val ram_read_desc_ila  = Module(new ILA(new ram_read_desc_t))
  ram_read_desc_ila.io.ila_data := core.io.ram_read_desc.bits
  val ram_write_desc_ila = Module(new ILA(new ram_write_desc_t))
  ram_write_desc_ila.io.ila_data := core.io.ram_write_desc.bits
  //val ram_write_data_ila = Module(new ILA(new ram_write_data_t))
  //ram_write_data_ila.io.ila_data := core.io.ram_write_data.bits
  val dma_read_desc_ila  = Module(new ILA(new dma_read_desc_t))
  dma_read_desc_ila.io.ila_data := core.io.dma_read_desc.bits
  val dma_write_desc_ila = Module(new ILA(new dma_write_desc_t))
  dma_write_desc_ila.io.ila_data := core.io.dma_write_desc.bits

  // val ram_read_desc_status_ila  = Module(new ILA(new ram_read_desc_status_t))
  // ram_read_desc_status_ila.io.ila_data := core.io.ram_read_desc_status.bits
  val ram_read_data_ila         = Module(new ILA(new ram_read_data_t))
  ram_read_data_ila.io.ila_data := core.io.ram_read_data.bits
  val ram_write_desc_status_ila = Module(new ILA(new ram_write_desc_status_t))
  ram_write_desc_status_ila.io.ila_data := core.io.ram_write_desc_status.bits
  // val dma_read_desc_status_ila  = Module(new ILA(new dma_read_desc_status_t))
  // dma_read_desc_status_ila.io.ila_data := core.io.dma_read_desc_status.bits
  // val dma_write_desc_status_ila = Module(new ILA(new dma_write_desc_status_t))
  // dma_write_desc_status_ila.io.ila_data := core.io.dma_write_desc_status.bits
  }
}

/* Main - generate the system verilog code of the entire design (top.sv)
 */
object Main extends App {
  ChiselStage.emitSystemVerilogFile(
    new top,
    Array("--split-verilog", "--target-dir", "gen"),
    Array("--disable-all-randomization", "--strip-debug-info"),
  )
}
