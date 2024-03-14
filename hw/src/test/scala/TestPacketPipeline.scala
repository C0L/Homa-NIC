package gpnic

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

class TestPPegressLookup extends AnyFreeSpec {
  "packet input triggers RAM read request" in {
    simulate(new PPegressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
    }
  }

  "RAM read response triggers next stage" in {
    simulate(new PPegressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(false.B)
      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_data.valid.poke(true.B)

      dut.clock.step()
      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
    }
  }

  "lack of RAM read response blocks next stage" in {
    simulate(new PPegressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(false.B)
      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_data.valid.poke(false.B)

      dut.clock.step()
      dut.clock.step()

      dut.io.packet_out.valid.expect(false.B)
    }
  }

  "test address out" in {
    simulate(new PPegressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.rpc_id.poke(100.U)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_desc.bits.addr.expect((100*64).U)
    }
  }

  "test data out" in {
    simulate(new PPegressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(false.B)
      dut.io.ram_read_desc.valid.expect(true.B)

      dut.clock.step()

      dut.io.ram_read_data.valid.poke(true.B)
      dut.io.ram_read_data.bits.data.poke("hdeadbeef".U)

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
      dut.io.packet_out.bits.cb.unused.expect("hdeadbeef".U)
    }
  }

  "test multiple ram reads" in {
    simulate(new PPegressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.rpc_id.poke(99.U)

      dut.io.packet_out.ready.poke(false.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.rpc_id.poke(100.U)

      dut.io.ram_read_desc.bits.addr.expect((99*64).U)
      dut.io.ram_read_desc.valid.expect(true.B)

      dut.io.packet_out.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.io.ram_read_data.valid.poke(true.B)
      dut.io.ram_read_data.bits.data.poke("hdeadbeef".U)

      dut.io.ram_read_desc.bits.addr.expect((100*64).U)
      dut.io.ram_read_desc.valid.expect(true.B)

      dut.io.packet_out.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.packet_out.valid.expect(true.B)
      dut.io.packet_out.bits.cb.unused.expect("hdeadbeef".U)

      dut.io.ram_read_data.valid.poke(true.B)
      dut.io.ram_read_data.bits.data.poke("hbeefdead".U)

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
      dut.io.packet_out.bits.cb.unused.expect("hbeefdead".U)
    }
  }
}

class TestPPegressDupe extends AnyFreeSpec {
  "testing packet duplication" in {
    simulate(new PPegressDupe) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.data.data.seg_len.poke(1386.U)

      dut.clock.step()

      dut.io.packet_in.valid.poke(false.B)
      dut.io.packet_out.ready.poke(true.B)

      dut.clock.step()

      // TODO temporary count
      for (transaction <- 0 to 1500/64) {
        dut.io.packet_out.valid.expect(true.B)
        dut.io.packet_out.bits.frame_off.expect((transaction * 64).U)
        dut.clock.step()
      }
      
      dut.io.packet_out.valid.expect(false.B)
    }
  }

  "testing pipelined packet duplication" in {
    simulate(new PPegressDupe) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_out.ready.poke(true.B)
      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.data.data.seg_len.poke(1386.U)

      dut.clock.step()
      dut.clock.step()

      // TODO temporary count
      for (transaction <- 0 to 1500/64) {
        dut.io.packet_out.valid.expect(true.B)
        dut.io.packet_out.bits.frame_off.expect((transaction * 64).U)
        dut.clock.step()
      }

      // TODO temporary count
      for (transaction <- 0 to 1500/64) {
        dut.io.packet_out.valid.expect(true.B)
        dut.io.packet_out.bits.frame_off.expect((transaction * 64).U)
        dut.clock.step()
      }
      
      // dut.io.packet_out.valid.expect(false.B)
    }
  }

  // Test short legnths and long lengths
}

// TODO return
// class TestPPegressPayload extends AnyFreeSpec {
//   "packet input triggers RAM read request" in {
//     simulate(new PPegressPayload) { dut =>
// 
//       dut.reset.poke(true.B)
//       dut.clock.step()
//       dut.reset.poke(false.B)
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(true.B)
// 
//       dut.clock.step()
// 
//       dut.io.ram_read_desc.valid.expect(true.B)
//     }
//   }
// 
//   "RAM read response triggers next stage" in {
//     simulate(new PPegressPayload) { dut =>
// 
//       dut.reset.poke(true.B)
//       dut.clock.step()
//       dut.reset.poke(false.B)
//       dut.clock.step()
// 
//       dut.io.packet_out.ready.poke(true.B)
//       dut.io.packet_in.valid.poke(true.B)
// 
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(false.B)
//       dut.io.ram_read_desc.valid.expect(true.B)
//       dut.io.ram_read_data.valid.poke(true.B)
// 
//       dut.clock.step()
//       dut.clock.step()
// 
//       dut.io.packet_out.valid.expect(true.B)
//     }
//   }
// 
//   "lack of RAM read response blocks next stage" in {
//     simulate(new PPegressPayload) { dut =>
// 
//       dut.reset.poke(true.B)
//       dut.clock.step()
//       dut.reset.poke(false.B)
//       dut.clock.step()
// 
//       dut.io.packet_out.ready.poke(true.B)
//       dut.io.packet_in.valid.poke(true.B)
// 
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(false.B)
//       dut.io.ram_read_desc.valid.expect(true.B)
//       dut.io.ram_read_data.valid.poke(false.B)
// 
//       dut.clock.step()
//       dut.clock.step()
// 
//       dut.io.packet_out.valid.expect(false.B)
//     }
//   }
// 
//   "test address out" in {
//     simulate(new PPegressPayload) { dut =>
// 
//       dut.reset.poke(true.B)
//       dut.clock.step()
//       dut.reset.poke(false.B)
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(true.B)
//       dut.io.packet_in.bits.trigger.dbuff_id.poke(3.U)
//       dut.io.packet_in.bits.cb.buff_size.poke(100.U)
//       dut.io.packet_in.bits.trigger.remaining.poke(90.U)
//       dut.io.packet_in.bits.frame_off.poke(1.U)
// 
//       dut.clock.step()
// 
//       dut.io.ram_read_desc.valid.expect(true.B)
//       dut.io.ram_read_desc.bits.addr.expect((3*16384 + 100 - 90 + 0).U)
//     }
//   }
// 
//   "test sequenece of addresses out" in {
//     simulate(new PPegressPayload) { dut =>
// 
//       dut.reset.poke(true.B)
//       dut.clock.step()
//       dut.reset.poke(false.B)
//       dut.clock.step()
// 
//       dut.io.ram_read_data.valid.poke(true.B)
// 
//       dut.io.packet_in.valid.poke(true.B)
//       dut.io.packet_in.bits.trigger.dbuff_id.poke(0.U)
//       dut.io.packet_in.bits.cb.buff_size.poke(0.U)
//       dut.io.packet_in.bits.trigger.remaining.poke(0.U)
//       dut.io.packet_in.bits.frame_off.poke(0.U)
// 
//       dut.clock.step()
// 
//       dut.io.ram_read_desc.valid.expect(true.B)
//       dut.io.ram_read_desc.bits.addr.expect(0.U)
// 
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(true.B)
//       dut.io.packet_in.bits.trigger.dbuff_id.poke(0.U)
//       dut.io.packet_in.bits.cb.buff_size.poke(0.U)
//       dut.io.packet_in.bits.trigger.remaining.poke(0.U)
//       dut.io.packet_in.bits.frame_off.poke(64.U)
// 
//       dut.clock.step()
// 
//       dut.io.ram_read_desc.valid.expect(true.B)
//       dut.io.ram_read_desc.bits.addr.expect(0.U)
// 
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(true.B)
//       dut.io.packet_in.bits.trigger.dbuff_id.poke(0.U)
//       dut.io.packet_in.bits.cb.buff_size.poke(0.U)
//       dut.io.packet_in.bits.trigger.remaining.poke(0.U)
//       dut.io.packet_in.bits.frame_off.poke(128.U)
// 
//       dut.clock.step()
// 
//       dut.io.ram_read_desc.valid.expect(true.B)
//       dut.io.ram_read_desc.bits.addr.expect(14.U)
// 
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(true.B)
//       dut.io.packet_in.bits.trigger.dbuff_id.poke(0.U)
//       dut.io.packet_in.bits.cb.buff_size.poke(0.U)
//       dut.io.packet_in.bits.trigger.remaining.poke(0.U)
//       dut.io.packet_in.bits.frame_off.poke(192.U)
// 
//       dut.clock.step()
// 
//       dut.io.ram_read_desc.valid.expect(true.B)
//       dut.io.ram_read_desc.bits.addr.expect(78.U)
//     }
//   }
// 
//   "test data out" in {
//     simulate(new PPegressPayload) { dut =>
// 
//       dut.reset.poke(true.B)
//       dut.clock.step()
//       dut.reset.poke(false.B)
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(true.B)
// 
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(false.B)
//       dut.io.ram_read_desc.valid.expect(true.B)
// 
//       dut.clock.step()
// 
//       dut.io.ram_read_data.valid.poke(true.B)
//       dut.io.ram_read_data.bits.data.poke("hdeadbeef".U(512.W))
// 
//       dut.clock.step()
// 
//       dut.io.packet_out.valid.expect(true.B)
//       dut.io.packet_out.bits.payload.expect("hdeadbeef".U(512.W))
//     }
//   }
// 
//   "test multiple ram reads" in {
//     simulate(new PPegressPayload) { dut =>
// 
//       dut.reset.poke(true.B)
//       dut.clock.step()
//       dut.reset.poke(false.B)
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(true.B)
// 
//       dut.io.packet_out.ready.poke(false.B)
// 
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(true.B)
// 
//       dut.io.ram_read_desc.valid.expect(true.B)
// 
//       dut.io.packet_out.valid.expect(false.B)
// 
//       dut.clock.step()
// 
//       dut.io.packet_in.valid.poke(true.B)
// 
//       dut.io.ram_read_data.valid.poke(true.B)
//       dut.io.ram_read_data.bits.data.poke("hdeadbeef".U)
// 
//       dut.io.ram_read_desc.valid.expect(true.B)
// 
//       dut.io.packet_out.valid.expect(false.B)
// 
//       dut.clock.step()
// 
//       dut.io.packet_out.ready.poke(true.B)
//       dut.io.packet_out.valid.expect(true.B)
//       dut.io.packet_out.bits.payload.expect("hdeadbeef".U)
// 
//       dut.io.ram_read_data.valid.poke(true.B)
//       dut.io.ram_read_data.bits.data.poke("hbeefdead".U)
// 
//       dut.clock.step()
// 
//       dut.io.packet_out.valid.expect(true.B)
//       dut.io.packet_out.bits.payload.expect("hbeefdead".U)
//     }
//   }
// }


class TestPPegressCtor extends AnyFreeSpec {
  "data in gives data out" in {
    simulate(new PPegressCtor) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(false.B)

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
    }
  }

  "fully pipelined" in {
    simulate(new PPegressCtor) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()
      dut.clock.step()
      dut.io.packet_out.valid.expect(true.B)
      dut.clock.step()
      dut.io.packet_out.valid.expect(true.B)
      dut.clock.step()
      dut.io.packet_out.valid.expect(true.B)
      dut.clock.step()
      dut.io.packet_out.valid.expect(true.B)
      dut.clock.step()
      dut.io.packet_out.valid.expect(true.B)
      dut.clock.step()
      dut.io.packet_out.valid.expect(true.B)
    }
  }
}


class TestPPegressXmit extends AnyFreeSpec {
  "data in gives data out" in {
    simulate(new PPegressXmit) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.egress.tready.poke(true.B)

      dut.clock.step()

      dut.io.egress.tvalid.expect(true.B)
    }
  }

  "first 64 bytes out" in {
    simulate(new PPegressXmit) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.egress.tready.poke(true.B)

      dut.io.packet_in.bits.eth.mac_dest.poke("hdeadbeefdead".U)
      dut.io.packet_in.bits.frame_off.poke(0.U)
      dut.io.egress.tready.poke(true.B)

      dut.clock.step()

      dut.io.egress.tvalid.expect(true.B)
      dut.io.egress.tdata.expect("hdeadbeefdeaf00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000".U)
    }
  }

  "first 128 bytes out" in {
    simulate(new PPegressXmit) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.egress.tready.poke(true.B)

      dut.io.packet_in.bits.common.unused3.poke("hdead".U)
      dut.io.packet_in.bits.frame_off.poke(64.U)
      dut.io.egress.tready.poke(true.B)

      dut.clock.step()

      // dut.io.egress.tlast.get.expect(false.B)
      dut.io.egress.tvalid.expect(true.B)
      dut.io.egress.tdata.expect("h00dead00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000".U)
    }
  }

  "payload out" in {
    simulate(new PPegressXmit) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()
 
      dut.io.packet_in.valid.poke(true.B)
      dut.io.egress.tready.poke(true.B)

      dut.io.packet_in.bits.payload.poke("hdeadbeef".U)
      dut.io.packet_in.bits.frame_off.poke(128.U)
      dut.io.egress.tready.poke(true.B)

      dut.clock.step()

      // dut.io.egress.tlast.get.expect(false.B)
      dut.io.egress.tvalid.expect(true.B)
      dut.io.egress.tdata.expect("hdeadbeef".U)
    }
  }

  "testing tlast assertion" in {
    simulate(new PPegressXmit) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.egress.tready.poke(true.B)

      dut.io.packet_in.bits.payload.poke("hdeadbeef".U)
      dut.io.packet_in.bits.data.data.seg_len.poke(1386.U)
      dut.io.packet_in.bits.frame_off.poke((23*64).U) // TODO eventually this will be based on seg length
      dut.io.egress.tready.poke(true.B)

      dut.clock.step()

      dut.io.egress.tlast.get.expect(true.B)
      dut.io.egress.tvalid.expect(true.B)
      dut.io.egress.tdata.expect("hdeadbeef".U)
    }
  }
}


class TestPPegressDtor extends AnyFreeSpec {

  "data in gives data out" in {
    simulate(new PPingressDtor) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.ingress.tvalid.poke(true.B)

      dut.clock.step()

      dut.io.packet_out.valid.expect(false.B)
      dut.io.ingress.tvalid.poke(true.B)

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
      dut.io.ingress.tvalid.poke(false.B)
    }
  }

  "testing first block packet header reconstruction" in {
    simulate(new PPingressDtor) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.ingress.tvalid.poke(true.B)
      dut.io.ingress.tdata.poke("hdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef".U)

      dut.clock.step()

      dut.io.packet_out.valid.expect(false.B)
      dut.io.ingress.tvalid.poke(true.B)

      dut.clock.step()

      dut.io.packet_out.bits.frame_off.expect(64.U)
      dut.io.packet_out.bits.eth.mac_dest.expect("hdeadbeefdead".U)
      dut.io.packet_out.valid.expect(true.B)

      dut.clock.step()
    }
  }

  "testing second block packet header reconstruction" in {
    simulate(new PPingressDtor) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.ingress.tvalid.poke(true.B)
      dut.io.ingress.tdata.poke("hdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef".U)

      dut.clock.step()

      dut.io.packet_out.valid.expect(false.B)
      dut.io.ingress.tvalid.poke(true.B)
      dut.io.ingress.tdata.poke("hdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef".U)

      dut.clock.step()

      dut.io.packet_out.bits.frame_off.expect(64.U)
      dut.io.packet_out.bits.eth.mac_dest.expect("hdeadbeefdead".U)
      dut.io.packet_out.bits.common.unused3.expect("hdead".U)
      dut.io.packet_out.valid.expect(true.B)

      dut.clock.step()
    }
  }

  "testing payload" in {
    simulate(new PPingressDtor) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.ingress.tvalid.poke(true.B)
      dut.io.ingress.tdata.poke("hdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef".U)

      dut.clock.step()

      dut.io.packet_out.valid.expect(false.B)
      dut.io.ingress.tvalid.poke(true.B)
      dut.io.ingress.tdata.poke("hdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef".U)

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
      dut.io.ingress.tvalid.poke(true.B)
      dut.io.ingress.tdata.poke("hdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef".U)

      dut.clock.step()

      dut.io.packet_out.bits.frame_off.expect(128.U)
      dut.io.packet_out.bits.eth.mac_dest.expect("hdeadbeefdead".U)
      dut.io.packet_out.bits.common.unused3.expect("hdead".U)
      dut.io.packet_out.bits.payload.expect("hdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef".U)
      dut.io.packet_out.valid.expect(true.B)

      dut.clock.step()
    }
  }
}

class TestPPingressLookup extends AnyFreeSpec {
  "packet input triggers RAM read request" in {
    simulate(new PPingressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
    }
  }

  "RAM read response triggers next stage" in {
    simulate(new PPingressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.packet_in.valid.poke(true.B)
      dut.io.ram_read_desc.valid.expect(true.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(false.B)
      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_data.valid.poke(true.B)

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
      dut.io.ram_read_data.valid.poke(true.B)

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
      dut.io.ram_read_data.valid.poke(false.B)


    }
  }

  "lack of RAM read response blocks next stage" in {
    simulate(new PPingressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(false.B)
      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_data.valid.poke(false.B)

      dut.clock.step()
      dut.clock.step()

      dut.io.packet_out.valid.expect(false.B)
    }
  }

  "test address out" in {
    simulate(new PPingressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.cb.id.poke(100.U)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_desc.bits.addr.expect((100*64).U)
    }
  }

  "test data out" in {
    simulate(new PPingressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(false.B)
      dut.io.ram_read_desc.valid.expect(true.B)

      dut.clock.step()

      dut.io.ram_read_data.valid.poke(true.B)
      dut.io.ram_read_data.bits.data.poke("hdeadbeef".U)

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
      dut.io.packet_out.bits.cb.unused.expect("hdeadbeef".U)
    }
  }

  "test multiple ram reads" in {
    simulate(new PPingressLookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.cb.id.poke(99.U)

      dut.io.packet_out.ready.poke(false.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.cb.id.poke(100.U)

      dut.io.ram_read_desc.bits.addr.expect((99*64).U)
      dut.io.ram_read_desc.valid.expect(true.B)

      dut.io.packet_out.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.io.ram_read_data.valid.poke(true.B)
      dut.io.ram_read_data.bits.data.poke("hdeadbeef".U)

      dut.io.ram_read_desc.bits.addr.expect((100*64).U)
      dut.io.ram_read_desc.valid.expect(true.B)

      dut.io.packet_out.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.packet_out.valid.expect(true.B)
      dut.io.packet_out.bits.cb.unused.expect("hdeadbeef".U)

      dut.io.ram_read_data.valid.poke(true.B)
      dut.io.ram_read_data.bits.data.poke("hbeefdead".U)

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
      dut.io.packet_out.bits.cb.unused.expect("hbeefdead".U)
    }
  }
}

class TestPPingressPayload extends AnyFreeSpec {
  "data in gives data out" in {
    simulate(new PPingressPayload) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.bits.frame_off.poke(64.U)
      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.c2hPayloadDmaReq.valid.expect(true.B)
    }
  }

  "pcie write addr 64 write buffer" in {
    simulate(new PPingressPayload) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.dynamicConfiguration.writeBufferSize.poke(64.U)

      // TODO check RAM out and DMA out
      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.data.data.seg_len.poke(1500.U)
      dut.io.packet_in.bits.data.data.offset.poke(0.U)
      dut.io.packet_in.bits.frame_off.poke(0.U)

      dut.clock.step()

      dut.io.packet_in.bits.frame_off.poke(64.U)
      dut.io.c2hPayloadRamReq.valid.expect(false.B)
      dut.io.c2hPayloadDmaReq.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_in.bits.frame_off.poke(128.U)
      dut.io.c2hPayloadRamReq.valid.expect(true.B)
      dut.io.c2hPayloadDmaReq.valid.expect(false.B)

      dut.clock.step()

      dut.io.c2hPayloadRamReq.valid.expect(true.B)
      dut.io.c2hPayloadDmaReq.valid.expect(true.B)
      dut.io.c2hPayloadDmaReq.bits.pcie_addr.expect(0.U)
      dut.io.c2hPayloadDmaReq.bits.ram_addr.expect(0.U)


      // dut.io.packet_in.bits.frame_off.poke(196.U)
      // dut.io.c2hPayloadRamReq.valid.expect(true.B)
      // dut.io.c2hPayloadDmaReq.valid.expect(false.B)

      // dut.clock.step()

      // dut.io.packet_in.bits.frame_off.poke(256.U)
      // dut.io.c2hPayloadRamReq.valid.expect(true.B)
      // dut.io.c2hPayloadDmaReq.valid.expect(false.B)

      // dut.clock.step()

      // dut.io.packet_in.bits.frame_off.poke(320.U)
      // dut.io.c2hPayloadRamReq.valid.expect(true.B)
      // dut.io.c2hPayloadDmaReq.valid.expect(false.B)

      // dut.clock.step()

      // dut.io.c2hPayloadRamReq.valid.expect(true.B)
      // dut.io.c2hPayloadDmaReq.valid.expect(true.B)
      // dut.io.c2hPayloadDmaReq.bits.pcie_addr.expect(0.U)

      // dut.clock.step()

      // dut.io.packet_in.bits.frame_off.poke(64.U)

      // dut.io.dynamicConfiguration.writeBufferSize.poke(8.U)

      // dut.io.packet_in.bits.data.data.offset.poke(80.U)

      // dut.clock.step()

      // dut.io.c2hPayloadRamReq.valid.expect(true.B)
      // dut.io.c2hPayloadDmaReq.valid.expect(true.B)
    }
  }

  "pcie write addr 256 write buffer" in {
    simulate(new PPingressPayload) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.dynamicConfiguration.writeBufferSize.poke(256.U)

      // TODO check RAM out and DMA out
      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.data.data.seg_len.poke(1500.U)
      dut.io.packet_in.bits.data.data.offset.poke(0.U)
      dut.io.packet_in.bits.frame_off.poke(0.U)

      dut.clock.step()

      dut.io.packet_in.bits.frame_off.poke(64.U)
      dut.io.c2hPayloadRamReq.valid.expect(false.B)
      dut.io.c2hPayloadDmaReq.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_in.bits.frame_off.poke(128.U)
      dut.io.c2hPayloadRamReq.valid.expect(true.B)
      dut.io.c2hPayloadDmaReq.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_in.bits.frame_off.poke(196.U)
      dut.io.c2hPayloadRamReq.valid.expect(true.B)
      dut.io.c2hPayloadDmaReq.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_in.bits.frame_off.poke(256.U)
      dut.io.c2hPayloadRamReq.valid.expect(true.B)
      dut.io.c2hPayloadDmaReq.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_in.bits.frame_off.poke(320.U)
      dut.io.c2hPayloadRamReq.valid.expect(true.B)
      dut.io.c2hPayloadDmaReq.valid.expect(false.B)

      dut.clock.step()

      dut.io.c2hPayloadRamReq.valid.expect(true.B)
      dut.io.c2hPayloadDmaReq.valid.expect(true.B)
      dut.io.c2hPayloadDmaReq.bits.pcie_addr.expect(0.U)

      dut.clock.step()

      dut.io.packet_in.bits.frame_off.poke(64.U)

      dut.io.dynamicConfiguration.writeBufferSize.poke(8.U)

      dut.io.packet_in.bits.data.data.offset.poke(80.U)

      dut.clock.step()

      dut.io.c2hPayloadRamReq.valid.expect(true.B)
      dut.io.c2hPayloadDmaReq.valid.expect(true.B)
    }
  }
}
