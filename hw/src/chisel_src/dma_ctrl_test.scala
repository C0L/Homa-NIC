package gpnic 

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

class h2c_dma_test extends AnyFreeSpec {

  "h2c_dma_test: testing dma_read_req_i -> pcie_read_cmd_o -> pcie_read_status_i -> dbuff_notif_o flow and tag memory" in {
    simulate(new h2c_dma) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      for (transaction <- 0 to 511) {

        dut.io.pcie_read_status_i.valid.poke(true.B)
        dut.io.dbuff_notif_o.ready.poke(true.B)

        dut.io.dma_read_req_i.valid.poke(true.B)
        dut.io.pcie_read_cmd_o.ready.poke(true.B)

        dut.io.dma_read_req_i.bits.pcie_read_addr.poke(transaction.U)
        dut.io.dma_read_req_i.bits.cache_id.poke(transaction.U)
        dut.io.dma_read_req_i.bits.message_size.poke(transaction.U)
        dut.io.dma_read_req_i.bits.read_len.poke((transaction + 256).U)
        dut.io.dma_read_req_i.bits.dest_ram_addr.poke(0.U)

        dut.io.dma_read_req_i.ready.expect(true.B)
        dut.io.pcie_read_cmd_o.valid.expect(true.B)
        dut.io.pcie_read_cmd_o.bits.pcie_addr.expect(transaction.U)
        dut.io.pcie_read_cmd_o.bits.ram_sel.expect(0.U)
        dut.io.pcie_read_cmd_o.bits.ram_addr.expect(0.U)
        dut.io.pcie_read_cmd_o.bits.len.expect((transaction + 256).U)
        dut.io.pcie_read_cmd_o.bits.tag.expect((transaction % 256).U)

        dut.clock.step()

        dut.io.dma_read_req_i.valid.poke(false.B)
        dut.io.pcie_read_cmd_o.ready.poke(false.B)

        dut.io.pcie_read_status_i.valid.poke(true.B)
        dut.io.dbuff_notif_o.ready.poke(true.B)

        dut.io.pcie_read_status_i.bits.tag.poke((transaction % 256).U)
        dut.io.pcie_read_status_i.bits.error.poke(2.U)

        dut.io.pcie_read_status_i.ready.expect(true.B)
        dut.io.dbuff_notif_o.valid.expect(true.B)

        dut.io.dbuff_notif_o.bits.rpc_id.expect(0.U)
        dut.io.dbuff_notif_o.bits.dbuff_id.expect(transaction.U)
        dut.io.dbuff_notif_o.bits.remaining.expect(transaction.U)
        dut.io.dbuff_notif_o.bits.dbuffered.expect(0.U) 
        dut.io.dbuff_notif_o.bits.granted.expect(0.U)
        dut.io.dbuff_notif_o.bits.priority.expect(1.U)

        dut.clock.step()
        // TODO ensure the signals reset in the next cycle
      }
    }
  }




  //"Gcd should calculate proper greatest common denominator" in {
  //  simulate(new DecoupledGcd(16)) { dut =>
  //    val testValues = for { x <- 0 to 10; y <- 0 to 10} yield (x, y)
  //    val inputSeq = testValues.map { case (x, y) => (new GcdInputBundle(16)).Lit(_.value1 -> x.U, _.value2 -> y.U) }
  //    val resultSeq = testValues.map { case (x, y) =>
  //      (new GcdOutputBundle(16)).Lit(_.value1 -> x.U, _.value2 -> y.U, _.gcd -> BigInt(x).gcd(BigInt(y)).U)
  //    }

  //    dut.reset.poke(true.B)
  //    dut.clock.step()
  //    dut.reset.poke(false.B)
  //    dut.clock.step()

  //    var sent, received, cycles: Int = 0
  //    while (sent != 100 && received != 100) {
  //      assert(cycles <= 1000, "timeout reached")

  //      if (sent < 100) {
  //        dut.input.valid.poke(true.B)
  //        dut.input.bits.value1.poke(testValues(sent)._1.U)
  //        dut.input.bits.value2.poke(testValues(sent)._2.U)
  //        if (dut.input.ready.peek().litToBoolean) {
  //          sent += 1
  //        }
  //      }

  //      if (received < 100) {
  //        dut.output.ready.poke(true.B)
  //        if (dut.output.valid.peekValue().asBigInt == 1) {
  //          dut.output.bits.gcd.expect(BigInt(testValues(received)._1).gcd(testValues(received)._2))
  //          received += 1
  //        }
  //      }

  //      // Step the simulation forward.
  //      dut.clock.step()
  //      cycles += 1
  //    }
  //  }
  //}
}


class addr_map_test extends AnyFreeSpec {

  "addr_map_test: testing that dma read and writes are augmented with correct host offset" in {
    simulate(new addr_map) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      // Fill every slot
      for (transaction <- 0 to 16383) {

        dut.io.dma_map_i.valid.poke(true.B)

        dut.io.dma_map_i.bits.pcie_addr.poke(transaction.U)
        dut.io.dma_map_i.bits.port.poke(transaction.U)
        dut.io.dma_map_i.bits.map_type.poke(dma_map_type.h2c_map)

        dut.clock.step()

        dut.io.dma_map_i.valid.poke(true.B)

        dut.io.dma_map_i.bits.pcie_addr.poke((transaction + 16384).U)
        dut.io.dma_map_i.bits.port.poke(transaction.U)
        dut.io.dma_map_i.bits.map_type.poke(dma_map_type.c2h_map)

        dut.clock.step()

        dut.io.dma_map_i.valid.poke(true.B)

        dut.io.dma_map_i.bits.pcie_addr.poke((transaction + 32768).U)
        dut.io.dma_map_i.bits.port.poke(transaction.U)
        dut.io.dma_map_i.bits.map_type.poke(dma_map_type.meta_map)

        dut.clock.step()
      }

      dut.io.dma_map_i.valid.poke(false.B)

      for (transaction <- 0 to 16383) {
        // TODO these should all be vars

        dut.io.dma_w_meta_i.valid.poke(true.B)

        dut.io.dma_w_meta_i.bits.pcie_write_addr.poke(transaction.U)
        dut.io.dma_w_meta_i.bits.data.poke((transaction + 1).U)
        dut.io.dma_w_meta_i.bits.length.poke(((transaction + 2) % 256).U)
        dut.io.dma_w_meta_i.bits.port.poke(transaction.U)

        dut.clock.step()

        dut.io.dma_w_meta_i.valid.poke(false.B)
        dut.io.dma_w_req_o.valid.expect(false.B)
        dut.io.dma_r_req_o.valid.expect(false.B)

        dut.clock.step()

        dut.io.dma_w_req_o.valid.expect(true.B)
        dut.io.dma_r_req_o.valid.expect(false.B)

        dut.io.dma_w_req_o.bits.pcie_write_addr.expect((transaction + transaction + 32768).U)
        dut.io.dma_w_req_o.bits.data.expect((transaction + 1).U)
        dut.io.dma_w_req_o.bits.length.expect(((transaction + 2) % 256).U)
        dut.io.dma_w_req_o.bits.port.expect(transaction.U)

        dut.clock.step()

        dut.io.dma_w_req_o.valid.expect(false.B)
        dut.io.dma_r_req_o.valid.expect(false.B)

      }


      for (transaction <- 0 to 16383) {
        // TODO these should all be vars

        dut.io.dma_w_data_i.valid.poke(true.B)

        dut.io.dma_w_data_i.bits.pcie_write_addr.poke(transaction.U)
        dut.io.dma_w_data_i.bits.data.poke((transaction + 1).U)
        dut.io.dma_w_data_i.bits.length.poke(((transaction + 2) % 256).U)
        dut.io.dma_w_data_i.bits.port.poke(transaction.U)

        dut.clock.step()

        dut.io.dma_w_data_i.valid.poke(false.B)
        dut.io.dma_w_req_o.valid.expect(false.B)
        dut.io.dma_r_req_o.valid.expect(false.B)

        dut.clock.step()

        dut.io.dma_w_req_o.valid.expect(true.B)
        dut.io.dma_r_req_o.valid.expect(false.B)

        dut.io.dma_w_req_o.bits.pcie_write_addr.expect((transaction + transaction + 16384).U)
        dut.io.dma_w_req_o.bits.data.expect((transaction + 1).U)
        dut.io.dma_w_req_o.bits.length.expect(((transaction + 2) % 256).U)
        dut.io.dma_w_req_o.bits.port.expect(transaction.U)

        dut.clock.step()

        dut.io.dma_w_req_o.valid.expect(false.B)
        dut.io.dma_r_req_o.valid.expect(false.B)

      }

      // Should just torture test everything according to some spec
      // Verify simultanous executions of writes

      for (transaction <- 0 to 16383) {
        // TODO these should all be vars or randomly generated values

        dut.io.dma_r_req_i.valid.poke(true.B)

        dut.io.dma_r_req_i.bits.pcie_read_addr.poke(transaction.U)
        dut.io.dma_r_req_i.bits.cache_id.poke(((transaction + 1) % 1024).U)
        dut.io.dma_r_req_i.bits.message_size.poke((transaction + 2).U)
        dut.io.dma_r_req_i.bits.read_len.poke(((transaction + 3) % 2048).U)
        dut.io.dma_r_req_i.bits.dest_ram_addr.poke((transaction + 4).U)
        dut.io.dma_r_req_i.bits.port.poke(transaction.U)

        dut.clock.step()

        dut.io.dma_r_req_i.valid.poke(false.B)
        dut.io.dma_w_req_o.valid.expect(false.B)
        dut.io.dma_r_req_o.valid.expect(false.B)

        dut.clock.step()

        dut.io.dma_w_req_o.valid.expect(false.B)
        dut.io.dma_r_req_o.valid.expect(true.B)

        dut.io.dma_r_req_o.bits.pcie_read_addr.expect((transaction + transaction).U)
        dut.io.dma_r_req_o.bits.cache_id.expect(((transaction + 1) % 1024).U)
        dut.io.dma_r_req_o.bits.message_size.expect((transaction + 2).U)
        dut.io.dma_r_req_o.bits.read_len.expect(((transaction + 3) % 2048).U)
        dut.io.dma_r_req_o.bits.dest_ram_addr.expect((transaction + 4).U)
        dut.io.dma_r_req_o.bits.port.expect(transaction.U)

        dut.clock.step()

        dut.io.dma_w_req_o.valid.expect(false.B)
        dut.io.dma_r_req_o.valid.expect(false.B)

      }
    }
  }
}

