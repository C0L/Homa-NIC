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
class MgmtCore extends Module {
  val io = IO(new Bundle {
    val s_axi = Flipped(new axi(512, 26, true, 8, true, 4, true, 4))

    val h2cPayloadRamReq  = Decoupled(new RamReadReq)
    val h2cPayloadRamResp = Flipped(Decoupled(new RamReadResp))

    val c2hPayloadRamReq  = Decoupled(new RamWriteReq)
    val c2hMetadataRamReq = Decoupled(new RamWriteReq)

    val h2cPayloadDmaReq  = Decoupled(new DmaReadReq)
    val h2cPayloadDmaStat = Flipped(Decoupled(new DmaReadStat))

    val c2hPayloadDmaReq  = Decoupled(new DmaWriteReq)
    // val c2hPayloadDmaStat = Flipped(Decoupled(new dma_write_desc_status_t(1)))

    val c2hMetadataDmaReq  = Decoupled(new DmaWriteReq)
    // val c2hMetadataDmaStat = Flipped(Decoupled(new dma_write_desc_status_t(1)))
  })

  val axi2axis       = Module(new axi2axis) // Convert incoming AXI requests to AXIS
  val delegate       = Module(new Delegate) // Decide where those requests should be placed

  val addr_map       = Module(new AddressMap) // Map read and write requests to DMA address

  val fetch_queue    = Module(new FetchQueue)    // Fetch the next best chunk of data
  val sendmsg_queue  = Module(new SendmsgQueue)  // Send the next best message

  val sendmsg_cb     = Module(new SegmentedRam(262144)) // Memory for sendmsg control blocks
  val recvmsg_cb     = Module(new SegmentedRam(262144)) // Memory for recvmsg control blocks
  val sendmsg_cb_wr  = Module(new SegmentedRamWrite)    // Interface to write sendmsg cbs
  val sendmsg_cb_rd  = Module(new SegmentedRamRead)     // 
  val recvmsg_cb_wr  = Module(new SegmentedRamWrite)    //
  val recvmsg_cb_rd  = Module(new SegmentedRamRead)     //

  delegate.io.c2hMetadataRamReq <> io.c2hMetadataRamReq

  fetch_queue.io.fetchSize   := delegate.io.dynamicConfiguration.fetchRequestSize
  sendmsg_queue.io.fetchSize := delegate.io.dynamicConfiguration.fetchRequestSize

  // h2c_dma.io.dynamicConfiguration := delegate.io.dynamicConfiguration

  val pp_ingress = Module(new PPingressStages)

  pp_ingress.io.dynamicConfiguration := delegate.io.dynamicConfiguration

  pp_ingress.io.cb_ram_read_desc <> recvmsg_cb_rd.io.ramReadReq
  pp_ingress.io.cb_ram_read_data <> recvmsg_cb_rd.io.ramReadResp

  pp_ingress.io.c2hPayloadRamReq <> io.c2hPayloadRamReq 
  pp_ingress.io.c2hPayloadDmaReq <> addr_map.io.c2hPayloadDmaReqIn

  val pp_egress  = Module(new PPegressStages)

  pp_egress.io.cb_ram_read_desc <> sendmsg_cb_rd.io.ramReadReq
  pp_egress.io.cb_ram_read_data <> sendmsg_cb_rd.io.ramReadResp

  pp_egress.io.payload_ram_read_desc <> io.h2cPayloadRamReq
  pp_egress.io.payload_ram_read_data <> io.h2cPayloadRamResp

  pp_egress.io.trigger <> sendmsg_queue.io.dequeue
  pp_egress.io.egress  <> pp_ingress.io.ingress

  // TODO should be able to handle this with a small wrapper + implicit clk/reset

  sendmsg_cb.io.ram_wr <> sendmsg_cb_wr.io.ram_wr
  sendmsg_cb.io.ram_rd <> sendmsg_cb_rd.io.ram_rd

  recvmsg_cb.io.ram_wr <> recvmsg_cb_wr.io.ram_wr
  recvmsg_cb.io.ram_rd <> recvmsg_cb_rd.io.ram_rd

  delegate.io.ppEgressSendmsgRamReq  <> sendmsg_cb_wr.io.ramWriteReq
  delegate.io.ppIngressRecvmsgRamReq <> recvmsg_cb_wr.io.ramWriteReq

  addr_map.io.dmaMap              <> delegate.io.newDmaMap
  addr_map.io.c2hMetadataDmaReqIn <> delegate.io.c2hMetadataDmaReq
  addr_map.io.c2hPayloadDmaReqIn  <> pp_ingress.io.c2hPayloadDmaReq
  addr_map.io.h2cPayloadDmaReqIn  <> fetch_queue.io.dequeue 

  addr_map.io.c2hMetadataDmaReqOut <> io.c2hMetadataDmaReq
  addr_map.io.c2hPayloadDmaReqOut  <> io.c2hPayloadDmaReq
  addr_map.io.h2cPayloadDmaReqOut  <> io.h2cPayloadDmaReq

  fetch_queue.io.readcmpl <> io.h2cPayloadDmaStat
  fetch_queue.io.enqueue <> delegate.io.newFetchdata

  // TODO need to throw out read requests
  // axi2axis takes the address of an AXI write and converts it to an
  // AXIS transaction where the user bits store the original address
  axi2axis.io.s_axi         <> io.s_axi
  axi2axis.io.m_axis        <> delegate.io.function_i
  axi2axis.io.s_axi_aclk    <> clock
  axi2axis.io.s_axi_aresetn <> !reset.asBool


  val newSendmsgQueue = Module(new Queue(new QueueEntry, 1, true, false)) 
  val newDnotifsQueue = Module(new Queue(new QueueEntry, 1, true, false))

  newSendmsgQueue.io.enq <> delegate.io.newSendmsg
  newDnotifsQueue.io.enq <> fetch_queue.io.notifout

  // Alternate between fetchdata and notifs to the fetch queue
  val sendmsg_arbiter = Module(new RRArbiter(new QueueEntry, 2))
  sendmsg_arbiter.io.in(0) <> newSendmsgQueue.io.deq
  sendmsg_arbiter.io.in(1) <> newDnotifsQueue.io.deq
  sendmsg_arbiter.io.out   <> sendmsg_queue.io.enqueue
  // delegate.io.newSendmsg <> sendmsg_queue.io.enqueue

  /* DEBUGGING ILAS */
  // val axi2axis_ila = Module(new ILA(new axis(512, false, 0, true, 32, false, 0, false)))
  // axi2axis_ila.io.ila_data := axi2axis.io.m_axis

  val dbuff_ila = Module(new ILA(new QueueEntry))
  dbuff_ila.io.ila_data := newDnotifsQueue.io.deq.bits

  // val sendmsg_in_ila = Module(new ILA(Decoupled(new QueueEntry)))
  // sendmsg_in_ila.io.ila_data := sendm

  val sendmsg_out_ila = Module(new ILA(Decoupled(new QueueEntry)))
  sendmsg_out_ila.io.ila_data := sendmsg_queue.io.dequeue

  // val axi_ila = Module(new ILA(new axi(512, 26, true, 8, true, 4, true, 4)))
  // axi_ila.io.ila_data := io.s_axi

  // val dma_map_ila = Module(new ILA(new dma_map_t))
  // dma_map_ila.io.ila_data := delegate.io.dma_map_o.bits

  // val fetch_in_ila = Module(new ILA(new QueueEntry))
  // fetch_in_ila.io.ila_data := fetch_queue.io.enqueue.bits

  val fetch_out_ila = Module(new ILA(new DmaReadReq))
  fetch_out_ila.io.ila_data := fetch_queue.io.dequeue.bits

  val dyncfg_ila = Module(new ILA(new DynamicConfiguration))
  dyncfg_ila.io.ila_data := delegate.io.dynamicConfiguration

  // val dma_meta_w_ila = Module(new ILA(new dma_write_t))
  // dma_meta_w_ila.io.ila_data := delegate.io.dma_w_req_o.bits
}
