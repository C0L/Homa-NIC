package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

class TestSendmsgQueue extends AnyFreeSpec {
  "single entry, single packet out, fully granted, fully dbuffered" in {
    simulate(new SendmsgQueue) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.dequeue.ready.poke(true.B)
      dut.io.enqueue.valid.poke(true.B)
      dut.io.enqueue.ready.expect(true.B)

      dut.io.enqueue.bits.priority.poke(5.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(1.U)
      dut.io.enqueue.bits.dbuff_id.poke(1.U)
      dut.io.enqueue.bits.rpc_id.poke(1.U)

      dut.clock.step()

      dut.io.enqueue.valid.poke(false.B)

      dut.clock.step()
      dut.io.dequeue.bits.remaining.expect(1.U)
      dut.io.dequeue.valid.expect(true.B)

      dut.clock.step()

      dut.io.dequeue.valid.expect(false.B)
    }
  }

  "single entry, single packet out, ungranted, fully dbuffered" in {
    simulate(new SendmsgQueue) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.dequeue.ready.poke(true.B)
      dut.io.enqueue.valid.poke(true.B)
      dut.io.enqueue.ready.expect(true.B)

      dut.io.enqueue.bits.priority.poke(5.U)
      dut.io.enqueue.bits.granted.poke(1.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(1.U)
      dut.io.enqueue.bits.dbuff_id.poke(1.U)
      dut.io.enqueue.bits.rpc_id.poke(1.U)

      dut.clock.step()

      dut.io.enqueue.valid.poke(false.B)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.enqueue.valid.poke(true.B)
      dut.io.enqueue.bits.priority.poke(2.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(1.U)
      dut.io.enqueue.bits.dbuff_id.poke(1.U)
      dut.io.enqueue.bits.rpc_id.poke(1.U)

      dut.clock.step()

      dut.io.enqueue.valid.poke(false.B)

      dut.clock.step()

      dut.io.dequeue.bits.remaining.expect(1.U)
      dut.io.dequeue.valid.expect(true.B)
    }
  }


  "single entry, single packet out, fully granted, undbuffered" in {
    simulate(new SendmsgQueue) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.dequeue.ready.poke(true.B)
      dut.io.enqueue.valid.poke(true.B)
      dut.io.enqueue.ready.expect(true.B)

      dut.io.enqueue.bits.priority.poke(5.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(1.U)
      dut.io.enqueue.bits.remaining.poke(1.U)
      dut.io.enqueue.bits.dbuff_id.poke(1.U)
      dut.io.enqueue.bits.rpc_id.poke(1.U)

      dut.clock.step()

      dut.io.enqueue.valid.poke(false.B)

      dut.clock.step()

      dut.io.enqueue.valid.poke(true.B)
      dut.io.enqueue.bits.priority.poke(1.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(1.U)
      dut.io.enqueue.bits.dbuff_id.poke(1.U)
      dut.io.enqueue.bits.rpc_id.poke(1.U)

      dut.clock.step()

      dut.io.enqueue.valid.poke(false.B)

      dut.clock.step()

      dut.io.dequeue.bits.remaining.expect(1.U)
      dut.io.dequeue.valid.expect(true.B)
    }
  }

  "single entry, multiple packets out, fully granted, fully dbuffered" in {
    simulate(new SendmsgQueue) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.dequeue.ready.poke(true.B)
      dut.io.enqueue.valid.poke(true.B)
      dut.io.enqueue.ready.expect(true.B)

      dut.io.enqueue.bits.priority.poke(5.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(10000.U)
      dut.io.enqueue.bits.dbuff_id.poke(1.U)
      dut.io.enqueue.bits.rpc_id.poke(1.U)

      dut.clock.step()

      dut.io.enqueue.valid.poke(false.B)

      for (count <- 0 to 10000/NetworkCfg.payloadBytes) {
        dut.clock.step()
        dut.io.dequeue.bits.remaining.expect((10000 - (count * NetworkCfg.payloadBytes)).U)
        dut.io.dequeue.valid.expect(true.B)
      }

      dut.clock.step()
      dut.io.dequeue.valid.expect(false.B)
    }
  }


  "two entries, single packets out, fully granted, fully dbuffered" in {
    simulate(new SendmsgQueue) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.dequeue.ready.poke(false.B)
      dut.io.enqueue.valid.poke(true.B)
      dut.io.enqueue.ready.expect(true.B)

      dut.io.enqueue.bits.priority.poke(5.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(5.U)
      dut.io.enqueue.bits.dbuff_id.poke(1.U)
      dut.io.enqueue.bits.rpc_id.poke(1.U)

      dut.clock.step()

      dut.io.enqueue.bits.priority.poke(5.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(2.U)
      dut.io.enqueue.bits.dbuff_id.poke(2.U)
      dut.io.enqueue.bits.rpc_id.poke(2.U)

      dut.clock.step()

      dut.io.enqueue.valid.poke(false.B)

      dut.clock.step()

      dut.io.dequeue.ready.poke(true.B)
      dut.io.dequeue.bits.remaining.expect(2.U)
      dut.io.dequeue.valid.expect(true.B)

      dut.clock.step()

      dut.io.dequeue.bits.remaining.expect(5.U)
      dut.io.dequeue.valid.expect(true.B)
    }
  }


  "two entries, multiple packets out, fully granted, fully dbuffered" in {
    simulate(new SendmsgQueue) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.dequeue.ready.poke(false.B)
      dut.io.enqueue.valid.poke(true.B)
      dut.io.enqueue.ready.expect(true.B)

      dut.io.enqueue.bits.priority.poke(5.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(5000.U)
      dut.io.enqueue.bits.dbuff_id.poke(1.U)
      dut.io.enqueue.bits.rpc_id.poke(1.U)

      dut.clock.step()

      dut.io.enqueue.bits.priority.poke(5.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(2000.U)
      dut.io.enqueue.bits.dbuff_id.poke(2.U)
      dut.io.enqueue.bits.rpc_id.poke(2.U)

      dut.clock.step()

      dut.io.enqueue.valid.poke(false.B)

      dut.clock.step()

      dut.io.dequeue.ready.poke(true.B)
      dut.io.dequeue.bits.remaining.expect(2000.U)
      dut.io.dequeue.valid.expect(true.B)

      dut.clock.step()

      dut.io.dequeue.ready.poke(true.B)
      dut.io.dequeue.bits.remaining.expect((2000 - NetworkCfg.payloadBytes).U)
      dut.io.dequeue.valid.expect(true.B)

      dut.clock.step()

      dut.io.dequeue.bits.remaining.expect(5000.U)
      dut.io.dequeue.valid.expect(true.B)
      dut.clock.step()

      dut.io.dequeue.bits.remaining.expect((5000-NetworkCfg.payloadBytes).U)
      dut.io.dequeue.valid.expect(true.B)
    }
  }

  "two entries, multiple sequential packets out, fully granted, fully dbuffered" in {
    simulate(new SendmsgQueue) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.dequeue.ready.poke(true.B)
      dut.io.enqueue.valid.poke(true.B)
      dut.io.enqueue.ready.expect(true.B)

      dut.io.enqueue.bits.priority.poke(5.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(5000.U)
      dut.io.enqueue.bits.dbuff_id.poke(1.U)
      dut.io.enqueue.bits.rpc_id.poke(1.U)

      dut.clock.step()

      dut.io.enqueue.valid.poke(false.B)

      dut.clock.step()

      dut.io.dequeue.bits.remaining.expect(5000.U)
      dut.io.dequeue.valid.expect(true.B)

      dut.clock.step()

      dut.io.dequeue.bits.remaining.expect((5000-NetworkCfg.payloadBytes).U)
      dut.io.dequeue.valid.expect(true.B)

      dut.clock.step()

      dut.io.dequeue.bits.remaining.expect((5000-NetworkCfg.payloadBytes-NetworkCfg.payloadBytes).U)
      dut.io.dequeue.valid.expect(true.B)

      dut.clock.step()

      dut.io.dequeue.bits.remaining.expect((5000-NetworkCfg.payloadBytes-NetworkCfg.payloadBytes-NetworkCfg.payloadBytes).U)
      dut.io.dequeue.valid.expect(true.B)

      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()
      dut.clock.step()

      dut.io.enqueue.valid.poke(true.B)
      dut.io.enqueue.bits.priority.poke(5.U)
      dut.io.enqueue.bits.granted.poke(0.U)
      dut.io.enqueue.bits.dbuffered.poke(0.U)
      dut.io.enqueue.bits.remaining.poke(5000.U)
      dut.io.enqueue.bits.dbuff_id.poke(2.U)
      dut.io.enqueue.bits.rpc_id.poke(2.U)

      dut.clock.step()

      dut.io.enqueue.valid.poke(false.B)

      dut.clock.step()

      dut.io.dequeue.ready.poke(true.B)
      dut.io.dequeue.bits.remaining.expect(5000.U)
      dut.io.dequeue.valid.expect(true.B)

      dut.clock.step()

      dut.io.dequeue.ready.poke(true.B)
      dut.io.dequeue.bits.remaining.expect((5000 - NetworkCfg.payloadBytes).U)
      dut.io.dequeue.valid.expect(true.B)

      dut.clock.step()
    }
  }
}
