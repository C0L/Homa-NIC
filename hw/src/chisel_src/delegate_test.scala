package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

class delegate_test extends AnyFreeSpec {

  "delegate_test: testing dma map" in {
    simulate(new delegate) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.function_i.tready.expect(true.B)

      dut.io.function_i.tvalid.poke(true.B)
      dut.io.function_i.tdata.poke("h01AABBDEADBEEFDEADBEEF".U)
      dut.io.function_i.tuser.get.poke(0.U)

      dut.clock.step()

      dut.io.function_i.tvalid.poke(false.B)

      dut.io.dma_map_o.ready.poke(true.B)
      dut.io.dma_map_o.valid.expect(true.B)
      dut.io.dma_map_o.bits.pcie_addr.expect("hDEADBEEFDEADBEEF".U)
      dut.io.dma_map_o.bits.port.expect("hAABB".U)
      dut.io.dma_map_o.bits.map_type.expect(1.U)

      dut.clock.step()

      dut.io.dma_map_o.valid.expect(false.B)
    }
  }

  "delegate_test: testing virtualization" in {
    simulate(new delegate) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.function_i.tready.expect(true.B)

      dut.io.function_i.tvalid.poke(true.B)
      dut.io.function_i.tdata.poke(0.U)
      dut.io.function_i.tuser.get.poke(4096.U)

      dut.clock.step()

      dut.io.function_i.tvalid.poke(false.B)

      dut.io.sendmsg_o.ready.poke(true.B)
      dut.io.fetchdata_o.ready.poke(true.B)
      dut.io.dma_w_req_o.ready.poke(true.B)

      dut.io.sendmsg_o.valid.expect(true.B)
      dut.io.fetchdata_o.valid.expect(true.B)
      dut.io.dma_w_req_o.valid.expect(true.B)

      // TODO not currently checking the port out

      dut.clock.step()

      dut.io.sendmsg_o.valid.expect(false.B)
      dut.io.fetchdata_o.valid.expect(false.B)
      dut.io.dma_w_req_o.valid.expect(false.B)
    }

  }

 "delegate_test: ID assignment" in {
 }
}