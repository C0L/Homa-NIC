package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

// TODO need a better hash function!
// TODO no flow control
// TODO a lot of this logic can be decomposed into a cuckoo table class

/* CAM - content addressable memory module based on cuckoo hashing
 * techniques. This design uses 4 tables with 4 hash functions (on
 * assigned to each).
 */
class CAM (KEY_WIDTH: Int, VALUE_WIDTH: Int) extends Module {
  val io = IO(new Bundle {
    val search = Flipped(Decoupled(new CAMEntry(KEY_WIDTH, VALUE_WIDTH)))
    val result = Decoupled(new CAMEntry(KEY_WIDTH, VALUE_WIDTH))
    val insert = Flipped(Decoupled(new CAMEntry(KEY_WIDTH, VALUE_WIDTH)))
  })

  // 2 read, 2 write, 0 read-write ported SRAM
  val cam_0 = SRAM(16384, new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 2, 1, 0)
  val cam_1 = SRAM(16384, new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 2, 1, 0)
  val cam_2 = SRAM(16384, new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 2, 1, 0)
  val cam_3 = SRAM(16384, new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 2, 1, 0)

  /***** Insertion Logic *****/

  // CAMEntry ready to be inserted into respective sub-table
  val cam_0_insert = Module(new Queue(new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 1, true, false))
  val cam_1_insert = Module(new Queue(new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 1, true, false))
  val cam_2_insert = Module(new Queue(new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 1, true, false))
  val cam_3_insert = Module(new Queue(new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 1, true, false))

  // Save the state of the CAMEnrtry write and read operations
  val cam_0_insert_pending = RegNext(cam_0_insert.io.deq.valid)
  val cam_1_insert_pending = RegNext(cam_1_insert.io.deq.valid)
  val cam_2_insert_pending = RegNext(cam_2_insert.io.deq.valid)

  cam_0_insert.io.enq <> io.insert
  cam_1_insert.io.enq.valid := false.B
  cam_2_insert.io.enq.valid := false.B
  cam_3_insert.io.enq.valid := false.B

  cam_1_insert.io.enq.bits := 0.U.asTypeOf(new CAMEntry(KEY_WIDTH, VALUE_WIDTH))
  cam_2_insert.io.enq.bits := 0.U.asTypeOf(new CAMEntry(KEY_WIDTH, VALUE_WIDTH))
  cam_3_insert.io.enq.bits := 0.U.asTypeOf(new CAMEntry(KEY_WIDTH, VALUE_WIDTH))

  cam_1_insert.io.enq.ready := DontCare 
  cam_2_insert.io.enq.ready := DontCare 
  cam_3_insert.io.enq.ready := DontCare 

  cam_0_insert.io.deq.ready := true.B 
  cam_1_insert.io.deq.ready := true.B 
  cam_2_insert.io.deq.ready := true.B 
  cam_3_insert.io.deq.ready := true.B 

  // Initiate insert write commands
  cam_0.writePorts(0).address := RegNext(cam_0_insert.io.deq.bits.hash(0))
  cam_0.writePorts(0).data    := RegNext(cam_0_insert.io.deq.bits)
  cam_0.writePorts(0).enable  := RegNext(cam_0_insert.io.deq.valid)
  cam_1.writePorts(0).address := RegNext(cam_1_insert.io.deq.bits.hash(1))
  cam_1.writePorts(0).data    := RegNext(cam_1_insert.io.deq.bits)
  cam_1.writePorts(0).enable  := RegNext(cam_1_insert.io.deq.valid)
  cam_2.writePorts(0).address := RegNext(cam_2_insert.io.deq.bits.hash(2))
  cam_2.writePorts(0).data    := RegNext(cam_2_insert.io.deq.bits)
  cam_2.writePorts(0).enable  := RegNext(cam_2_insert.io.deq.valid)
  cam_3.writePorts(0).address := RegNext(cam_3_insert.io.deq.bits.hash(3))
  cam_3.writePorts(0).data    := RegNext(cam_3_insert.io.deq.bits)
  cam_3.writePorts(0).enable  := RegNext(cam_3_insert.io.deq.valid)

  // Initiate insert read commands
  cam_0.readPorts(0).address := cam_0_insert.io.deq.bits.hash(0)
  cam_0.readPorts(0).enable  := cam_0_insert.io.deq.valid
  cam_1.readPorts(0).address := cam_1_insert.io.deq.bits.hash(1)
  cam_1.readPorts(0).enable  := cam_1_insert.io.deq.valid
  cam_2.readPorts(0).address := cam_2_insert.io.deq.bits.hash(2)
  cam_2.readPorts(0).enable  := cam_2_insert.io.deq.valid
  cam_3.readPorts(0).address := cam_3_insert.io.deq.bits.hash(3)
  cam_3.readPorts(0).enable  := cam_3_insert.io.deq.valid

  // If the insertion read completed, and a valid value came out,
  // insert that value in the next queue over
  when (cam_0_insert_pending && (cam_0.readPorts(0).data.set === 1.U)) {
    cam_1_insert.io.enq.bits  := cam_0.readPorts(0).data
    cam_1_insert.io.enq.valid := true.B
  }

  when (cam_1_insert_pending && cam_1.readPorts(0).data.set === 1.U) {
    cam_2_insert.io.enq.bits  := cam_1.readPorts(0).data
    cam_2_insert.io.enq.valid := true.B
  }

  when (cam_2_insert_pending && cam_2.readPorts(0).data.set === 1.U) {
    cam_3_insert.io.enq.bits  := cam_2.readPorts(0).data
    cam_3_insert.io.enq.valid := true.B
  }

  /***** Search Logic *****/

  // CAMEntry ready to be inserted into respective sub-table
  val cam_0_search = Module(new Queue(new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 1, true, false))
  val cam_1_search = Module(new Queue(new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 1, true, false))
  val cam_2_search = Module(new Queue(new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 1, true, false))
  val cam_3_search = Module(new Queue(new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 1, true, false))

  cam_0_search.io.enq.bits  := io.search.bits
  cam_0_search.io.enq.valid := io.search.valid
  cam_1_search.io.enq.bits  := io.search.bits
  cam_1_search.io.enq.valid := io.search.valid
  cam_2_search.io.enq.bits  := io.search.bits
  cam_2_search.io.enq.valid := io.search.valid
  cam_3_search.io.enq.bits  := io.search.bits
  cam_3_search.io.enq.valid := io.search.valid

  // TODO 
  cam_0_search.io.deq.ready := true.B 
  cam_1_search.io.deq.ready := true.B 
  cam_2_search.io.deq.ready := true.B 
  cam_3_search.io.deq.ready := true.B 

  io.search.ready := true.B // TODO always the case?

  // Initiate search read command
  cam_0.readPorts(1).address := cam_0_search.io.deq.bits.hash(0)
  cam_0.readPorts(1).enable  := cam_0_search.io.deq.valid
  cam_1.readPorts(1).address := cam_1_search.io.deq.bits.hash(1)
  cam_1.readPorts(1).enable  := cam_1_search.io.deq.valid
  cam_2.readPorts(1).address := cam_2_search.io.deq.bits.hash(2)
  cam_2.readPorts(1).enable  := cam_2_search.io.deq.valid
  cam_3.readPorts(1).address := cam_3_search.io.deq.bits.hash(3)
  cam_3.readPorts(1).enable  := cam_3_search.io.deq.valid

  // All searches happen together, so only save data on one of them
  val cam_search_pending = RegNext(cam_0_search.io.deq.valid)
  val cam_search_key     = RegNext(cam_0_search.io.deq.bits.key)

  io.result.valid := false.B
  io.result.bits  := 0.U.asTypeOf(new CAMEntry(KEY_WIDTH, VALUE_WIDTH))

  val results = VecInit(cam_0.readPorts(1).data, cam_1.readPorts(1).data, cam_2.readPorts(1).data, cam_3.readPorts(1).data)
  when (cam_search_pending) {
    io.result.bits     := results(results.indexWhere(a => a.key === cam_search_key))
    io.result.valid    := true.B
    io.result.bits.set := results.exists(a => a.key === cam_search_key)
  }
}