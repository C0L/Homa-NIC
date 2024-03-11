package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util.Decoupled
import chisel3.util._

// TODO need a better hash function!
// TODO no flow control

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

  val tables = Seq.fill(4)(SRAM(1024, new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 2, 1, 0))
  val insertQueues = Seq.fill(5)(Module(new Queue(new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 1, true, false)))
  val searchQueues = Seq.fill(4)(Module(new Queue(new CAMEntry(KEY_WIDTH, VALUE_WIDTH), 1, true, false)))

  insertQueues(0).io.enq <> io.insert
  io.search.ready := true.B // TODO always the case?
  io.result.valid := false.B
  io.result.bits  := 0.U.asTypeOf(new CAMEntry(KEY_WIDTH, VALUE_WIDTH))

  insertQueues(4).io.deq.ready := true.B

  for (table <- 0 to 3) {
    /* Insert Logic */
    val insertPending = RegNext(insertQueues(table).io.deq.valid)

    // Initiate insert write commands
    tables(table).writePorts(0).address := RegNext(insertQueues(table).io.deq.bits.hash(table))
    tables(table).writePorts(0).data    := RegNext(insertQueues(table).io.deq.bits)
    tables(table).writePorts(0).enable  := RegNext(insertQueues(table).io.deq.valid)

    // Initiate insert read commands
    tables(table).readPorts(0).address := insertQueues(table).io.deq.bits.hash(table)
    tables(table).readPorts(0).enable  := insertQueues(table).io.deq.valid

    insertQueues(table+1).io.enq.valid := false.B
    insertQueues(table+1).io.enq.bits  := 0.U.asTypeOf(new CAMEntry(KEY_WIDTH, VALUE_WIDTH))

    when (insertPending && tables(table).readPorts(0).data.set === 1.U) {
      insertQueues(table+1).io.enq.bits  := tables(table).readPorts(0).data
      insertQueues(table+1).io.enq.valid := true.B
    }

    // TODO no flow control?
    insertQueues(table).io.deq.ready := true.B

    /* Search Logic */
    searchQueues(table).io.enq.bits  := io.search.bits
    searchQueues(table).io.enq.valid := io.search.valid

    // TODO
    searchQueues(table).io.deq.ready := true.B

    tables(table).readPorts(1).address := searchQueues(table).io.deq.bits.hash(table)
    tables(table).readPorts(1).enable  := searchQueues(table).io.deq.valid

    val searchPending = RegNext(searchQueues(table).io.deq.valid)
    val searchKey     = RegNext(searchQueues(table).io.deq.bits.key)

    when (searchPending) {
      when (tables(table).readPorts(1).data.key === searchKey) {
        io.result.bits     := tables(table).readPorts(1).data
        io.result.bits.set := 1.U
      }
      io.result.valid := true.B
    }
  }
}
