package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

// TODO these add a few unneccesary cycles of latency
// TODO need to more carefully test the flow control
// TODO this assumes the "even" and "odd" channels always act together

/* SegmentedRAMRead - simplisitc fixed width read interface to psdram
 * module. This assumes the corresponding segmented memory is
 * parameterized with a 512 bit segmented size, 2 segments, and 11 bit
 * address per segment.
 */
class SegmentedRamRead extends Module {
  val io = IO(new Bundle {
    val ramReadReq  = Flipped(Decoupled(new RamReadReq)) // Requests to read data from RAM
    val ramReadResp = Decoupled(new RamReadResp) // Returned result from RAM
    val ram_rd        = new ram_rd_t(1) // Interface from client (this) to RAM module
  })

  /* Read requests are latched by the in_latch. When the RAM core is
   * ready for it (ram_rd.cmd_ready), we will send a request to the
   * core and move the element from in_latch to pending. Pending
   * should be as large or larger than the number of pipeline stages
   * in the RAM core to operate at full throughput. When the RAM
   * result is valid (ram_rd.resp_valid), the out_latch latches on the
   * result, which is connected to the output port.
   */
  val in_latch  = Module(new Queue(new RamReadReq, 1, true, false))
  val pending   = Module(new Queue(new RamReadReq, 6, true, false)) 
  val out_latch = Module(new Queue(new RamReadResp, 1, true, false))
  in_latch.io.enq <> io.ramReadReq

  // A read requests moves from in_latch to pending when the read request is fired to RAM
  pending.io.enq.bits  := in_latch.io.deq.bits
  pending.io.enq.valid := in_latch.io.deq.fire

  // A read request will occur if the in_latch has data and the RAM core is ready to accept it
  in_latch.io.deq.ready := io.ram_rd.cmd_ready.andR
  io.ram_rd.cmd_valid   := Fill(2, in_latch.io.deq.valid)

  /* The segmented memory RAM has even and odd segments, which are
   * word addressed rather than the byte addressing of the input. If
   * we want to read from address 32, we issue a read address of 0,0.
   * If we want to read from address 160, we issue a read address of
   * 1,1. If We want to read from address 96, we issue a read request
   * of 0, 1, as we are wrapping around from the 0th odd chunk (bytes
   * 64-128) to the 1st even chunk (bytes 128-192).
   */
  val even_addr = Wire(UInt(11.W))
  val odd_addr  = Wire(UInt(11.W))

  // Get word offset and shift even addr if wrap around 
  even_addr := (in_latch.io.deq.bits.addr >> 7) + in_latch.io.deq.bits.addr(6)
  odd_addr  := in_latch.io.deq.bits.addr >> 7

  // Combine the even and odd address and ready into a single signal
  io.ram_rd.cmd_addr   := Cat(odd_addr, even_addr)
  io.ram_rd.resp_ready := Fill(2, out_latch.io.enq.ready)

  // Element is no longer pending when we trasfer from RAM to output reg
  pending.io.deq.ready := out_latch.io.enq.fire

  // Reconstruct the contiguous 128 byte block based on whether or not we wrapped around 
  val ordered_data = Wire(UInt(1024.W))
  ordered_data := 0.U

  when (pending.io.deq.bits.addr(6)) {
    ordered_data := Cat(io.ram_rd.resp_data(511,0), io.ram_rd.resp_data(1023,512))
  }.otherwise {
    ordered_data := io.ram_rd.resp_data
  }

  // Determine how much we need to shift the output data to gather the 64 byte request with LSB at offset 0
  val shift_bits   = Wire(UInt(9.W))

  shift_bits := pending.io.deq.bits.addr(5,0) * 8.U

  out_latch.io.enq.bits      := 0.U.asTypeOf(new RamReadResp)
  out_latch.io.enq.bits.data := (ordered_data >> shift_bits)(511,0)
  out_latch.io.enq.valid     := io.ram_rd.resp_valid.andR

  // Connect output latch to the IO output 
  out_latch.io.deq <> io.ramReadResp
}


/* SegmentedRAMWrite - simplisitc fixed width write interface to psdram
 * module. This assumes the corresponding segmented memory is
 * parameterized with a 512 bit segmented size, 2 segments, and 11 bit
 * address per segment.
 */
class SegmentedRamWrite extends Module {
  val io = IO(new Bundle {
    val ramWriteReq = Flipped(Decoupled(new RamWriteReq))
    val ram_wr      = new ram_wr_t(1)
  })

  // Buffer for write requests going to RAM
  val in_latch = Module(new Queue(new RamWriteReq, 1, true, false))

  // Write commands get added to the in latch before reaching RAM interface
  in_latch.io.enq <> io.ramWriteReq

  /* A latched write is dequeued when both channels of RAM core are
   * ready to receive and so both channels of the ram_wr.cmd_valid
   * will become valid simultaneously.
   */
  in_latch.io.deq.ready := io.ram_wr.cmd_ready.andR
  io.ram_wr.cmd_valid   := Fill(2, in_latch.io.deq.valid)

  /* The segmented memory RAM has even and odd segments, which are
   * word addressed rather than the byte addressing of the input. If
   * we want to read from address 32, we issue a read address of 0,0.
   * If we want to read from address 160, we issue a read address of
   * 1,1. If We want to read from address 96, we issue a read request
   * of 0, 1, as we are wrapping around from the 0th odd chunk (bytes
   * 64-128) to the 1st even chunk (bytes 128-192).
   */
  val even_addr = Wire(UInt(11.W))
  val odd_addr  = Wire(UInt(11.W))

  // Get word offset and shift even addr if wrap around 
  even_addr := (in_latch.io.deq.bits.addr >> 7) + in_latch.io.deq.bits.addr(6)
  odd_addr  := in_latch.io.deq.bits.addr >> 7

  // Combine the even and odd address 
  io.ram_wr.cmd_addr   := Cat(odd_addr, even_addr)

  // We assume that the RAM core is always ready for a command and we ignore the done signal
  io.ram_wr.cmd_ready := DontCare 
  io.ram_wr.done  := DontCare 

  // We need to shift the input data to the correct byte offset of a 128 byte write op
  val shift_bits = Wire(UInt(9.W))
  shift_bits := in_latch.io.deq.bits.addr(5,0) * 8.U

  // Extend the 64 byte write to 128 bytes and shift 64 bytes of write data to offset in word
  val extended = Wire(UInt(1024.W))
  extended := in_latch.io.deq.bits.data.asTypeOf(UInt(1024.W)) << shift_bits

  /* Compute which bytes of the write will be active, or byte enable.
   * Begin with 128 bits of all 1s which corresponds to 128 bytes of
   * write data. Shift right to zero the upper bytes based on the
   * length of the write (0 length write results in 0 value byte
   * enable, 64 byte write results in all 1s value byte enable). Shift
   * left based on the offset of the address within the word.
   */
  val be       = Wire(UInt(128.W))
  be       := ("hffffffffffffffffffffffffffffffff".U >> (128.U - in_latch.io.deq.bits.len)) << in_latch.io.deq.bits.addr(5,0) 

  // Break contiguous 128 byte block into segmented write based on whether data wraps or not
  when (in_latch.io.deq.bits.addr(6)) {
    io.ram_wr.cmd_data := Cat(extended(511,0), extended(1023,512))
    io.ram_wr.cmd_be   := Cat(be(63,0), be(127,64))
  }.otherwise {
    io.ram_wr.cmd_data := extended
    io.ram_wr.cmd_be   := be
  }
}
