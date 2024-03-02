package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

// TODO ensure that still infers brams 

class BRAM[T <: Data](size: Int, aw: Int, gen: T, carry: T) extends Module {
  val io = IO (new Bundle {
    val w_cmd_addr   = Input(UInt(aw.W))
    val w_cmd_data   = Input(gen)
    val w_cmd_valid  = Input(Bool())
    val w_cmd_ready  = Output(Bool())

    val r_cmd_ready  = Output(Bool())
    val r_cmd_valid  = Input(Bool())
    val r_cmd_addr   = Input(UInt(aw.W))
    val r_data_ready = Input(Bool())
    val r_data_valid = Output(Bool())
    val r_data_data  = Output(gen)

    val track_in     = Input(carry)
    val track_out    = Output(carry)
  })

  io.w_cmd_ready := true.B

  val memory = SyncReadMem(size, gen)

  // We are holding data if a transaction took place
  val r_cmd_output_hold  = RegInit(false.B)

  // We cannot accept more data if we are holding and it cannot progress
  val r_cmd_output_ready = !(r_cmd_output_hold && !io.r_data_ready)

  // We are holding data if a transaction took place
  val r_cmd_input_hold = RegInit(false.B)

  // We cannot accept more data if we are holding and it cannot progress
  val r_cmd_input_ready = (r_cmd_input_hold && r_cmd_output_ready) || !r_cmd_input_hold

  // Grab the addr if a transaction occurs
  val r_cmd_input_addr  = RegEnable(io.r_cmd_addr, r_cmd_input_ready && io.r_cmd_valid)

  io.r_data_valid   := r_cmd_output_hold
  io.r_data_data    := memory.read(r_cmd_input_addr)

  r_cmd_input_hold  := r_cmd_input_ready & io.r_cmd_valid
  r_cmd_output_hold := r_cmd_output_ready && r_cmd_input_hold
  io.r_cmd_ready    := r_cmd_input_ready

  val w_cmd_addr_reg = RegNext(io.w_cmd_addr)
  val w_cmd_data_reg = RegNext(io.w_cmd_data)

  when (RegNext(io.w_cmd_valid)) {
    memory.write(w_cmd_addr_reg, w_cmd_data_reg)
  }

  // Grab side data if transaction occurs
  val r_cmd_input_track = RegEnable(io.track_in, r_cmd_input_ready && io.r_cmd_valid)

  io.track_out := RegEnable(r_cmd_input_track, r_cmd_output_ready && r_cmd_input_hold)
}
