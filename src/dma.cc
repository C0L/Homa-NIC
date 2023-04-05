#include "homa.hh"
#include "dma.hh"
#include <string.h>

/*
 *
 */
void proc_dma_ingress(hls::stream<user_input_t> & dma_ingress,
		      homa_rpc_t * rpcs,
		      char * ddr_ram,
		      user_output_t * dma_egress,
		      rpc_stack_t & rpc_stack,
		      srpt_queue_t & srpt_queue) {

  user_input_t dma_in;

#pragma HLS pipeline
  if (!dma_ingress.empty() && rpc_stack.size != 0) {

    dma_ingress.read(dma_in);

    #ifdef DEBUG
    // Ignored during synthesis
    std::cerr << "DEBUG: Processing Incoming DMA Request:" << std::endl;
    std::cerr << "DEBUG:     -> Message Length = " << dma_in.length << std::endl;
    std::cerr << "DEBUG:     -> Destination Port = " << dma_in.dport << std::endl;
    std::cerr << "DEBUG:     -> Output Slot = " << dma_in.output_slot << std::endl;
    #endif

    #ifdef DEBUG
    std::cerr << "DEBUG: Getting Availible RPC:" << std::endl;
    std::cerr << "DEBUG:     -> Availible RPCs = " << rpc_stack.size << std::endl;
    #endif
    rpc_id_t rpc_id = rpc_stack.pop();
    #ifdef DEBUG
    std::cerr << "DEBUG:     -> Popped RPC = " << rpc_id << std::endl;
    #endif

    homa_rpc_t * homa_rpc = &(rpcs[rpc_id]);
    homa_rpc->dport                   = dma_in.dport;
    homa_rpc->homa_message_out.length = dma_in.length;

    // AXI Burst 
    if (homa_rpc->homa_message_out.length < HOMA_MESSAGE_CACHE) {
      #ifdef DEBUG
      std::cerr << "DEBUG: Cacheable Message" << std::endl;
      #endif
      memcpy(&(homa_rpc->homa_message_out.message), dma_in.message, dma_in.length);
    } else {
      #ifdef DEBUG
      std::cerr << "DEBUG: Uncacheable Message" << std::endl;
      #endif
      memcpy(&(homa_rpc->homa_message_out.message), dma_in.message, HOMA_MESSAGE_CACHE);
      memcpy(ddr_ram + (HOMA_MAX_MESSAGE_LENGTH * rpc_id), dma_in.message + HOMA_MESSAGE_CACHE, dma_in.length - HOMA_MESSAGE_CACHE);
    }

    // Write the RPC corresponding to this message to the user's output DMA buffer
    homa_rpc->homa_message_in.dma_out = (dma_egress + (dma_in.output_slot * sizeof(user_output_t)));
    homa_rpc->homa_message_in.dma_out->rpc_id = rpc_id;

    // Add to SRPT queue
    srpt_queue.push(rpc_id, homa_rpc->homa_message_out.length);
  }
}

