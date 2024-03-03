package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

class TestDelegate extends AnyFreeSpec {

  "TestDelagate: testing dma map" in {
    simulate(new Delegate) { dut =>

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

      dut.io.newDmaMap.ready.poke(true.B)
      dut.io.newDmaMap.valid.expect(true.B)
      dut.io.newDmaMap.bits.pcie_addr.expect("hDEADBEEFDEADBEEF".U)
      dut.io.newDmaMap.bits.port.expect("hAABB".U)
      dut.io.newDmaMap.bits.map_type.expect(1.U)

      dut.clock.step()

      dut.io.newDmaMap.valid.expect(false.B)
    }
  }

  "TestDelegate: testing virtualization" in {
    simulate(new Delegate) { dut =>

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

      dut.io.newSendmsg.ready.poke(true.B)
      dut.io.newFetchdata.ready.poke(true.B)
      dut.io.ppEgressSendmsgRamReq.ready.poke(true.B)
      dut.io.ppIngressRecvmsgRamReq.ready.poke(true.B)
      dut.io.c2hMetadataRamReq.ready.poke(true.B)
      dut.io.c2hMetadataDmaReq.ready.poke(true.B)

      dut.io.newSendmsg.valid.expect(true.B)
      dut.io.newFetchdata.valid.expect(true.B)
      dut.io.ppEgressSendmsgRamReq.valid.expect(true.B)
      dut.io.c2hMetadataRamReq.valid.expect(true.B)
      dut.io.c2hMetadataDmaReq.valid.expect(true.B)

      // Should check more items
      dut.io.ppEgressSendmsgRamReq.bits.len.expect(64.U)

      // TODO not currently checking the port out

      dut.clock.step()

      dut.io.newSendmsg.valid.expect(false.B)
      dut.io.newFetchdata.valid.expect(false.B)
      dut.io.c2hMetadataRamReq.valid.expect(false.B)
      dut.io.c2hMetadataDmaReq.valid.expect(false.B)
    }
  }


  "TestDelegate: testing sendmsg" in {
    simulate(new Delegate) { dut =>

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

      dut.io.newSendmsg.ready.poke(true.B)
      dut.io.newFetchdata.ready.poke(true.B)
      dut.io.ppEgressSendmsgRamReq.ready.poke(true.B)
      dut.io.c2hMetadataRamReq.ready.poke(true.B)
      dut.io.c2hMetadataDmaReq.ready.poke(true.B)

      dut.io.newSendmsg.valid.expect(true.B)
      dut.io.newFetchdata.valid.expect(true.B)
      dut.io.c2hMetadataRamReq.valid.expect(true.B)
      dut.io.c2hMetadataDmaReq.valid.expect(true.B)

      // TODO not currently checking the port out

      dut.clock.step()

      dut.io.newSendmsg.valid.expect(false.B)
      dut.io.newFetchdata.valid.expect(false.B)
      dut.io.c2hMetadataRamReq.valid.expect(false.B)
      dut.io.c2hMetadataDmaReq.valid.expect(false.B)
    }
  }


 "delegate_test: ID assignment" in {
 }
}
