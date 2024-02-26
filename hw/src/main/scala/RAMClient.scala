package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

// TODO these add a few unneccesary cycles of latency


/* psdpram_rd - simplisitc fixed width read interface to psdram
 * module. This assumes the corresponding segmented memory is
 * parameterized with a 512 bit segmented size, 2 segments, and 11 bit
 * address per segment.
 */
class psdpram_rd extends Module {
  val io = IO(new Bundle {
    val ram_read_desc = Flipped(Decoupled(new ram_read_desc_t))
    val ram_read_data = Decoupled(new ram_read_data_t)
    val ram_rd        = new ram_rd_t
  })

  // TODO we do assume that both even and odd channels will act together

  val in_latch = Module(new Queue(new ram_read_desc_t, 1, true, false))
  val pending = Module(new Queue(new ram_read_desc_t, 6, true, false)) 
  val out_latch = Module(new Queue(new ram_read_data_t, 1, true, false))
  in_latch.io.enq <> io.ram_read_desc

  // Element is pending once it enters the RAM module
  pending.io.enq.bits  := in_latch.io.deq.bits
  pending.io.enq.valid := in_latch.io.deq.fire

  in_latch.io.deq.ready := io.ram_rd.cmd_ready.andR
  io.ram_rd.cmd_valid   := Fill(2, in_latch.io.deq.valid)

  val even_addr = Wire(UInt(11.W))
  val odd_addr  = Wire(UInt(11.W))

  even_addr := (in_latch.io.deq.bits.ram_addr >> 7) + in_latch.io.deq.bits.ram_addr(6)
  odd_addr  := in_latch.io.deq.bits.ram_addr >> 7

  io.ram_rd.cmd_addr   := Cat(odd_addr, even_addr)
  io.ram_rd.resp_ready := Fill(2, out_latch.io.enq.ready)

  // Element is no longer pending when we trasfer from RAM to output reg
  pending.io.deq.ready := out_latch.io.enq.fire

  val shift_bits   = Wire(UInt(9.W))
  val ordered_data = Wire(UInt(1024.W))
  ordered_data := 0.U

  when (pending.io.deq.bits.ram_addr(6)) {
    ordered_data := Cat(io.ram_rd.resp_data(511,0), io.ram_rd.resp_data(1023,512))
  }.otherwise {
    ordered_data := io.ram_rd.resp_data
  }

  shift_bits := pending.io.deq.bits.ram_addr(5,0) * 8.U

  out_latch.io.enq.bits      := 0.U.asTypeOf(new ram_read_data_t)
  out_latch.io.enq.bits.data := (ordered_data >> shift_bits)(511,0)
  out_latch.io.enq.valid     := io.ram_rd.resp_valid.andR

  out_latch.io.deq <> io.ram_read_data
}


/* psdpram_wr - simplisitc fixed width write interface to psdram
 * module. This assumes the corresponding segmented memory is
 * parameterized with a 512 bit segmented size, 2 segments, and 11 bit
 * address per segment.
 */
class psdpram_wr extends Module {
  val io = IO(new Bundle {
    val ram_write_cmd  = Flipped(Decoupled(new RamWrite))
    val ram_wr         = new ram_wr_t
  })

  // TODO we do assume that both even and odd channels will act together

  val in_latch = Module(new Queue(new RamWrite, 1, true, false))

  in_latch.io.enq <> io.ram_write_cmd

  in_latch.io.deq.ready := io.ram_wr.cmd_ready.andR
  io.ram_wr.cmd_valid   := Fill(2, in_latch.io.deq.valid)

  val even_addr = Wire(UInt(11.W))
  val odd_addr  = Wire(UInt(11.W))

  even_addr := (in_latch.io.deq.bits.addr >> 7) + in_latch.io.deq.bits.addr(6)
  odd_addr  := in_latch.io.deq.bits.addr >> 7

  io.ram_wr.cmd_addr   := Cat(odd_addr, even_addr)

  io.ram_wr.cmd_ready := DontCare // ignore this for now?
  io.ram_wr.done  := DontCare // ignore this for now?

  val shift_bits = Wire(UInt(9.W))
  val ordered_data = Wire(UInt(1024.W))
  ordered_data := 0.U
  shift_bits := in_latch.io.deq.bits.addr(5,0) * 8.U

  val extended = Wire(UInt(1024.W))
  val be       = Wire(UInt(128.W))
  extended := in_latch.io.deq.bits.data.asTypeOf(UInt(1024.W)) << shift_bits
  be       := ("hffffffffffffffffffffffffffffffff".U >> (128.U - in_latch.io.deq.bits.len)) << in_latch.io.deq.bits.addr(5,0) 
                    
  when (in_latch.io.deq.bits.addr(6)) {
    io.ram_wr.cmd_data := Cat(extended(511,0), extended(1023,512))
    io.ram_wr.cmd_be   := Cat(be(63,0), be(127,64))
  }.otherwise {
    io.ram_wr.cmd_data := extended
    io.ram_wr.cmd_be   := be
  }
}
