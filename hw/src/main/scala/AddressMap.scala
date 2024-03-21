package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

// TODO not flow control in addr map core!!!!

/* AddressMap - performs DMA address translations for read and write
 * requests before they are passed the pcie core.
 */
class AddressMap extends Module {
  val io = IO(new Bundle {
    val dmaMap      = Flipped(Decoupled(new DmaMap)) // New DMA address translations

    val mappableIn  = Vec(3, Flipped(Decoupled(new DmaReq)))
    val mappableOut = Vec(3, Decoupled(new DmaReq))
  })

  val map = SyncReadMem(3*100, new DmaMap)
  // TODO really need to have three seperate maps due to limited RW channels!!!

  // TODO can data still get lost in this if a read request goes out then there is stall??
  // Should reg read output?
  for (channel <- 0 to 2) {

    val queueIn   = Module(new Queue(new DmaReq, 1, true, false))
    val readStage = Module(new Queue(new DmaReq, 1, true, false))
    val queueOut  = Module(new Queue(new DmaReq, 1, true, false))

    queueIn.io.enq          <> io.mappableIn(channel)
    readStage.io.enq        <> queueIn.io.deq
    queueOut.io.enq         <> readStage.io.deq
    io.mappableOut(channel) <> queueOut.io.deq

    val read = map.read(queueIn.io.deq.bits.port + (channel * 100).U, queueIn.io.deq.fire)

    when(queueOut.io.enq.fire) {
      queueOut.io.enq.bits.pcie_addr := readStage.io.deq.bits.pcie_addr + read.pcie_addr
    }
  }

  val dmaMapReg = Module(new Queue(new DmaMap, 1, true, false))

  io.dmaMap <> dmaMapReg.io.enq
  dmaMapReg.io.deq.ready := true.B

  when (dmaMapReg.io.deq.valid) {
    // printf("Write: %d\n", dmaMapReg.io.deq.bits.port + (dmaMapReg.io.deq.bits.map_type * 16384.U))
    map.write(dmaMapReg.io.deq.bits.port + (dmaMapReg.io.deq.bits.map_type * 100.U), dmaMapReg.io.deq.bits)
  }
}
