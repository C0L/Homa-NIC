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
    val s_axi = Flipped(new axi(512, 26, true, 8, true, 4, true, 4))

    val ram_read_desc         = Decoupled(new ram_read_desc_t)
    val ram_read_desc_status  = Flipped(Decoupled(new ram_read_desc_status_t))
    val ram_read_data         = Flipped(Decoupled(new ram_read_data_t))

    val ram_write_desc        = Decoupled(new ram_write_desc_t)
    val ram_write_data        = Decoupled(new ram_write_data_t)
    val ram_write_desc_status = Flipped(Decoupled(new ram_write_desc_status_t))

    val dma_read_desc         = Decoupled(new dma_read_desc_t)
    val dma_read_desc_status  = Flipped(Decoupled(new dma_read_desc_status_t))

    val dma_write_desc        = Decoupled(new dma_write_desc_t)
    val dma_write_desc_status = Flipped(Decoupled(new dma_write_desc_status_t))


  })

  val axi2axis       = Module(new axi2axis) // Convert incoming AXI requests to AXIS
  val delegate       = Module(new delegate) // Decide where those requests should be placed

  val addr_map       = Module(new addr_map) // Map read and write requests to DMA address
  val h2c_dma        = Module(new h2c_dma)  // Manage dma reads 
  val c2h_dma        = Module(new c2h_dma)  // Manage dma writes

  val fetch_queue    = Module(new fetch_queue)    // Fetch the next best chunk of data
  val sendmsg_queue  = Module(new sendmsg_queue)  // Send the next best message

  val sendmsg_cb     = Module(new dma_psdpram)
  val recvmsg_cb     = Module(new dma_psdpram)
  val sendmsg_cb_wr  = Module(new dma_client_axis_sink)
  val sendmsg_cb_rd  = Module(new dma_client_axis_source)
  val recvmsg_cb_wr  = Module(new dma_client_axis_sink)
  val recvmsg_cb_rd  = Module(new dma_client_axis_source)

  // TODO this should be seperated out

  val pp_ingress = Module(new pp_ingress_stages)

  pp_ingress.io.cb_ram_read_desc        <> recvmsg_cb_rd.io.ram_read_desc
  pp_ingress.io.cb_ram_read_desc_status <> recvmsg_cb_rd.io.ram_read_desc_status
  pp_ingress.io.cb_ram_read_data        <> recvmsg_cb_rd.io.ram_read_data

  addr_map.io.dma_w_data_i <> pp_ingress.io.dma_w_data

  // pp_ingress.io.cb_ram_read_desc        <> cb_client_rd.io.ram_read_desc
  // pp_ingress.io.cb_ram_read_desc_status <> cb_client_rd.io.ram_read_desc_status
  // pp_ingress.io.cb_ram_read_data        <> cb_client_rd.io.ram_read_data

  val pp_egress  = Module(new pp_egress_stages)

  pp_egress.io.cb_ram_read_desc        <> sendmsg_cb_rd.io.ram_read_desc
  pp_egress.io.cb_ram_read_desc_status <> sendmsg_cb_rd.io.ram_read_desc_status
  pp_egress.io.cb_ram_read_data        <> sendmsg_cb_rd.io.ram_read_data

  pp_egress.io.payload_ram_read_desc        <> io.ram_read_desc
  pp_egress.io.payload_ram_read_desc_status <> io.ram_read_desc_status
  pp_egress.io.payload_ram_read_data        <> io.ram_read_data

  pp_egress.io.trigger <> sendmsg_queue.io.dequeue
  pp_egress.io.egress  <> pp_ingress.io.ingress

  // TODO should be able to handle this with a small wrapper + implicit clk/reset

  sendmsg_cb.io.clk := clock
  sendmsg_cb.io.rst := reset.asUInt

  recvmsg_cb.io.clk := clock
  recvmsg_cb.io.rst := reset.asUInt

  sendmsg_cb_wr.io.clk := clock
  sendmsg_cb_wr.io.rst := reset.asUInt

  sendmsg_cb_rd.io.clk := clock
  sendmsg_cb_rd.io.rst := reset.asUInt

  sendmsg_cb.io.ram_wr <> sendmsg_cb_wr.io.ram_wr
  sendmsg_cb.io.ram_rd <> sendmsg_cb_rd.io.ram_rd

  recvmsg_cb_wr.io.clk := clock
  recvmsg_cb_wr.io.rst := reset.asUInt

  recvmsg_cb_rd.io.clk := clock
  recvmsg_cb_rd.io.rst := reset.asUInt

  recvmsg_cb.io.ram_wr <> recvmsg_cb_wr.io.ram_wr
  recvmsg_cb.io.ram_rd <> recvmsg_cb_rd.io.ram_rd

  sendmsg_cb_wr.io.enable := 1.U
  sendmsg_cb_rd.io.enable := 1.U

  recvmsg_cb_wr.io.enable := 1.U
  recvmsg_cb_rd.io.enable := 1.U



// TODO pass to pp ingress
  // pp_ingress.io.cb_ram_read_desc        <> sendmsg_cb_rd.io.ram_read_desc
  // pp_ingress.io.cb_ram_read_desc_status <> sendmsg_cb_rd.io.ram_read_desc_status
  // pp_ingress.io.cb_ram_read_data        <> sendmsg_cb_rd.io.ram_read_data       

  // pp_ingress.io.cb_ram_read_desc        <> recvmsg_cb_rd.io.ram_read_desc       
  // pp_ingress.io.cb_ram_read_desc_status <> recvmsg_cb_rd.io.ram_read_desc_status
  // pp_ingress.io.cb_ram_read_data        <> recvmsg_cb_rd.io.ram_read_data       

  delegate.io.sendmsg_ram_write_desc        <> sendmsg_cb_wr.io.ram_write_desc
  delegate.io.sendmsg_ram_write_desc_status <> sendmsg_cb_wr.io.ram_write_desc_status
  delegate.io.sendmsg_ram_write_data        <> sendmsg_cb_wr.io.ram_write_data

  delegate.io.recvmsg_ram_write_desc        <> recvmsg_cb_wr.io.ram_write_desc
  delegate.io.recvmsg_ram_write_desc_status <> recvmsg_cb_wr.io.ram_write_desc_status
  delegate.io.recvmsg_ram_write_data        <> recvmsg_cb_wr.io.ram_write_data

  io.ram_read_desc        := DontCare
  io.ram_read_desc_status := DontCare
  io.ram_read_data        := DontCare

  addr_map.io.dma_map_i    <> delegate.io.dma_map_o
  addr_map.io.dma_w_meta_i <> delegate.io.dma_w_req_o
  addr_map.io.dma_r_req_i  <> fetch_queue.io.dequeue


  addr_map.io.dma_r_req_o  <> h2c_dma.io.dma_r_req

  h2c_dma.io.dma_read_desc   <> io.dma_read_desc
  h2c_dma.io.dma_read_status <> io.dma_read_desc_status

  c2h_dma.io.ram_write_desc        <> io.ram_write_desc
  c2h_dma.io.ram_write_data        <> io.ram_write_data
  c2h_dma.io.ram_write_desc_status <> io.ram_write_desc_status

  io.dma_write_desc                <> c2h_dma.io.dma_write_desc

  c2h_dma.io.dma_write_desc_status <> io.dma_write_desc_status
  c2h_dma.io.dma_write_req         <> addr_map.io.dma_w_req_o

  io.dma_write_desc        <> c2h_dma.io.dma_write_desc
  io.dma_write_desc_status <> c2h_dma.io.dma_write_desc_status

  // Alternate between fetchdata and notifs to the fetch queue
  val fetch_arbiter = Module(new RRArbiter(new queue_entry_t, 2))
  fetch_arbiter.io.in(0) <> delegate.io.fetchdata_o
  fetch_arbiter.io.in(1) <> h2c_dma.io.dbuff_notif_o
  fetch_arbiter.io.out   <> fetch_queue.io.enqueue

  // TODO need to throw out read requests
  // axi2axis takes the address of an AXI write and converts it to an
  // AXIS transaction where the user bits store the original address
  axi2axis.io.s_axi         <> io.s_axi
  axi2axis.io.m_axis        <> delegate.io.function_i
  axi2axis.io.s_axi_aclk    <> clock
  axi2axis.io.s_axi_aresetn <> !reset.asBool

  delegate.io.sendmsg_o <> sendmsg_queue.io.enqueue 

  // TODO placeholder
  // delegate.io.dma_w_req_o       := DontCare
  // io.dma_w_meta_o <> delegate.io.dma_w_req_o

  // TODO eventually goes to packet constructor
  // sendmsg_queue.io.dequeue      := DontCare

  /* DEBUGGING ILAS */
  val axi2axis_ila = Module(new ILA(new axis(512, false, 0, true, 32, false, 0, false)))
  axi2axis_ila.io.ila_data := axi2axis.io.m_axis

  val sendmsg_ila = Module(new ILA(Decoupled(new queue_entry_t)))
  sendmsg_ila.io.ila_data := delegate.io.sendmsg_o

  val axi_ila = Module(new ILA(new axi(512, 26, true, 8, true, 4, true, 4)))
  axi_ila.io.ila_data := io.s_axi

  // val ila = Module(new ILA(Decoupled(new queue_entry_t)))
  // ila.io.ila_data := io.dbuff_notif_i

  val dma_map_ila = Module(new ILA(new dma_map_t))
  dma_map_ila.io.ila_data := delegate.io.dma_map_o.bits

  val fetch_in_ila = Module(new ILA(new queue_entry_t))
  fetch_in_ila.io.ila_data := fetch_queue.io.enqueue.bits

  val fetch_out_ila = Module(new ILA(new dma_read_t))
  fetch_out_ila.io.ila_data := fetch_queue.io.dequeue.bits

  val dma_meta_w_ila = Module(new ILA(new dma_write_t))
  dma_meta_w_ila.io.ila_data := delegate.io.dma_w_req_o.bits
}