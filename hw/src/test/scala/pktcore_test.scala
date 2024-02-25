package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

class pp_egress_lookup_test extends AnyFreeSpec {
  "pp_lookup_test: packet input triggers RAM read request" in {
    simulate(new pp_egress_lookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
    }
  }

  "pp_lookup_test: RAM read response triggers next stage" in {
    simulate(new pp_egress_lookup) { dut =>

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

  "pp_lookup_test: lack of RAM read response blocks next stage" in {
    simulate(new pp_egress_lookup) { dut =>

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

  "pp_lookup_test: test address out" in {
    simulate(new pp_egress_lookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.rpc_id.poke(100.U)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_desc.bits.ram_addr.expect((100*64).U)
    }
  }

  "pp_lookup_test: test data out" in {
    simulate(new pp_egress_lookup) { dut =>

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

  "pp_lookup_test: test multiple ram reads" in {
    simulate(new pp_egress_lookup) { dut =>

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

      dut.io.ram_read_desc.bits.ram_addr.expect((99*64).U)
      dut.io.ram_read_desc.valid.expect(true.B)

      dut.io.packet_out.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.io.ram_read_data.valid.poke(true.B)
      dut.io.ram_read_data.bits.data.poke("hdeadbeef".U)

      dut.io.ram_read_desc.bits.ram_addr.expect((100*64).U)
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


class pp_egress_dupe_test extends AnyFreeSpec {
  "pp_dupe_test: testing packet duplication" in {
    simulate(new pp_egress_dupe) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

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

  "pp_dupe_test: testing pipelined packet duplication" in {
    simulate(new pp_egress_dupe) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_out.ready.poke(true.B)

      dut.clock.step()
      dut.clock.step()

      // TODO temporary count 
      for (transaction <- 0 to 1500/64) {
        dut.io.packet_out.valid.expect(true.B)
        dut.io.packet_out.bits.frame_off.expect((transaction * 64).U)
        dut.clock.step()
      }

      //  dut.io.packet_in.valid.poke(false.B)

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


class pp_egress_payload_test extends AnyFreeSpec {
  "pp_payload_test: packet input triggers RAM read request" in {
    simulate(new pp_egress_payload) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
    }
  }

  "pp_payload_test: RAM read response triggers next stage" in {
    simulate(new pp_egress_payload) { dut =>

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

  "pp_payload_test: lack of RAM read response blocks next stage" in {
    simulate(new pp_egress_payload) { dut =>

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

  "pp_payload_test: test address out" in {
    simulate(new pp_egress_payload) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.trigger.dbuff_id.poke(3.U)
      dut.io.packet_in.bits.cb.buff_size.poke(100.U)
      dut.io.packet_in.bits.trigger.remaining.poke(90.U)
      dut.io.packet_in.bits.frame_off.poke(1.U)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_desc.bits.ram_addr.expect((3*16384 + 100 - 90 + 0).U)
    }
  }

  "pp_payload_test: test sequenece of addresses out" in {
    simulate(new pp_egress_payload) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.ram_read_data.valid.poke(true.B)

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.trigger.dbuff_id.poke(0.U)
      dut.io.packet_in.bits.cb.buff_size.poke(0.U)
      dut.io.packet_in.bits.trigger.remaining.poke(0.U)
      dut.io.packet_in.bits.frame_off.poke(0.U)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_desc.bits.ram_addr.expect(0.U)

      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.trigger.dbuff_id.poke(0.U)
      dut.io.packet_in.bits.cb.buff_size.poke(0.U)
      dut.io.packet_in.bits.trigger.remaining.poke(0.U)
      dut.io.packet_in.bits.frame_off.poke(64.U)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_desc.bits.ram_addr.expect(0.U)

      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.trigger.dbuff_id.poke(0.U)
      dut.io.packet_in.bits.cb.buff_size.poke(0.U)
      dut.io.packet_in.bits.trigger.remaining.poke(0.U)
      dut.io.packet_in.bits.frame_off.poke(128.U)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_desc.bits.ram_addr.expect(18.U)

      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.trigger.dbuff_id.poke(0.U)
      dut.io.packet_in.bits.cb.buff_size.poke(0.U)
      dut.io.packet_in.bits.trigger.remaining.poke(0.U)
      dut.io.packet_in.bits.frame_off.poke(192.U)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_desc.bits.ram_addr.expect(82.U)
    }
  }

  "pp_payload_test: test data out" in {
    simulate(new pp_egress_payload) { dut =>

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
      dut.io.ram_read_data.bits.data.poke("hdeadbeef".U(512.W))

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
      dut.io.packet_out.bits.payload.expect("hdeadbeef".U(512.W))
    }
  }

  "pp_payload_test: test multiple ram reads" in {
    simulate(new pp_egress_payload) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.io.packet_out.ready.poke(false.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.io.ram_read_desc.valid.expect(true.B)

      dut.io.packet_out.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.io.ram_read_data.valid.poke(true.B)
      dut.io.ram_read_data.bits.data.poke("hdeadbeef".U)

      dut.io.ram_read_desc.valid.expect(true.B)

      dut.io.packet_out.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_out.ready.poke(true.B)
      dut.io.packet_out.valid.expect(true.B)
      dut.io.packet_out.bits.payload.expect("hdeadbeef".U)

      dut.io.ram_read_data.valid.poke(true.B)
      dut.io.ram_read_data.bits.data.poke("hbeefdead".U)

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
      dut.io.packet_out.bits.payload.expect("hbeefdead".U)
    }
  }
}


class pp_egress_ctor_test extends AnyFreeSpec {
  "pp_ctor_test: data in gives data out" in {
    simulate(new pp_egress_ctor) { dut =>
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

  "pp_ctor_test: fully pipelined" in {
    simulate(new pp_egress_ctor) { dut =>
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


class pp_egress_xmit_test extends AnyFreeSpec {
  "pp_xmit_test: data in gives data out" in {
    simulate(new pp_egress_xmit) { dut =>
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

  // class xmit_test_shim extends Module {
  //   val io = IO(new Bundle {
  //     val packet_in  = Flipped(Decoupled(UInt((new PacketFactory).getWidth.W)))
  //     val egress  = new axis(512, false, 0, false, 0, true, 64, true) 
  //   })

  //   val dut = Module(new pp_egress_xmit)
  //   dut.io.packet_in.bits  := io.packet_in.bits.asTypeOf(new PacketFactory)
  //   dut.io.packet_in.valid := io.packet_in.valid
  //   io.packet_in.ready     := dut.io.packet_in.ready
  //   dut.io.egress          <> io.egress
  // }

  "pp_xmit_test: first 64 bytes out" in {
    simulate(new pp_egress_xmit) { dut =>
      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.egress.tready.poke(true.B)

      // dut.io.packet_in.bits.poke("hdeadbeef".U)
      dut.io.packet_in.bits.eth.mac_dest.poke("hdeadbeefe".U)
      dut.io.packet_in.bits.frame_off.poke(0.U)
      dut.io.egress.tready.poke(true.B)

      dut.clock.step()

      dut.io.egress.tvalid.expect(true.B)
      dut.io.egress.tdata.expect("hdeadbeefe00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000".U)
    }
  }

  "pp_xmit_test: first 128 bytes out" in {
    simulate(new pp_egress_xmit) { dut =>
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
      dut.io.egress.tdata.expect("hdead0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000".U)
    }
  }

  "pp_xmit_test: payload out" in {
    simulate(new pp_egress_xmit) { dut =>
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

  "pp_xmit_test: testing tlast assertion" in {
    simulate(new pp_egress_xmit) { dut =>
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


class pp_ingress_dtor_test extends AnyFreeSpec {

  "pp_dtor_test: data in gives data out" in {
    simulate(new pp_ingress_dtor) { dut =>
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

  "pp_dtor_test: testing first block packet header reconstruction" in {
    simulate(new pp_ingress_dtor) { dut =>
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
      dut.io.packet_out.bits.eth.mac_dest.expect("hdeadbeefd".U)
      dut.io.packet_out.valid.expect(true.B)

      dut.clock.step()
    }
  }

  "pp_dtor_test: testing second block packet header reconstruction" in {
    simulate(new pp_ingress_dtor) { dut =>
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
      dut.io.packet_out.bits.eth.mac_dest.expect("hdeadbeefd".U)
      dut.io.packet_out.bits.common.unused3.expect("hdead".U)
      dut.io.packet_out.valid.expect(true.B)

      dut.clock.step()
    }
  }


  "pp_dtor_test: testing payload" in {
    simulate(new pp_ingress_dtor) { dut =>
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
      dut.io.packet_out.bits.eth.mac_dest.expect("hdeadbeefd".U)
      dut.io.packet_out.bits.common.unused3.expect("hdead".U)
      dut.io.packet_out.bits.payload.expect("hdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef".U)
      dut.io.packet_out.valid.expect(true.B)

      dut.clock.step()
    }
  }
}

class pp_ingress_lookup_test extends AnyFreeSpec {
  "pp_lookup_test: packet input triggers RAM read request" in {
    simulate(new pp_ingress_lookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
    }
  }

  "pp_lookup_test: RAM read response triggers next stage" in {
    simulate(new pp_ingress_lookup) { dut =>

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

  "pp_lookup_test: lack of RAM read response blocks next stage" in {
    simulate(new pp_ingress_lookup) { dut =>

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

  "pp_lookup_test: test address out" in {
    simulate(new pp_ingress_lookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.cb.id.poke(100.U)

      dut.clock.step()

      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_desc.bits.ram_addr.expect((100*64).U)
    }
  }

  "pp_lookup_test: test data out" in {
    simulate(new pp_ingress_lookup) { dut =>

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

  "pp_lookup_test: test multiple ram reads" in {
    simulate(new pp_ingress_lookup) { dut =>

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

      dut.io.ram_read_desc.bits.ram_addr.expect((99*64).U)
      dut.io.ram_read_desc.valid.expect(true.B)

      dut.io.packet_out.valid.expect(false.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.io.ram_read_data.valid.poke(true.B)
      dut.io.ram_read_data.bits.data.poke("hdeadbeef".U)

      dut.io.ram_read_desc.bits.ram_addr.expect((100*64).U)
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


class pp_ingress_payload_test extends AnyFreeSpec {
  "pp_payload_test: data in gives data out" in {
    simulate(new pp_ingress_payload) { dut =>

       dut.reset.poke(true.B)
       dut.clock.step()
       dut.reset.poke(false.B)
       dut.clock.step()

       dut.io.packet_in.valid.poke(true.B)

       dut.clock.step()

       dut.io.dma_w_data.valid.expect(true.B)
     }
   }

  "pp_payload_test: pcie write addr" in {
    simulate(new pp_ingress_payload) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_in.bits.data.data.offset.poke(100.U)
      dut.io.packet_in.bits.frame_off.poke(256.U)

      dut.clock.step()

      dut.io.dma_w_data.bits.pcie_write_addr.expect(246.U)
      dut.io.dma_w_data.valid.expect(true.B)
    }
  }
}
