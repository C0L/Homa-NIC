package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

class pp_egress_lookup_test extends AnyFreeSpec {
  "pp_lookup_test: read reaquest dispatch" in {
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

  "pp_lookup_test: read response triggers next stage" in {
    simulate(new pp_egress_lookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.ram_read_desc.valid.expect(true.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(false.B)
      dut.io.ram_read_data.valid.poke(true.B)

      dut.clock.step()

      dut.io.packet_out.valid.expect(true.B)
    }
  }

  "pp_lookup_test: testing multiple frame emission" in {
    simulate(new pp_egress_lookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)

      dut.clock.step()

      dut.io.packet_in.valid.poke(false.B)
      dut.io.ram_read_data.valid.poke(true.B)
      dut.io.packet_out.ready.poke(true.B)
      
      for (transaction <- 0 to 1386/64) {
        dut.io.packet_out.valid.expect(true.B)

        dut.clock.step()
      }
    }
  }

  "pp_lookup_test: test address out" in {
    simulate(new pp_egress_lookup) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.packet_in.valid.poke(true.B)
      dut.io.packet_out.ready.poke(true.B)
      dut.io.ram_read_desc.ready.poke(false.B)
      dut.io.packet_in.bits.rpc_id.poke(100.U)

      // dut.io.packet_in.valid.poke(false.B)
      dut.io.ram_read_desc.valid.expect(true.B)
      dut.io.ram_read_desc.bits.ram_addr.expect((100*64).U)
    }
  }
}

// class pp_egress_stages_test extends AnyFreeSpec {
//   "pp_egress_stages: test complete flow" in {
//     simulate(new pp_egress_stages) { dut =>
// 
//       dut.reset.poke(true.B)
//       dut.clock.step()
//       dut.reset.poke(false.B)
//       dut.clock.step()
// 
// 
//   }
// }