package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._


class CAMTest extends AnyFreeSpec {
  "cam test: Single cam insertion and read" in {
    simulate(new CAM(16,16)) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.insert.key.poke("hffff".U)
      dut.io.insert.value.poke("haaaa".U)
      dut.io.insert.set.poke(1.U)
      dut.io.insert.valid.poke(1.U)

      dut.clock.step()

      dut.io.insert.valid.poke(0.U)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.search.key.poke("hffff".U)
      dut.io.search.valid.poke(1.U)

      dut.clock.step()
      dut.io.search.valid.poke(0.U)

      dut.clock.step()

      dut.io.result.expect("haaaa".U)
    }
  }
}
