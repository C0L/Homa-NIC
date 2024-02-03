package dma

import chisel3._
import circt.stage.ChiselStage
import chisel3.util.Decoupled

class ctrl extends Module {
  //val io = IO(new Bundle {
  //  val dma_req  = Decoupled(UInt(512.W))
  //  val dma_resp = Decoupled(UInt(512.W))
  //})

  // Create a consumer interface
  val io = IO(new Bundle {
    val input = Flipped(Decoupled(UInt(512.W)))
  })

  io.input.ready := false.B
  // input.ready := true.B

  when (io.input.valid) {
    // io.dma_resp.bits := "hdeadbeef".U
    // io.dma_resp := true.B
  }
}

object Main extends App {
  // These lines generate the Verilog output
  println(
    ChiselStage.emitSystemVerilog(
      new ctrl,
      firtoolOpts = Array("-disable-all-randomization", "-strip-debug-info")
    )
  )
}

// object Main extends App{
//       (new ChiselStage).emitVerilog(new ctrl, args)
// }

// object finalMainClass extends App {
//   // These lines generate the Verilog output
//   println(
//     ChiselStage.emitSystemVerilog(
//       new dma_ctrl(),
//       firtoolOpts = Array("-disable-all-randomization", "-strip-debug-info")
//     )
//   )
// }
