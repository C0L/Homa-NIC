package gpnic 

import chisel3._
import circt.stage.ChiselStage
import chisel3.util._

/*
void user(
    hls::stream<msghdr_send_t> & sendmsg_i,
    hls::stream<msghdr_send_t> & sendmsg_o,
    hls::stream<msghdr_send_t> & recvmsg_i,
    ap_uint<512> cbs[MAX_RPCS],
    hls::stream<srpt_queue_entry_t> & new_fetch_o,
    hls::stream<srpt_queue_entry_t> & new_sendmsg_o
    ) {

#pragma HLS interface mode=ap_ctrl_none port=return

#pragma HLS pipeline II=1

#pragma HLS interface axis port=sendmsg_i
#pragma HLS interface axis port=sendmsg_o
#pragma HLS interface axis port=recvmsg_i
#pragma HLS interface bram port=cbs
#pragma HLS interface axis port=new_fetch_o
#pragma HLS interface axis port=new_sendmsg_o

    /* Unique local RPC ID assigned when this core is the client */
    static stack_t<local_id_t, MAX_RPCS> client_ids;

    /* Unique local databuffer IDs assigned for outgoing data */
    static stack_t<local_id_t, NUM_EGRESS_BUFFS> msg_cache_ids;
 
    msghdr_send_t msghdr_send;
    if (sendmsg_i.read_nb(msghdr_send)) {

	msghdr_send(MSGHDR_SEND_ID) = msghdr_send(MSGHDR_SEND_ID) == 0 ? client_ids.pop() : msghdr_send(MSGHDR_SEND_ID);

	// TODO this only works for requests

	ap_uint<HDR_BLOCK_SIZE> cb;

	cb(HDR_BLOCK_SADDR)    = msghdr_send(MSGHDR_SADDR);
	cb(HDR_BLOCK_DADDR)    = msghdr_send(MSGHDR_DADDR);
	cb(HDR_BLOCK_SPORT)    = msghdr_send(MSGHDR_SPORT);
	cb(HDR_BLOCK_DPORT)    = msghdr_send(MSGHDR_DPORT);
	cb(HDR_BLOCK_RPC_ID)   = msghdr_send(MSGHDR_SEND_ID);
	cb(HDR_BLOCK_CC)       = msghdr_send(MSGHDR_SEND_CC);
	cb(HDR_BLOCK_LOCAL_ID) = msghdr_send(MSGHDR_SEND_ID);
	// cb(HDR_BLOCK_MSG_ADDR) = msghdr_send(MSGHDR_BUFF_SIZE);
	cb(HDR_BLOCK_LENGTH)   = msghdr_send(MSGHDR_BUFF_SIZE);

	cbs[msghdr_send(MSGHDR_SEND_ID)] = cb;

	ap_uint<32> dbuff_id = msg_cache_ids.pop();

	srpt_queue_entry_t srpt_data_in = 0;
	srpt_data_in(SRPT_QUEUE_ENTRY_RPC_ID)    = msghdr_send(MSGHDR_SEND_ID);
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = dbuff_id;
	srpt_data_in(SRPT_QUEUE_ENTRY_REMAINING) = msghdr_send(MSGHDR_BUFF_SIZE);
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFFERED) = msghdr_send(MSGHDR_BUFF_SIZE); // Fully unbuffered 
	srpt_data_in(SRPT_QUEUE_ENTRY_GRANTED)   = 0; // Fully granted
	//msghdr_send(MSGHDR_BUFF_SIZE) - ((((ap_uint<32>) RTT_BYTES) > rpc.buff_size) ? rpc.buff_size : ((ap_uint<32>) RTT_BYTES));
	srpt_data_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;

	// Insert this entry into the SRPT data queue
	new_sendmsg_o.write(srpt_data_in);
	
	srpt_queue_entry_t srpt_fetch_in = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_RPC_ID)    = msghdr_send(MSGHDR_SEND_ID);
	    // msghdr_send(MSGHDR_SPORT);
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = dbuff_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_REMAINING) = msghdr_send(MSGHDR_BUFF_SIZE);
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_GRANTED)   = msghdr_send(MSGHDR_BUFF_SIZE);
	srpt_fetch_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;
	
	// Insert this entry into the SRPT fetch queue
	new_fetch_o.write(srpt_fetch_in);

	sendmsg_o.write(msghdr_send);
    }
}
*/


class delegate extends Module {
  val io = IO(new Bundle {
    val function    = Flipped(new axis)
    val sendmsg     = Decoupled(new queue_entry_t)
    val fetchdata   = Decoupled(new queue_entry_t)
    val dma_w_req_t = Decoupled(new dma_w_req_t)
  })

  // TODO we are always ready to accept the data?
  io.m_axis.ready := 1.U

  when(io.function.valid) {
    /* User bits specify the function of this 512 bit block
     * these bits are assigned from the AXI4 write address
     */
    switch (io.funtion.user) {
      is (0.U) {
        // TODO This assumes the consumer is always ready
        fetchd
      }
      is (64.U) {
        val msghdr_send := function.asTypeOf(new msghdr_send_t)

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

        val sendmsg := new srpt_queue_entry_t

        sendmsg.rpc_id := msghdr_send.send_id
        sendmsg.dbuff_id := msghdr_send.send_id
        sendmsg.remaining := msghdr_send.send_id
        sendmsg.dbuffered := msghdr_send.send_id
        sendmsg.granted := msghdr_send.send_id

	//srpt_queue_entry_t srpt_data_in = 0;
	//srpt_data_in(SRPT_QUEUE_ENTRY_RPC_ID)    = msghdr_send(MSGHDR_SEND_ID);
	//srpt_data_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = dbuff_id;
	//srpt_data_in(SRPT_QUEUE_ENTRY_REMAINING) = msghdr_send(MSGHDR_BUFF_SIZE);
	//srpt_data_in(SRPT_QUEUE_ENTRY_DBUFFERED) = msghdr_send(MSGHDR_BUFF_SIZE); // Fully unbuffered 
	//srpt_data_in(SRPT_QUEUE_ENTRY_GRANTED)   = 0; // Fully granted
	////msghdr_send(MSGHDR_BUFF_SIZE) - ((((ap_uint<32>) RTT_BYTES) > rpc.buff_size) ? rpc.buff_size : ((ap_uint<32>) RTT_BYTES));
	//srpt_data_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;

	//// Insert this entry into the SRPT data queue
	//new_sendmsg_o.write(srpt_data_in);
	//
	//srpt_queue_entry_t srpt_fetch_in = 0;
	//srpt_fetch_in(SRPT_QUEUE_ENTRY_RPC_ID)    = msghdr_send(MSGHDR_SEND_ID);
	//    // msghdr_send(MSGHDR_SPORT);
	//srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = dbuff_id;
	//srpt_fetch_in(SRPT_QUEUE_ENTRY_REMAINING) = msghdr_send(MSGHDR_BUFF_SIZE);
	//srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
	//srpt_fetch_in(SRPT_QUEUE_ENTRY_GRANTED)   = msghdr_send(MSGHDR_BUFF_SIZE);
	//srpt_fetch_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;
	
	// Insert this entry into the SRPT fetch queue
	new_fetch_o.write(srpt_fetch_in);

	sendmsg_o.write(msghdr_send);
      }
      // is (128.U) {
      //   metadata_maps.write(new_dma_map_i_reg_0_bits.port, new_dma_map_i_reg_0_bits)
      // }
    }
  }


  // TODO will need to store some data associated with the sendmsg somewhere

  // TODO we are always ready for data


}
