package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

class SendmsgQueueTest extends AnyFreeSpec {
  "sendmsg_queue: single entry, single packet out, fully granted, fully dbuffered" in {
    simulate(new SendmsgQueue) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.enqueue.valid.poke(true.B)

      dut.io.enqueue.bits.priority.poke(5.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(1000.U)
      dut.io.enqueue.bits.dbuff_id.poke(1.U)
      dut.io.enqueue.bits.rpc_id.poke(1.U)

      dut.io.enqueue.ready.expect(true.B)

      dut.clock.step()

      dut.io.dequeue.ready.poke(true.B)
      dut.io.dequeue.bits.priority.expect(5.U)
      dut.io.dequeue.bits.granted.expect(0.U)
      dut.io.dequeue.bits.dbuffered.expect(0.U)
      // dut.io.dequeue.bits.remaining.expect(5.U)
      dut.io.dequeue.bits.dbuff_id.expect(1.U)
      dut.io.dequeue.bits.rpc_id.expect(1.U)
      dut.io.dequeue.valid.expect(true.B)
    }
  }
}
