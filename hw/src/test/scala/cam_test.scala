package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._
import scala.math.pow

class CAMTest extends AnyFreeSpec {
  "cam test: single cam insertion and read" in {
    simulate(new CAM(32,32)) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.insert.bits.key.poke("hffff".U)
      dut.io.insert.bits.value.poke("haaaa".U)
      dut.io.insert.bits.set.poke(1.U)
      dut.io.insert.valid.poke(true.B)

      dut.clock.step()

      dut.io.insert.valid.poke(false.B)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.search.bits.key.poke("hffff".U)
      dut.io.search.valid.poke(true.B)

      dut.clock.step()
      dut.io.search.valid.poke(false.B)

      dut.clock.step()

      dut.io.result.bits.value.expect("haaaa".U)
    }
  }

  "cam test: stress" in {
    simulate(new CAM(32,32)) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      val tvs = Array.fill(2)(0)
      val rnd = new scala.util.Random(99)

      for (i <- 0 to 2-1) {
        val next = rnd.nextInt(294967296)

        tvs(i) = next
        
        dut.io.insert.bits.key.poke((next).U)
        dut.io.insert.bits.value.poke((next).U)
        dut.io.insert.bits.set.poke(1.U)
        dut.io.insert.valid.poke(true.B)
        dut.clock.step()
        dut.clock.step()
        dut.clock.step()
      }

      dut.io.insert.valid.poke(false.B)

      dut.clock.step()

      var count = 0
      for (tv <- tvs) {
        println(count)
        count = count + 1
        dut.clock.step()
 
        dut.io.search.bits.key.poke(tv.U)
        dut.io.search.valid.poke(true.B)
 
        dut.clock.step()
        dut.io.search.valid.poke(false.B)
 
        dut.clock.step()
 
        dut.io.result.bits.value.expect(tv.U)

        dut.clock.step()
        dut.clock.step()
      }
    }
  }
}
