package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

/* SimpleRAM - Used to testing the interaction between RAM clients and
 * the segmented RAM verilog module. Only used for testing here
 */
class SimpleRAM extends Module {
  val io = IO(new Bundle {
    val ramReadReq  = Flipped(Decoupled(new RAMReadReq))
    val ramReadResp = Decoupled(new RAMReadResp)
    val ramWriteReq = Flipped(Decoupled(new RAMWriteReq))
  })

  val ram = Module(new SegmentedRAM(262144))
  val wr  = Module(new SegmentedRAMWrite)
  val rd  = Module(new SegmentedRAMRead)

  ram.io.ram_wr <> wr.io.ram_wr
  ram.io.ram_rd <> rd.io.ram_rd

  io.ramReadReq  <> rd.io.ramReadReq
  io.ramReadResp <> rd.io.ramReadResp
  io.ramWriteReq <> wr.io.ramWriteReq
}

// TODO test writes that are not aligned at 32 bytes!! Maybe the shifting is backwards???

// TODO test both the writers and the readers

class TestSegmentedRAMRead extends AnyFreeSpec {
  "aligned writes, unaligned reads" in {
    simulate(new SimpleRAM) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.ramWriteReq.valid.poke(true.B)
      dut.io.ramWriteReq.ready.expect(true.B)
      dut.io.ramWriteReq.bits.addr.poke(0.U)
      dut.io.ramWriteReq.bits.len.poke(64.U)
      dut.io.ramWriteReq.bits.data.poke("h11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111".U(512.W))

      dut.clock.step()

      dut.io.ramWriteReq.valid.poke(true.B)
      dut.io.ramWriteReq.ready.expect(true.B)
      dut.io.ramWriteReq.bits.addr.poke(64.U)
      dut.io.ramWriteReq.bits.len.poke(64.U)
      dut.io.ramWriteReq.bits.data.poke("h22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222".U(512.W))

      dut.clock.step()

      dut.io.ramWriteReq.valid.poke(true.B)
      dut.io.ramWriteReq.ready.expect(true.B)
      dut.io.ramWriteReq.bits.addr.poke(128.U)
      dut.io.ramWriteReq.bits.len.poke(64.U)
      dut.io.ramWriteReq.bits.data.poke("h33333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333".U(512.W))

      dut.clock.step()

      dut.io.ramWriteReq.valid.poke(true.B)
      dut.io.ramWriteReq.ready.expect(true.B)
      dut.io.ramWriteReq.bits.addr.poke(192.U)
      dut.io.ramWriteReq.bits.len.poke(64.U)
      dut.io.ramWriteReq.bits.data.poke("h44444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444".U(512.W))

      dut.clock.step()

      dut.io.ramWriteReq.valid.poke(false.B)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.ramReadReq.ready.expect(true.B)
      dut.io.ramReadReq.valid.poke(true.B)
      dut.io.ramReadReq.bits.addr.poke(32.U)
      dut.io.ramReadReq.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ramReadReq.ready.expect(true.B)
      dut.io.ramReadReq.valid.poke(true.B)
      dut.io.ramReadReq.bits.addr.poke(160.U)
      dut.io.ramReadReq.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ramReadReq.ready.expect(true.B)
      dut.io.ramReadReq.valid.poke(true.B)
      dut.io.ramReadReq.bits.addr.poke(96.U)
      dut.io.ramReadReq.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ramReadReq.valid.poke(false.B)

      dut.clock.step()
      dut.clock.step()

      dut.io.ramReadResp.valid.expect(true.B)
      dut.io.ramReadResp.ready.poke(true.B)
      dut.io.ramReadResp.bits.data.expect("h22222222222222222222222222222222222222222222222222222222222222221111111111111111111111111111111111111111111111111111111111111111".U)

      dut.clock.step()

      dut.io.ramReadResp.valid.expect(true.B)
      dut.io.ramReadResp.ready.poke(true.B)
      dut.io.ramReadResp.bits.data.expect("h44444444444444444444444444444444444444444444444444444444444444443333333333333333333333333333333333333333333333333333333333333333".U)

      dut.clock.step()

      dut.io.ramReadResp.valid.expect(true.B)
      dut.io.ramReadResp.ready.poke(true.B)
      dut.io.ramReadResp.bits.data.expect("h33333333333333333333333333333333333333333333333333333333333333332222222222222222222222222222222222222222222222222222222222222222".U)

      dut.clock.step()

      dut.io.ramReadReq.ready.expect(true.B)
      dut.io.ramReadReq.valid.poke(true.B)
      dut.io.ramReadReq.bits.addr.poke(16.U)
      dut.io.ramReadReq.bits.len.poke(64.U)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.ramReadResp.valid.expect(true.B)
      dut.io.ramReadResp.ready.poke(true.B)
      dut.io.ramReadResp.bits.data.expect("h22222222222222222222222222222222111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111".U)
    }
  }
}

class TestSegmentedRAMWrite extends AnyFreeSpec {
  "unaligned write test" in {
    simulate(new SimpleRAM) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.ramWriteReq.ready.expect(true.B)
      dut.io.ramWriteReq.valid.poke(true.B)
      dut.io.ramWriteReq.bits.addr.poke(32.U)
      dut.io.ramWriteReq.bits.data.poke("h11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111".U(512.W))
      dut.io.ramWriteReq.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ramWriteReq.valid.poke(false.B)

      dut.clock.step()

      dut.io.ramWriteReq.ready.expect(true.B)
      dut.io.ramWriteReq.valid.poke(true.B)
      dut.io.ramWriteReq.bits.addr.poke(96.U)
      dut.io.ramWriteReq.bits.data.poke("h22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222".U(512.W))
      dut.io.ramWriteReq.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ramWriteReq.valid.poke(false.B)

      dut.clock.step()

      dut.io.ramWriteReq.ready.expect(true.B)
      dut.io.ramWriteReq.valid.poke(true.B)
      dut.io.ramWriteReq.bits.addr.poke(160.U)
      dut.io.ramWriteReq.bits.data.poke("h33333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333".U(512.W))
      dut.io.ramWriteReq.bits.len.poke(64.U)

      dut.clock.step()

      dut.io.ramWriteReq.valid.poke(false.B)

      dut.clock.step()

      dut.io.ramReadReq.ready.expect(true.B)
      dut.io.ramReadReq.valid.poke(true.B)
      dut.io.ramReadReq.bits.addr.poke(32.U(18.W))
      dut.io.ramReadReq.bits.len.poke(64.U)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.ramReadResp.valid.expect(true.B)
      dut.io.ramReadResp.ready.poke(true.B)
      dut.io.ramReadResp.bits.data.expect("h11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111".U)

      dut.clock.step()

      dut.io.ramReadReq.ready.expect(true.B)
      dut.io.ramReadReq.valid.poke(true.B)
      dut.io.ramReadReq.bits.addr.poke(96.U(18.W))
      dut.io.ramReadReq.bits.len.poke(64.U)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.ramReadResp.valid.expect(true.B)
      dut.io.ramReadResp.ready.poke(true.B)
      dut.io.ramReadResp.bits.data.expect("h22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222".U)

      dut.clock.step()
    }
  }

  "aligned partial write" in {
    simulate(new SimpleRAM) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.ramWriteReq.ready.expect(true.B)
      dut.io.ramWriteReq.valid.poke(true.B)
      dut.io.ramWriteReq.bits.addr.poke(0.U)
      dut.io.ramWriteReq.bits.data.poke("h11111111111111111111111111111111".U(512.W))
      dut.io.ramWriteReq.bits.len.poke(16.U)

      dut.clock.step()

      dut.io.ramWriteReq.valid.poke(false.B)

      dut.clock.step()

      dut.io.ramReadReq.ready.expect(true.B)
      dut.io.ramReadReq.valid.poke(true.B)
      dut.io.ramReadReq.bits.addr.poke(0.U)
      dut.io.ramReadReq.bits.len.poke(16.U)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.ramReadResp.valid.expect(true.B)
      dut.io.ramReadResp.ready.poke(true.B)
      dut.io.ramReadResp.bits.data.expect("h11111111111111111111111111111111".U)

      dut.clock.step()
    }
  }


  "aligned multipel partial write" in {
    simulate(new SimpleRAM) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.ramWriteReq.ready.expect(true.B)
      dut.io.ramWriteReq.valid.poke(true.B)
      dut.io.ramWriteReq.bits.addr.poke(64.U)
      dut.io.ramWriteReq.bits.data.poke("h11111111111111111111111111111111".U(512.W))
      dut.io.ramWriteReq.bits.len.poke(16.U)

      dut.clock.step()

      dut.io.ramWriteReq.ready.expect(true.B)
      dut.io.ramWriteReq.valid.poke(true.B)
      dut.io.ramWriteReq.bits.addr.poke(80.U)
      dut.io.ramWriteReq.bits.data.poke("h222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222".U(512.W))
      dut.io.ramWriteReq.bits.len.poke(48.U)

      dut.clock.step()

      dut.io.ramWriteReq.valid.poke(false.B)

      dut.clock.step()

      dut.io.ramReadReq.ready.expect(true.B)
      dut.io.ramReadReq.valid.poke(true.B)
      dut.io.ramReadReq.bits.addr.poke(64.U)
      dut.io.ramReadReq.bits.len.poke(64.U)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.ramReadResp.valid.expect(true.B)
      dut.io.ramReadResp.ready.poke(true.B)
      dut.io.ramReadResp.bits.data.expect("h22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222211111111111111111111111111111111".U)
    }
  }
}
