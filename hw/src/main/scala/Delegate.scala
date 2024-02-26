package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

/* delegate - 512 bit MMIO writes are converter to AXI Streams where
 * the ID field carries the destination address. The destination
 * address determines both the destination user and the desired
 * functionality. Currently that functionality is:
 *     Address 0                  -> New DMA Map
 *     Address 0  + (port * 4096) -> sendmsg
 *     Address 64 + (port * 4096) -> recvmsg 
 */
class Delegate extends Module {
  val io = IO(new Bundle {
    val function_i  = Flipped(new axis(512, false, 0, true, 32, false, 0, false)) // 512 bit blocks from user
    val sendmsg_o   = Decoupled(new QueueEntry) // Output to sendmsg priority queue
    val fetchdata_o = Decoupled(new QueueEntry) // Output to datafetch queue
    val dma_w_req_o = Decoupled(new dma_write_t)   // Output to pcie write
    val dma_map_o   = Decoupled(new dma_map_t)     // Output to add new DMA map

    val sendmsg_ram_write_desc        = Decoupled(new ram_write_desc_t) // Descriptor to write to RAM
    val sendmsg_ram_write_data        = Decoupled(new ram_write_data_t) // Data to write to RAM
    val sendmsg_ram_write_desc_status = Flipped(Decoupled(new ram_write_desc_status_t))   // Result of RAM write

    val recvmsg_ram_write_desc        = Decoupled(new ram_write_desc_t) // Descriptor to write to RAM
    val recvmsg_ram_write_data        = Decoupled(new ram_write_data_t) // Data to write to RAM
    val recvmsg_ram_write_desc_status = Flipped(Decoupled(new ram_write_desc_status_t))   // Result of RAM write
  })

  // By default all output streams are 0 initialized
  io.dma_map_o.bits   := 0.U.asTypeOf(new dma_map_t)

  // By default none of the output streams are active
  io.dma_map_o.valid   := false.B

  val sendmsg_queue   = Module(new Queue(new QueueEntry, 1, true, false))
  val fetchdata_queue = Module(new Queue(new QueueEntry, 1, true, false))
  val dma_w_req_queue = Module(new Queue(new dma_write_t, 1, true, false))

  sendmsg_queue.io.deq <> io.sendmsg_o
  fetchdata_queue.io.deq <> io.fetchdata_o
  dma_w_req_queue.io.deq <> io.dma_w_req_o

  sendmsg_queue.io.enq.bits   := io.sendmsg_o.bits
  fetchdata_queue.io.enq.bits := io.fetchdata_o.bits
  dma_w_req_queue.io.enq.bits := io.dma_w_req_o.bits

  sendmsg_queue.io.enq.valid   := false.B 
  fetchdata_queue.io.enq.valid := false.B
  dma_w_req_queue.io.enq.valid := false.B

  // Data stored in the queue
  class encaps extends Bundle {
    val data = UInt(512.W)
    val user = UInt(32.W)
  }

  /* We want our AXI stream interface to be registered but still
   * operate at full throughput An easy way to do this seems to
   * generate a queue with depth one with "pipe" and "flow" enabled.
   */
  val function_queue = Module(new Queue(new encaps, 1, true, true))
  function_queue.io.enq.bits.data := io.function_i.tdata
  function_queue.io.enq.bits.user := io.function_i.tuser.get
  function_queue.io.enq.valid     := io.function_i.tvalid
  io.function_i.tready            := function_queue.io.enq.ready

  io.sendmsg_ram_write_data.bits  := 0.U.asTypeOf(new ram_write_data_t)
  io.sendmsg_ram_write_data.valid := false.B

  // We throw away status returns
  io.sendmsg_ram_write_desc_status       := DontCare
  io.sendmsg_ram_write_desc_status.ready := true.B
  io.sendmsg_ram_write_desc              := DontCare

  io.recvmsg_ram_write_data.bits      := 0.U.asTypeOf(new ram_write_data_t)
  io.recvmsg_ram_write_data.valid := false.B

  // We throw away status returns
  io.recvmsg_ram_write_desc_status       := DontCare
  io.recvmsg_ram_write_desc_status.ready := true.B
  io.recvmsg_ram_write_desc              := DontCare

  // Be default nothing will exit our queue
  function_queue.io.deq.ready     := false.B

  // The user ID is the page allocated to this user
  val user_id   = function_queue.io.deq.bits.user >> 12
  // The user func is the ofset within that page
  val user_func = function_queue.io.deq.bits.user(11,0)

  val send_id  = RegInit(0.asUInt(16.W))
  val dbuff_id = RegInit(0.asUInt(6.W))

  // Is there a valid registered transaction?
  when(function_queue.io.deq.valid) {
    // Are we talking to the kernel (raw address 0) or a user (port * 4096)
    when (user_id === 0.U) {
      function_queue.io.deq.ready := io.dma_map_o.ready
      io.dma_map_o.bits           := function_queue.io.deq.bits.data.asTypeOf(new dma_map_t)
      io.dma_map_o.valid          := function_queue.io.deq.valid
    } otherwise {
      // User bits specify the function of this 512 bit block for a particular port
       switch (user_func) {
         // new sendmsg
         is (0.U) {
           val msghdr_send = function_queue.io.deq.bits.data.asTypeOf(new msghdr_t)

           msghdr_send.id := send_id

           io.sendmsg_ram_write_data.bits.data     := msghdr_send.asUInt
           io.sendmsg_ram_write_data.bits.keep     := ("hffffffffffffffffffff".U)
           io.sendmsg_ram_write_data.bits.last     := 1.U

           io.sendmsg_ram_write_desc.bits.ram_addr := 64.U * send_id
           io.sendmsg_ram_write_desc.bits.len      := 64.U
           io.sendmsg_ram_write_desc.bits.tag      := send_id

           // Construct a new meta data writeback request with the newly assigned ID
           dma_w_req_queue.io.enq.bits.pcie_write_addr := 64.U * msghdr_send.ret 
           dma_w_req_queue.io.enq.bits.data            := function_queue.io.deq.bits.data
           dma_w_req_queue.io.enq.bits.length          := 64.U
           dma_w_req_queue.io.enq.bits.port            := user_id

           // Construct a new entry for the packet queue to send packets
           sendmsg_queue.io.enq.bits.rpc_id    := msghdr_send.id
           sendmsg_queue.io.enq.bits.dbuff_id  := dbuff_id;
           sendmsg_queue.io.enq.bits.remaining := msghdr_send.buff_size
           sendmsg_queue.io.enq.bits.dbuffered := msghdr_send.buff_size
           sendmsg_queue.io.enq.bits.granted   := 0.U
           sendmsg_queue.io.enq.bits.priority  := queue_priority.ACTIVE.asUInt;

           // Construct a new entry for the fetch queue to fetch data
           fetchdata_queue.io.enq.bits.rpc_id    := msghdr_send.id
           fetchdata_queue.io.enq.bits.dbuff_id  := dbuff_id 
           fetchdata_queue.io.enq.bits.remaining := msghdr_send.buff_size
           fetchdata_queue.io.enq.bits.dbuffered := 0.U
           // TODO beginning as initially granted
           fetchdata_queue.io.enq.bits.granted   := msghdr_send.buff_size
           fetchdata_queue.io.enq.bits.priority  := queue_priority.ACTIVE.asUInt;

           // Only signal that a transaction is ready to exit our queue if all recievers are ready
           // Ram core will always be ready
           // TODO this could also block things in unintended ways, but they really should be one op
           function_queue.io.deq.ready     := sendmsg_queue.io.enq.ready & fetchdata_queue.io.enq.ready & dma_w_req_queue.io.enq.ready

           sendmsg_queue.io.enq.valid      := function_queue.io.deq.fire
           fetchdata_queue.io.enq.valid    := function_queue.io.deq.fire
           dma_w_req_queue.io.enq.valid    := function_queue.io.deq.fire
           io.sendmsg_ram_write_desc.valid := function_queue.io.deq.fire
           io.sendmsg_ram_write_data.valid := function_queue.io.deq.fire

           // Only when te transaction takes place we increment the send and dbuff IDs
           when (function_queue.io.deq.fire) {
             send_id  := send_id + 1.U
             dbuff_id := dbuff_id + 1.U
           }
         }

         // new recvmsg
         is (64.U) {
           val msghdr_recv = function_queue.io.deq.bits.data.asTypeOf(new msghdr_t)

           io.recvmsg_ram_write_data.bits.data     := msghdr_recv.asUInt
           io.recvmsg_ram_write_data.bits.keep     := ("hffffffffffffffffffff".U)
           io.recvmsg_ram_write_data.bits.last     := 1.U

           io.recvmsg_ram_write_desc.bits.ram_addr := 64.U * send_id
           io.recvmsg_ram_write_desc.bits.len      := 64.U
           io.recvmsg_ram_write_desc.bits.tag      := send_id
         }
       }
    }
  }
}
