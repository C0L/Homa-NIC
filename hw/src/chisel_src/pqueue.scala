package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

// TODO make module ontop to send native chisel types

class srpt_queue (qtype: String) extends BlackBox (
    Map("MAX_RPCS" -> 64,
      "TYPE" -> qtype,
  )) {

  val io = IO(new Bundle {
    val s_axis = Flipped(new axis(88, false, 0, false, 0, false))
    val m_axis = new axis(88, false, 0, false, 0, false)
  })
}

class fetch_queue extends Module {
  val io = IO(new Bundle {
    val enqueue = Flipped(Decoupled(new queue_entry_t))
    // TODO this is temporary
    val dequeue = Decoupled(new dma_read_t)
  })

  val fetch_queue_raw = Module(new srpt_queue("fetch"))

  fetch_queue_raw.io.s_axis.tdata  := io.enqueue.bits.asTypeOf(UInt(88.W))
  io.enqueue.ready                 := fetch_queue_raw.io.s_axis.tready
  fetch_queue_raw.io.s_axis.tvalid := io.enqueue.valid

  val fetch_queue_raw_out = Wire(new queue_entry_t)

  fetch_queue_raw_out := fetch_queue_raw.io.m_axis.tdata.asTypeOf(new queue_entry_t)
  val dma_read = Wire(new dma_read_t)

  dma_read.pcie_read_addr := fetch_queue_raw_out.dbuffered
  dma_read.cache_id       := fetch_queue_raw_out.dbuff_id // TODO bad name 
  dma_read.message_size   := fetch_queue_raw_out.granted
  dma_read.read_len       := 256.U // TODO placeholder
  dma_read.dest_ram_addr  := fetch_queue_raw_out.dbuffered
  dma_read.port           := 1.U // TODO placeholder

  io.dequeue.bits         := dma_read

  fetch_queue_raw.io.m_axis.tready := io.dequeue.ready
  io.dequeue.valid                 := fetch_queue_raw.io.m_axis.tvalid

}

//class sendmsg_queue_raw extends BlackBox (
//    Map("MAX_RPCS" -> "64",
//      "TYPE" -> "sendmsg",
//    )) {
//
//  val io = IO(new Bundle {
//    val s_axis = Flipped(new axis(88, false, 0, false, 0, false))
//    val m_axis = new axis(88, false, 0, false, 0, false)
//  })
//}

class sendmsg_queue extends Module {
  val io = IO(new Bundle {
    val enqueue = Flipped(Decoupled(new queue_entry_t))
    // TODO this is temporary
    val dequeue = Decoupled(new queue_entry_t)
  })

  val send_queue_raw = Module(new srpt_queue("sendmsg"))

  val raw_in  = io.enqueue.bits.asTypeOf(UInt(88.W))
  val raw_out = Wire(new queue_entry_t)

  raw_out := send_queue_raw.io.m_axis.tdata.asTypeOf(new queue_entry_t)

  io.enqueue.ready                := send_queue_raw.io.s_axis.tready
  send_queue_raw.io.s_axis.tvalid := io.enqueue.valid
  send_queue_raw.io.s_axis.tdata  := raw_in

  send_queue_raw.io.m_axis.tready := io.dequeue.ready
  io.dequeue.valid                := send_queue_raw.io.m_axis.tvalid
  io.dequeue.bits                 := raw_out
}
