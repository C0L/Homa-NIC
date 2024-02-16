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

        dut.io.dma_read_status.valid.poke(true.B)
        dut.io.dbuff_notif_o.ready.poke(true.B)

        dut.io.dma_r_req.valid.poke(true.B)
        dut.io.dma_read_desc.ready.poke(true.B)

        dut.io.dma_r_req.bits.pcie_read_addr.poke(transaction.U)
        dut.io.dma_r_req.bits.cache_id.poke(transaction.U)
        dut.io.dma_r_req.bits.message_size.poke(transaction.U)
        dut.io.dma_r_req.bits.read_len.poke((transaction + 256).U)
        dut.io.dma_r_req.bits.dest_ram_addr.poke(0.U)

        dut.io.dma_r_req.ready.expect(true.B)
        dut.io.dma_read_desc.valid.expect(true.B)
        dut.io.dma_read_desc.bits.pcie_addr.expect(transaction.U)
        dut.io.dma_read_desc.bits.ram_sel.expect(0.U)
        dut.io.dma_read_desc.bits.ram_addr.expect(0.U)
        dut.io.dma_read_desc.bits.len.expect((transaction + 256).U)
        dut.io.dma_read_desc.bits.tag.expect((transaction % 256).U)

        dut.clock.step()

        dut.io.dma_r_req.valid.poke(false.B)
        dut.io.dma_read_desc.ready.poke(false.B)

        dut.io.dma_read_status.valid.poke(true.B)
        dut.io.dbuff_notif_o.ready.poke(true.B)

        dut.io.dma_read_status.bits.tag.poke((transaction % 256).U)
        dut.io.dma_read_status.bits.error.poke(2.U)

        dut.io.dma_read_status.ready.expect(true.B)
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
}


class c2h_dma_test extends AnyFreeSpec {

  "c2h_dma_test: testing dma write request triggers RAM write" in {
    simulate(new c2h_dma) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.ram_write_desc.ready.poke(true.B)
      dut.io.ram_write_data.ready.poke(true.B)
      dut.io.ram_write_desc_status.valid.poke(false.B)

      dut.io.dma_write_desc.ready.poke(true.B)
      dut.io.dma_write_desc_status.valid.poke(false.B)

      dut.io.dma_write_req.valid.poke(true.B)
      dut.io.dma_write_req.bits.pcie_write_addr.poke(4096.U)
      dut.io.dma_write_req.bits.data.poke("hDEADBEEF".U)
      dut.io.dma_write_req.bits.length.poke(64.U)
      dut.io.dma_write_req.bits.port.poke(1.U)

      dut.io.ram_write_desc.valid.expect(true.B)
      dut.io.ram_write_data.valid.expect(true.B)

      dut.io.dma_write_req.valid.poke(false.B)

      // TODO check the values of the output 
    }
  }

//  "c2h_dma_test: testing status response triggers dma write request" in {
//    simulate(new c2h_dma) { dut =>
//
//      dut.reset.poke(true.B)
//      dut.clock.step()
//      dut.reset.poke(false.B)
//      dut.clock.step()
//
//      dut.io.dma_write_req.valid.poke(false.B)
//
//      dut.io.ram_write_desc.ready.poke(true.B)
//      dut.io.ram_write_data.ready.poke(true.B)
//      dut.io.ram_write_desc_status.valid.poke(true.B)
//
//      dut.io.dma_write_desc.ready.poke(true.B)
//      dut.io.dma_write_desc_status.valid.poke(false.B)
//
//      dut.io.ram_write_desc_status.bits.tag.poke(1.U)
//
//      dut.io.dma_write_desc.valid.expect(true.B)
//    }
//  }
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
        dut.io.dma_map_i.bits.map_type.poke(0.U)

        dut.clock.step()

        dut.io.dma_map_i.valid.poke(true.B)

        dut.io.dma_map_i.bits.pcie_addr.poke((transaction + 16384).U)
        dut.io.dma_map_i.bits.port.poke(transaction.U)
        dut.io.dma_map_i.bits.map_type.poke(1.U)

        dut.clock.step()

        dut.io.dma_map_i.valid.poke(true.B)

        dut.io.dma_map_i.bits.pcie_addr.poke((transaction + 32768).U)
        dut.io.dma_map_i.bits.port.poke(transaction.U)
        dut.io.dma_map_i.bits.map_type.poke(2.U)

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

