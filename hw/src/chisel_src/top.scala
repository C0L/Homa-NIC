package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

class top extends Module {
  val pcie_rx_p             = IO(Input(UInt(16.W)))
  val pcie_rx_n             = IO(Input(UInt(16.W)))
  val pcie_tx_p             = IO(Output(UInt(16.W)))
  val pcie_tx_n             = IO(Output(UInt(16.W)))
  val pcie_refclk_p         = IO(Input(Clock()))
  val pcie_refclk_n         = IO(Input(Clock()))
  val pcie_reset_n          = IO(Input(Reset()))

  /* Use default clock domain */
  val axi2axis   = Module(new axi2axis) // Convert incoming AXI requests to AXIS
  val delegate   = Module(new delegate) // Decide where those requests should be placed
  val fetch_cc   = Module(new axis_cc(new dma_read_t)) // TODO replace with asyncqueue
  val fetch_queue   = Module(new fetch_queue)    // Fetch the next best chunk of data
  val sendmsg_queue = Module(new sendmsg_queue)  // Send the next best message 

  /* Uses internal clock domain */
  val addr_map = Module(new addr_map) // Map read and write requests to DMA address
  val h2c_dma  = Module(new h2c_dma)  // Manage dma reads and writes TODO maybe redundant now
  val pcie     = Module(new pcie)     // pcie infrastructure

  // TODO Maybe RR is not the best strategy
  val fetch_arbiter = Module(new RRArbiter(new queue_entry_t, 2))
  fetch_arbiter.io.in(0) <> delegate.io.fetchdata_o
  fetch_arbiter.io.in(1) <> h2c_dma.io.dbuff_notif_o
  fetch_arbiter.io.out   <> fetch_queue.io.enqueue
  // h2c_dma.io.dbuff_notif_o := DontCare

    // consumer.io.in <> arb.io.out

  pcie.pcie_rx_p     := pcie_rx_p
  pcie.pcie_rx_n     := pcie_rx_p
  pcie_tx_p          := pcie.pcie_tx_p 
  pcie_tx_n          := pcie.pcie_tx_n
  pcie.pcie_refclk_p := pcie_refclk_p
  pcie.pcie_refclk_n := pcie_refclk_p
  pcie.pcie_reset_n  := pcie_reset_n

  /* Eventually from packet processing pipeline */
  pcie.ram_read_desc         := DontCare
  pcie.ram_read_desc_status  := DontCare
  pcie.ram_read_data         := DontCare

  pcie.ram_write_desc        := DontCare
  pcie.ram_write_data        := DontCare
  pcie.ram_write_desc_status := DontCare

  /* Eventually from c2h_dma */
  pcie.dma_write_desc        := DontCare
  pcie.dma_write_desc_status := DontCare

  axi2axis.io.s_axi  <> pcie.m_axi
  axi2axis.io.m_axis <> delegate.io.function_i

  delegate.io.dma_map_o   <> addr_map.io.dma_map_i    // TODO eventually shared
  delegate.io.sendmsg_o   <> sendmsg_queue.io.enqueue // TODO eventually shared
  // delegate.io.fetchdata_o <> fetch_arbiter.io.out 

  // TODO placeholder
  delegate.io.dma_w_req_o       := DontCare

  // TODO eventually goes to packet constructor
  sendmsg_queue.io.dequeue      := DontCare

  addr_map.io.dma_w_meta_i      := DontCare
  addr_map.io.dma_w_data_i      := DontCare
  addr_map.io.dma_w_req_o       := DontCare

  // TODO will not be able to infer this
  fetch_queue.io.dequeue     <> fetch_cc.io.s_axis
  fetch_cc.io.s_axis_aresetn := !reset.asBool
  fetch_cc.io.s_axis_aclk    := clock
  fetch_cc.io.m_axis_aresetn := !pcie.pcie_user_reset
  fetch_cc.io.m_axis_aclk    := pcie.pcie_user_clk

  // TODO will not be able to infer this 
  addr_map.io.dma_r_req_i <> fetch_cc.io.m_axis

  addr_map.io.dma_r_req_o <> h2c_dma.io.dma_read_req_i //TODO inconsistent names

  h2c_dma.io.pcie_read_cmd_o    <> pcie.dma_read_desc
  h2c_dma.io.pcie_read_status_i <> pcie.dma_read_desc_status

  val ila = Module(new system_ila(new queue_entry_t))

  // ila.io(0) := DontCare
  ila.io.clk         := clock
  ila.io.resetn      := !reset.asBool
  ila.io.SLOT_0_AXIS := DontCare
}

object Main extends App {
  ChiselStage.emitSystemVerilogFile(new top,
    firtoolOpts = Array("-disable-all-randomization", "-strip-debug-info")
  )
}
