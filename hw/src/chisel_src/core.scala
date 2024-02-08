package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

class core extends Module {
  val io = IO(new Bundle {
    val dbuff_notif_i = Flipped(Decoupled(new queue_entry_t))
    val fetch_dequeue = Decoupled(new dma_read_t)
    val dma_map_o     = Decoupled(new dma_map_t)
    val s_axi         = Flipped(new axi(512, 8, 32))
  })

  val axi2axis   = Module(new axi2axis) // Convert incoming AXI requests to AXIS
  val delegate   = Module(new delegate) // Decide where those requests should be placed

  val fetch_queue   = Module(new fetch_queue)    // Fetch the next best chunk of data
  val sendmsg_queue = Module(new sendmsg_queue)  // Send the next best message 

  fetch_queue.io.dequeue <> io.fetch_dequeue
  io.dma_map_o           <> delegate.io.dma_map_o

  // TODO Maybe RR is not the best strategy
  val fetch_arbiter = Module(new RRArbiter(new queue_entry_t, 2))
  fetch_arbiter.io.in(0) <> delegate.io.fetchdata_o
  fetch_arbiter.io.in(1) <> io.dbuff_notif_i
  fetch_arbiter.io.out   <> fetch_queue.io.enqueue

  axi2axis.io.s_axi  <> io.s_axi
  // pcie.m_axi
  axi2axis.io.m_axis <> delegate.io.function_i

  // TODO another clock converter
  delegate.io.sendmsg_o <> sendmsg_queue.io.enqueue // TODO eventually shared

  // TODO placeholder
  delegate.io.dma_w_req_o       := DontCare

  // TODO eventually goes to packet constructor
  sendmsg_queue.io.dequeue      := DontCare

  val ila = Module(new SystemILA(new queue_entry_t))

  // ila.io(0) := DontCare
  ila.io.clk         := clock
  ila.io.resetn      := !reset.asBool
  ila.io.SLOT_0_AXIS := DontCare
}
