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
class MGMTCore extends Module {
  val io = IO(new Bundle {
    val s_axi = Flipped(new axi(512, 26, true, 8, true, 4, true, 4))

    val payloadRamReadReq  = Decoupled(new RAMReadReq)
    val payloadRamReadResp = Flipped(Decoupled(new RAMReadResp))

    val payloadRamWriteReq = Decoupled(new RAMWriteReq)

    val dma_read_desc        = Decoupled(new dma_read_desc_t)
    val dma_read_desc_status = Flipped(Decoupled(new dma_read_desc_status_t))

    val dma_write_desc        = Decoupled(new dma_write_desc_t)
    val dma_write_desc_status = Flipped(Decoupled(new dma_write_desc_status_t))
  })

  val axi2axis       = Module(new axi2axis) // Convert incoming AXI requests to AXIS
  val delegate       = Module(new Delegate) // Decide where those requests should be placed

  val addr_map       = Module(new AddressMap) // Map read and write requests to DMA address
  val h2c_dma        = Module(new H2CDMA)  // Manage dma reads 
  val c2h_dma        = Module(new C2HDMA)  // Manage dma writes

  val fetch_queue    = Module(new FetchQueue)    // Fetch the next best chunk of data
  val sendmsg_queue  = Module(new SendmsgQueue)  // Send the next best message

  val sendmsg_cb     = Module(new SegmentedRAM(262144)) // Memory for sendmsg control blocks
  val recvmsg_cb     = Module(new SegmentedRAM(262144)) // Memory for recvmsg control blocks
  val sendmsg_cb_wr  = Module(new SegmentedRAMWrite)    // Interface to write sendmsg cbs
  val sendmsg_cb_rd  = Module(new SegmentedRAMRead)     // 
  val recvmsg_cb_wr  = Module(new SegmentedRAMWrite)    //
  val recvmsg_cb_rd  = Module(new SegmentedRAMRead)     //

  fetch_queue.io.CACHE_BLOCK_SIZE   := delegate.io.dynamicConfiguration.fetchSize
  sendmsg_queue.io.CACHE_BLOCK_SIZE := delegate.io.dynamicConfiguration.fetchSize

  h2c_dma.io.dynamicConfiguration := delegate.io.dynamicConfiguration

  val pp_ingress = Module(new PPingressStages)

  pp_ingress.io.cb_ram_read_desc <> recvmsg_cb_rd.io.ramReadReq
  pp_ingress.io.cb_ram_read_data <> recvmsg_cb_rd.io.ramReadResp

  addr_map.io.dma_w_data_i <> pp_ingress.io.dma_w_data

  val pp_egress  = Module(new PPegressStages)

  pp_egress.io.cb_ram_read_desc <> sendmsg_cb_rd.io.ramReadReq
  pp_egress.io.cb_ram_read_data <> sendmsg_cb_rd.io.ramReadResp

  pp_egress.io.payload_ram_read_desc        <> io.payloadRamReadReq
  // pp_egress.io.payload_ram_read_desc_status <> io.ram_read_desc_status
  pp_egress.io.payload_ram_read_data        <> io.payloadRamReadResp

  pp_egress.io.trigger <> sendmsg_queue.io.dequeue
  pp_egress.io.egress  <> pp_ingress.io.ingress

  // TODO should be able to handle this with a small wrapper + implicit clk/reset

  sendmsg_cb.io.ram_wr <> sendmsg_cb_wr.io.ram_wr
  sendmsg_cb.io.ram_rd <> sendmsg_cb_rd.io.ram_rd

  recvmsg_cb.io.ram_wr <> recvmsg_cb_wr.io.ram_wr
  recvmsg_cb.io.ram_rd <> recvmsg_cb_rd.io.ram_rd

  delegate.io.sendmsg_ram_write_data <> sendmsg_cb_wr.io.ramWriteReq
  delegate.io.recvmsg_ram_write_data <> recvmsg_cb_wr.io.ramWriteReq

  addr_map.io.dma_map_i    <> delegate.io.dma_map_o
  addr_map.io.dma_w_meta_i <> delegate.io.dma_w_req_o
  addr_map.io.dma_r_req_i  <> fetch_queue.io.dequeue

  addr_map.io.dma_r_req_o  <> h2c_dma.io.dma_r_req

  h2c_dma.io.dma_read_desc   <> io.dma_read_desc
  h2c_dma.io.dma_read_status <> io.dma_read_desc_status

  c2h_dma.io.ram_write_data  <> io.payloadRamWriteReq

  io.dma_write_desc          <> c2h_dma.io.dma_write_desc

  c2h_dma.io.dma_write_desc_status <> io.dma_write_desc_status
  c2h_dma.io.dma_write_req         <> addr_map.io.dma_w_req_o

  io.dma_write_desc        <> c2h_dma.io.dma_write_desc
  io.dma_write_desc_status <> c2h_dma.io.dma_write_desc_status

  // TODO These are updates from the outgoing packet queue
  // val fetch_arbiter = Module(new RRArbiter(new QueueEntry, 2))
  // fetch_arbiter.io.in(0) <> delegate.io.fetchdata_o
  // fetch_arbiter.io.in(1) <> h2c_dma.io.dbuff_notif_o
  // fetch_arbiter.io.out   <> fetch_queue.io.enqueue
  fetch_queue.io.enqueue <> delegate.io.fetchdata_o

  // TODO need to throw out read requests
  // axi2axis takes the address of an AXI write and converts it to an
  // AXIS transaction where the user bits store the original address
  axi2axis.io.s_axi         <> io.s_axi
  axi2axis.io.m_axis        <> delegate.io.function_i
  axi2axis.io.s_axi_aclk    <> clock
  axi2axis.io.s_axi_aresetn <> !reset.asBool


  // Alternate between fetchdata and notifs to the fetch queue
  val sendmsg_arbiter = Module(new RRArbiter(new QueueEntry, 2))
  sendmsg_arbiter.io.in(0) <> delegate.io.sendmsg_o
  sendmsg_arbiter.io.in(1) <> h2c_dma.io.dbuff_notif_o
  sendmsg_arbiter.io.out   <> sendmsg_queue.io.enqueue


  /* DEBUGGING ILAS */
  // val axi2axis_ila = Module(new ILA(new axis(512, false, 0, true, 32, false, 0, false)))
  // axi2axis_ila.io.ila_data := axi2axis.io.m_axis

  val dbuff_in_ila = Module(new ILA(new QueueEntry))
  dbuff_in_ila.io.ila_data := h2c_dma.io.dbuff_notif_o.bits

  // val sendmsg_in_ila = Module(new ILA(Decoupled(new QueueEntry)))
  // sendmsg_in_ila.io.ila_data := delegate.io.sendmsg_o

  // val sendmsg_out_ila = Module(new ILA(Decoupled(new QueueEntry)))
  // sendmsg_out_ila.io.ila_data := sendmsg_queue.io.dequeue

  // val axi_ila = Module(new ILA(new axi(512, 26, true, 8, true, 4, true, 4)))
  // axi_ila.io.ila_data := io.s_axi

  // val dma_map_ila = Module(new ILA(new dma_map_t))
  // dma_map_ila.io.ila_data := delegate.io.dma_map_o.bits

  // val fetch_in_ila = Module(new ILA(new QueueEntry))
  // fetch_in_ila.io.ila_data := fetch_queue.io.enqueue.bits

  // val fetch_out_ila = Module(new ILA(new dma_read_t))
  // fetch_out_ila.io.ila_data := fetch_queue.io.dequeue.bits

  // val dma_meta_w_ila = Module(new ILA(new dma_write_t))
  // dma_meta_w_ila.io.ila_data := delegate.io.dma_w_req_o.bits
}
