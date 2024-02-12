package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

/* mgmt_core - converts AXI read and write requests from pcie into
 * stream interface, virtualization/delegation module decides where to
 * route these requests. These requests can be routed to the dma map
 * core in pcie to store host map buffers, or the priority queues
 * which schedule operations.
 */
class mgmt_core extends Module {
  val io = IO(new Bundle {
    val dbuff_notif_i = Flipped(Decoupled(new queue_entry_t))
    val fetch_dequeue = Decoupled(new dma_read_t)
    val dma_map_o     = Decoupled(new dma_map_t)
    val s_axi         = Flipped(new axi(512, 26, true, 8, true, 4, true, 4))
  })

  val axi2axis   = Module(new axi2axis) // Convert incoming AXI requests to AXIS
  val delegate   = Module(new delegate) // Decide where those requests should be placed

  val fetch_queue   = Module(new fetch_queue)    // Fetch the next best chunk of data
  val sendmsg_queue = Module(new sendmsg_queue)  // Send the next best message 

  fetch_queue.io.dequeue <> io.fetch_dequeue
  io.dma_map_o           <> delegate.io.dma_map_o

  // Alternate between fetchdata and notifs to the fetch queue
  val fetch_arbiter = Module(new RRArbiter(new queue_entry_t, 2))
  fetch_arbiter.io.in(0) <> delegate.io.fetchdata_o
  fetch_arbiter.io.in(1) <> io.dbuff_notif_i
  fetch_arbiter.io.out   <> fetch_queue.io.enqueue

  // TODO need to throw out read requests
  // axi2axis takes the address of an AXI write and converts it to an
  // AXIS transaction where the user bits store the original address
  axi2axis.io.s_axi         <> io.s_axi
  axi2axis.io.m_axis        <> delegate.io.function_i
  axi2axis.io.s_axi_aclk    <> clock
  axi2axis.io.s_axi_aresetn <> !reset.asBool

  delegate.io.sendmsg_o <> sendmsg_queue.io.enqueue // TODO eventually shared

  // TODO placeholder
  delegate.io.dma_w_req_o       := DontCare

  // TODO eventually goes to packet constructor
  sendmsg_queue.io.dequeue      := DontCare


  /* DEBUGGING ILAS */
  val axi2axis_ila = Module(new ILA(new axis(512, false, 0, true, 32, false)))
  axi2axis_ila.io.ila_data := axi2axis.io.m_axis

  val sendmsg_ila = Module(new ILA(Decoupled(new queue_entry_t)))
  sendmsg_ila.io.ila_data := delegate.io.sendmsg_o

  val axi_ila = Module(new ILA(new axi(512, 26, true, 8, true, 4, true, 4)))
  axi_ila.io.ila_data := io.s_axi

  val ila = Module(new ILA(Decoupled(new queue_entry_t)))
  ila.io.ila_data := io.dbuff_notif_i

  val dma_map_ila = Module(new ILA(new dma_map_t))
  dma_map_ila.io.ila_data := delegate.io.dma_map_o.bits
}
