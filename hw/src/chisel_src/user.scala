package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

class delegate extends Module {
  val io = IO(new Bundle {
    val function_i  = Flipped(new axis(512, false, 0, true, 32, false))
    val sendmsg_o   = Decoupled(new queue_entry_t)
    val fetchdata_o = Decoupled(new queue_entry_t)
    val dma_w_req_o = Decoupled(new dma_write_t)
    val dma_map_o   = Decoupled(new dma_map_t)
  })

  // TODO we are always ready to accept the data?
  io.function_i.tready    := true.B

  io.sendmsg_o.bits   := 0.U.asTypeOf(new queue_entry_t)
  io.fetchdata_o.bits := 0.U.asTypeOf(new queue_entry_t)
  io.dma_w_req_o.bits := 0.U.asTypeOf(new dma_write_t)
  io.dma_map_o.bits   := 0.U.asTypeOf(new dma_map_t)

  io.sendmsg_o.valid   := false.B
  io.fetchdata_o.valid := false.B
  io.dma_w_req_o.valid := false.B
  io.dma_map_o.valid   := false.B

  // TODO add queue

  // TODO this operates on the registered value?
  when(io.function_i.tvalid) {
    /* User bits specify the function of this 512 bit block
     * these bits are assigned from the AXI4 write address
     */
    switch (io.function_i.tuser.get) {
      // TODO This assumes the consumer is always ready

      is (0.U) {
        // TODO This should use the registered value
        io.dma_map_o.enq(io.function_i.tdata.asTypeOf(new dma_map_t))
      }

      is (64.U) {
        val msghdr_send = io.function_i.tdata.asTypeOf(new msghdr_send_t)

        // TODO placeholder
        msghdr_send.send_id := 1.U

	// msghdr_send(MSGHDR_SEND_ID) = msghdr_send(MSGHDR_SEND_ID) == 0 ? client_ids.pop() : msghdr_send(MSGHDR_SEND_ID);
	// TODO this only works for requests

	//ap_uint<HDR_BLOCK_SIZE> cb;

	//cb(HDR_BLOCK_SADDR)    = msghdr_send(MSGHDR_SADDR);
	//cb(HDR_BLOCK_DADDR)    = msghdr_send(MSGHDR_DADDR);
	//cb(HDR_BLOCK_SPORT)    = msghdr_send(MSGHDR_SPORT);
	//cb(HDR_BLOCK_DPORT)    = msghdr_send(MSGHDR_DPORT);
	//cb(HDR_BLOCK_RPC_ID)   = msghdr_send(MSGHDR_SEND_ID);
	//cb(HDR_BLOCK_CC)       = msghdr_send(MSGHDR_SEND_CC);
	//cb(HDR_BLOCK_LOCAL_ID) = msghdr_send(MSGHDR_SEND_ID);
	//// cb(HDR_BLOCK_MSG_ADDR) = msghdr_send(MSGHDR_BUFF_SIZE);
	//cb(HDR_BLOCK_LENGTH)   = msghdr_send(MSGHDR_BUFF_SIZE);

	// cbs[msghdr_send(MSGHDR_SEND_ID)] = cb;

	// ap_uint<32> dbuff_id = msg_cache_ids.pop();

        // TODO this should really be registered
        io.sendmsg_o.bits.rpc_id    := msghdr_send.send_id
        io.sendmsg_o.bits.dbuff_id  := 1.U // TODO placeholder
        io.sendmsg_o.bits.remaining := msghdr_send.buff_size
        io.sendmsg_o.bits.dbuffered := msghdr_send.buff_size
        io.sendmsg_o.bits.granted   := 0.U
        io.sendmsg_o.bits.priority  := 5.U

        // TODO this needs to consider if the receiver is ready
        io.sendmsg_o.valid := true.B

        io.fetchdata_o.bits.rpc_id    := msghdr_send.send_id
        io.fetchdata_o.bits.dbuff_id  := 1.U // TODO placeholder
        io.fetchdata_o.bits.remaining := msghdr_send.buff_size
        io.fetchdata_o.bits.dbuffered := 0.U
        io.fetchdata_o.bits.granted   := msghdr_send.buff_size
        io.fetchdata_o.bits.priority  := 5.U

        io.fetchdata_o.valid := true.B
      }
    }
  }
}
