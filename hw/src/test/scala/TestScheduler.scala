package gpnic

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._


class TestScheduler extends AnyFreeSpec {
  "test SRPT ordering" in {
    simulate(new Scheduler) { dut =>
      def newMessage(length): {
        dut.io.newMessage.ready.poke(true.B)
        dut.io.newMessage.valid.poke(true.B)
        dut.io.newMessage.bits.totalLen.poke(length.U)
      }

      /* Reset the core */
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      // don't accept outputs yet
      dut.io.dequeue.ready.poke(false.b) 

      for (i <- 1 to 32) {
        newMessage(i * 1000)
        dut.clock.step()
      }

      // don't accept outputs yet
      dut.io.dequeue.ready.poke(true.B) 
      dut.io.dequeue.valid.poke(false.B) 
    }
  }
}
