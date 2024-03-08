package gpnic

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.simulator.EphemeralSimulator._
import org.scalatest.freespec.AnyFreeSpec
import org.scalatest.matchers.must.Matchers

class TestAddressMap extends AnyFreeSpec {

  "testing that dma read and writes are augmented with correct host offset" in {
    simulate(new AddressMap) { dut =>

      dut.reset.poke(true.B)
      dut.clock.step()
      dut.reset.poke(false.B)
      dut.clock.step()

      dut.io.mappableIn(0).valid.poke(false.B)
      dut.io.mappableIn(1).valid.poke(false.B)
      dut.io.mappableIn(2).valid.poke(false.B)

      // Fill every slot
      for (transaction <- 0 to 16383) {

        dut.io.dmaMap.valid.poke(true.B)

        dut.io.dmaMap.bits.pcie_addr.poke(transaction.U)
        dut.io.dmaMap.bits.port.poke(transaction.U)
        dut.io.dmaMap.bits.map_type.poke(0.U)

        dut.clock.step()

        dut.io.dmaMap.valid.poke(true.B)

        dut.io.dmaMap.bits.pcie_addr.poke((transaction + 16384).U)
        dut.io.dmaMap.bits.port.poke(transaction.U)
        dut.io.dmaMap.bits.map_type.poke(1.U)

        dut.clock.step()

        dut.io.dmaMap.valid.poke(true.B)

        dut.io.dmaMap.bits.pcie_addr.poke((transaction + 32768).U)
        dut.io.dmaMap.bits.port.poke(transaction.U)
        dut.io.dmaMap.bits.map_type.poke(2.U)

        dut.clock.step()
      }

      dut.io.dmaMap.valid.poke(false.B)
 
      for (transaction <- 0 to 16383) {
 
        dut.io.mappableIn(0).valid.poke(true.B)
        dut.io.mappableIn(0).ready.expect(true.B)
        dut.io.mappableIn(0).bits.pcie_addr.poke(transaction.U)
        dut.io.mappableIn(0).bits.port.poke(transaction.U)
  
         dut.clock.step()
  
         dut.io.mappableIn(0).valid.poke(false.B)
 
         dut.io.mappableOut(0).valid.expect(false.B)
         dut.io.mappableOut(1).valid.expect(false.B)
         dut.io.mappableOut(2).valid.expect(false.B)
 
         dut.clock.step()
 
         dut.io.mappableOut(0).valid.expect(false.B)
         dut.io.mappableOut(1).valid.expect(false.B)
         dut.io.mappableOut(2).valid.expect(false.B)
 
         dut.clock.step()
  
         dut.io.mappableOut(0).ready.poke(true.B)
 
         dut.io.mappableOut(0).valid.expect(true.B)
         dut.io.mappableOut(1).valid.expect(false.B)
         dut.io.mappableOut(2).valid.expect(false.B)
 
         dut.io.mappableOut(0).bits.pcie_addr.expect((transaction * 2).U)
 
         dut.clock.step()
 
         dut.io.mappableOut(0).valid.expect(false.B)
         dut.io.mappableOut(1).valid.expect(false.B)
         dut.io.mappableOut(2).valid.expect(false.B)
       }


//       for (transaction <- 0 to 16383) {
// 
//         dut.io.mappableIn(1).valid.poke(true.B)
//         dut.io.mappableIn(1).ready.expect(true.B)
//         dut.io.mappableIn(1).bits.pcie_addr.poke(transaction.U)
//  
// 
//         dut.clock.step()
//  
//         dut.io.mappableIn(1).valid.poke(false.B)
// 
//         dut.io.mappableOut(0).valid.expect(false.B)
//         dut.io.mappableOut(1).valid.expect(false.B)
//         dut.io.mappableOut(2).valid.expect(false.B)
// 
//         dut.clock.step()
// 
//         dut.io.mappableOut(0).valid.expect(false.B)
//         dut.io.mappableOut(1).valid.expect(false.B)
//         dut.io.mappableOut(2).valid.expect(false.B)
// 
//         dut.clock.step()
//  
//         dut.io.mappableOut(1).ready.poke(true.B)
// 
//         dut.io.mappableOut(0).valid.expect(false.B)
//         dut.io.mappableOut(1).valid.expect(true.B)
//         dut.io.mappableOut(2).valid.expect(false.B)
// 
//         dut.io.mappableOut(1).bits.pcie_addr.expect(transaction.U)
// 
//         dut.clock.step()
// 
//         dut.io.mappableOut(0).valid.expect(false.B)
//         dut.io.mappableOut(1).valid.expect(false.B)
//         dut.io.mappableOut(2).valid.expect(false.B)
//       }



//       for (transaction <- 0 to 16383) {
//         // TODO these should all be vars
// 
//         dut.io.dma_w_data_i.valid.poke(true.B)
// 
//         dut.io.dma_w_data_i.bits.pcie_write_addr.poke(transaction.U)
//         dut.io.dma_w_data_i.bits.data.poke((transaction + 1).U)
//         dut.io.dma_w_data_i.bits.length.poke(((transaction + 2) % 256).U)
//         dut.io.dma_w_data_i.bits.port.poke(transaction.U)
// 
//         dut.clock.step()
// 
//         dut.io.dma_w_data_i.valid.poke(false.B)
//         dut.io.dma_w_req_o.valid.expect(false.B)
//         dut.io.dma_r_req_o.valid.expect(false.B)
// 
//         dut.clock.step()
// 
//         dut.io.dma_w_req_o.valid.expect(true.B)
//         dut.io.dma_r_req_o.valid.expect(false.B)
// 
//         dut.io.dma_w_req_o.bits.pcie_write_addr.expect((transaction + transaction + 16384).U)
//         dut.io.dma_w_req_o.bits.data.expect((transaction + 1).U)
//         dut.io.dma_w_req_o.bits.length.expect(((transaction + 2) % 256).U)
//         dut.io.dma_w_req_o.bits.port.expect(transaction.U)
// 
//         dut.clock.step()
// 
//         dut.io.dma_w_req_o.valid.expect(false.B)
//         dut.io.dma_r_req_o.valid.expect(false.B)
// 
//       }
// 
//       for (transaction <- 0 to 16383) {
//         dut.io.dma_r_req_i.valid.poke(true.B)
// 
//         dut.io.dma_r_req_i.bits.pcie_read_addr.poke(transaction.U)
//         dut.io.dma_r_req_i.bits.cache_id.poke(((transaction + 1) % 1024).U)
//         dut.io.dma_r_req_i.bits.message_size.poke((transaction + 2).U)
//         dut.io.dma_r_req_i.bits.read_len.poke(((transaction + 3) % 2048).U)
//         dut.io.dma_r_req_i.bits.dest_ram_addr.poke((transaction + 4).U)
//         dut.io.dma_r_req_i.bits.port.poke(transaction.U)
// 
//         dut.clock.step()
// 
//         dut.io.dma_r_req_i.valid.poke(false.B)
//         dut.io.dma_w_req_o.valid.expect(false.B)
//         dut.io.dma_r_req_o.valid.expect(false.B)
// 
//         dut.clock.step()
// 
//         dut.io.dma_w_req_o.valid.expect(false.B)
//         dut.io.dma_r_req_o.valid.expect(true.B)
// 
//         dut.io.dma_r_req_o.bits.pcie_read_addr.expect((transaction + transaction).U)
//         dut.io.dma_r_req_o.bits.cache_id.expect(((transaction + 1) % 1024).U)
//         dut.io.dma_r_req_o.bits.message_size.expect((transaction + 2).U)
//         dut.io.dma_r_req_o.bits.read_len.expect(((transaction + 3) % 2048).U)
//         dut.io.dma_r_req_o.bits.dest_ram_addr.expect((transaction + 4).U)
//         dut.io.dma_r_req_o.bits.port.expect(transaction.U)
// 
//         dut.clock.step()
// 
//         dut.io.dma_w_req_o.valid.expect(false.B)
//         dut.io.dma_r_req_o.valid.expect(false.B)
// 
//       }
    }
  }
}

