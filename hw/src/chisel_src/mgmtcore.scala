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

    // TODO from the packet core Eventually maybe move queues to another module 
    val dma_w_data_i          = Flipped(Decoupled(new dma_write_t))
  })

  val axi2axis       = Module(new axi2axis) // Convert incoming AXI requests to AXIS
  val delegate       = Module(new delegate) // Decide where those requests should be placed

  val addr_map       = Module(new addr_map) // Map read and write requests to DMA address
  val h2c_dma        = Module(new h2c_dma)  // Manage dma reads 
  val c2h_dma        = Module(new c2h_dma)  // Manage dma writes

  val fetch_queue    = Module(new fetch_queue)    // Fetch the next best chunk of data
  val sendmsg_queue  = Module(new sendmsg_queue)  // Send the next best message

  val control_block = Module(new psdpram)
  val cb_client_wr  = Module(new client_axis_sink)
  val cb_client_rd  = Module(new client_axis_source)

  control_block.io.clk := clock
  control_block.io.rst := reset.asUInt

  cb_client_wr.io.clk := clock
  cb_client_wr.io.rst := reset.asUInt

  cb_client_rd.io.clk := clock
  cb_client_rd.io.rst := reset.asUInt

  control_block.io.ram_wr <> cb_client_wr.io.ram_wr
  control_block.io.ram_rd <> cb_client_rd.io.ram_rd

  cb_client_wr.io.enable := 1.U
  cb_client_rd.io.enable := 1.U

  cb_client_rd.io.ram_read_desc        := DontCare
  cb_client_rd.io.ram_read_desc_status := DontCare
  cb_client_rd.io.ram_read_data        := DontCare

  // delegate.io.ram__desc        <> cb_client_rd.io.ram_read_desc
  // delegate.io.ram__desc_status <> cb_client_rd.io.ram_read_desc_status
  // delegate.io.ram__data        <> cb_client_rd.io.ram_read_data

  delegate.io.ram_write_desc        <> cb_client_wr.io.ram_write_desc
  delegate.io.ram_write_desc_status <> cb_client_wr.io.ram_write_desc_status
  delegate.io.ram_write_data        <> cb_client_wr.io.ram_write_data


  io.ram_read_desc := DontCare
  io.ram_read_desc_status := DontCare
  io.ram_read_data := DontCare

  addr_map.io.dma_map_i    <> delegate.io.dma_map_o
  addr_map.io.dma_w_meta_i <> delegate.io.dma_w_req_o
  addr_map.io.dma_r_req_i  <> fetch_queue.io.dequeue
  addr_map.io.dma_w_data_i <> io.dma_w_data_i

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
  sendmsg_queue.io.dequeue      := DontCare

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
