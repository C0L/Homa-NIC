#include "homa.hh"
#include "dma.hh"
#include <string.h>

/*
 *
 */
void proc_dma_ingress(hls::stream<user_in> & dma_ingress,
		      homa_rpc (&rpcs)[MAX_RPC],
		      void * ddr_ram,
		      rpc_queue_t & rpc_queue,
		      srpt_queue_t & srpt_queue) {

  static user_in msg_buffer;

#pragma HLS pipeline
  if (dma_ingress.full()) {

    dma_ingress.read(msg_buffer);

    rpc_id_t rpc_id = avail_rpcs.pop();

    //rpcs[rpc_id].dport          = msg_buffer.dport;
    //rpcs[rpc_id].msgout.length  = msg_buffer.length;
    //memcpy(&(rpcs.[rpc_id].msg_out.message), &(msg_buffer.message[0]), HOMA_MESSAGE_CACHE);

    // AXI Burst transaction
    //memcpy(ddr_ram + (HOMA_MAX_MESSAGE_LENGTH * rpc_id), &(msg_buffer.message[HOMA_MESSAGE_CACHE]), msg_buffer.length - HOMA_MESSAGE_CACHE);

    // Add to SRPT queue
  }
}

