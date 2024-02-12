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
class delegate extends Module {
  val io = IO(new Bundle {
    val function_i  = Flipped(new axis(512, false, 0, true, 32, false))
    val sendmsg_o   = Decoupled(new queue_entry_t)
    val fetchdata_o = Decoupled(new queue_entry_t)
    val dma_w_req_o = Decoupled(new dma_write_t)
    val dma_map_o   = Decoupled(new dma_map_t)
  })

  // By default all output streams are 0 initialized
  io.sendmsg_o.bits   := 0.U.asTypeOf(new queue_entry_t)
  io.fetchdata_o.bits := 0.U.asTypeOf(new queue_entry_t)
  io.dma_w_req_o.bits := 0.U.asTypeOf(new dma_write_t)
  io.dma_map_o.bits   := 0.U.asTypeOf(new dma_map_t)

  // By default none of the output streams are active
  io.sendmsg_o.valid   := false.B
  io.fetchdata_o.valid := false.B
  io.dma_w_req_o.valid := false.B
  io.dma_map_o.valid   := false.B

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

  // Be default nothing will exit our queue
  function_queue.io.deq.ready     := false.B

  val user_id   = function_queue.io.deq.bits.user >> 12
  val user_func = function_queue.io.deq.bits.user(11,0)

  val send_id = RegInit(0.asUInt(16.W))
  // send_id := next_send_id

  val dbuff_id = RegInit(0.asUInt(6.W))
  // dbuff_id := next_dbuff_id

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
           val msghdr_send = function_queue.io.deq.bits.data.asTypeOf(new msghdr_send_t)

           msghdr_send.send_id := send_id

           io.sendmsg_o.bits.rpc_id    := msghdr_send.send_id
           io.sendmsg_o.bits.dbuff_id  := dbuff_id;
           io.sendmsg_o.bits.remaining := msghdr_send.buff_size
           io.sendmsg_o.bits.dbuffered := msghdr_send.buff_size
           io.sendmsg_o.bits.granted   := 0.U
           io.sendmsg_o.bits.priority  := queue_priority.ACTIVE.asUInt;

           io.fetchdata_o.bits.rpc_id    := msghdr_send.send_id
           io.fetchdata_o.bits.dbuff_id  := 1.U // TODO placeholder
           io.fetchdata_o.bits.remaining := msghdr_send.buff_size
           io.fetchdata_o.bits.dbuffered := 0.U
           io.fetchdata_o.bits.granted   := msghdr_send.buff_size
           io.fetchdata_o.bits.priority  := queue_priority.ACTIVE.asUInt;

           function_queue.io.deq.ready := io.sendmsg_o.ready & io.fetchdata_o.ready
           io.sendmsg_o.valid          := function_queue.io.deq.valid
           io.fetchdata_o.valid        := function_queue.io.deq.valid

           when (function_queue.io.deq.fire) {
             send_id  := send_id + 1.U
             dbuff_id := dbuff_id + 1.U
           }
         }

         // new recvmsg
         is (64.U) {
         }
       }
    }
  }

  /* DEBUGGING ILAS */

  val dma_map_ila = Module(new ILA(new dma_map_t))
  dma_map_ila.io.ila_data := io.dma_map_o.bits
}
