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
      dut.io.insert.bits.tag.poke("hdeadbeef".U)
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

  "TestCAM: search for an item that doesnt exist" in {
    simulate(new CAM(32,32)) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.search.bits.key.poke("hffff".U)
      dut.io.search.valid.poke(true.B)

      dut.clock.step()

      dut.io.search.valid.poke(false.B)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.result.valid.expect(true.B)
    }
  }

  "cam test: stress" in {
    simulate(new CAM(32,32)) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      val tvs = Array.fill(5000)(0)
      val rnd = new scala.util.Random(9)

      for (i <- 0 to 5000-1) {
        val next = rnd.nextInt(294967296)
        println("NEXT RAND", next)
        tvs(i) = next
        
        dut.io.insert.bits.key.poke(next.U)
        dut.io.insert.bits.value.poke(next.U)
        dut.io.insert.bits.set.poke(1.U)
        dut.io.insert.bits.tag.poke("hdeadbeef".U)
        dut.io.insert.valid.poke(true.B)
        // dut.clock.step()
        // dut.io.insert.valid.poke(false.B)
        // dut.clock.step()
        dut.clock.step()
      }

      dut.io.insert.valid.poke(false.B)

      dut.clock.step()

      var count = 0
      for (tv <- tvs) {
        println(count)
        println(tv)
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
