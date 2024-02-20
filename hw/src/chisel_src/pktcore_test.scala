package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

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
      for (transaction <- 0 to 1386/64) {
        dut.io.packet_out.valid.expect(true.B)
        // TODO check frame number
        dut.clock.step()
      }
      
      dut.io.packet_out.valid.expect(false.B)
    }
  }

  //  "pp_dupe_test: testing packet duplication input block" in {
  //    simulate(new pp_egress_dupe) { dut =>
  //
  //      dut.reset.poke(true.B)
  //      dut.clock.step()
  //      dut.reset.poke(false.B)
  //      dut.clock.step()
  //
  //      dut.io.packet_in.valid.poke(true.B)
  //
  //      dut.clock.step()
  //
  //      dut.io.packet_in.valid.poke(false.B)
  //      dut.io.packet_out.ready.poke(true.B)
  //
  //      dut.clock.step()
  //
  //      // TODO temporary count
  //      for (transaction <- 0 to 1386/64) {
  //        dut.io.packet_out.valid.expect(true.B)
  //        // TODO check frame number
  //        dut.clock.step()
  //      }
  //
  //      // dut.io.packet_out.valid.expect(false.B)
  //    }
  // }

  // Test short legnths and long lengths
}
