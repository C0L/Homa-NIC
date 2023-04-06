#include "homa.hh"
#include "dma.hh"
#include <string.h>

// TODO send back ack in every case -- may simplify timeout protocol

/**
 * proc_dma_ingress() - Process user submited Homa requests and responses to the core
 * @dma_ingress: Incoming DMA transfers
 * @rpcs:        RPC store
 * @ddr_ram:     On PCB RAM pointer
 * @dma_egress:  DMA memory space pointer
 * @rpc_stack:   RPCs that are not in use
 * @srpt_queue:  Sorted queue of RPCs ready to broadcast based on smallest 
 *               number of packets remaining
 *
 * This is roughly equivilent to a homa_sendmsg call in HomaModule. Requests are passed via the dma_ingress stream.
 * No socket is neccesary anymore.
 * 
 */
void proc_dma_ingress(hls::stream<user_input_t> & dma_ingress,
		      char * ddr_ram,
		      user_output_t * dma_egress) {
  user_input_t dma_in;

#pragma HLS pipeline
  // Try to read data from the AXI Stream buffer
  if (dma_ingress.read(dma_in)) {
  
#ifdef DEBUG
    std::cerr << "DEBUG: Processing Incoming DMA Request:" << std::endl;
    std::cerr << "DEBUG:     -> output_slot       = " << dma_in.output_slot << std::endl;
    std::cerr << "DEBUG:     -> dest_addr         = " << dma_in.dest_addr << std::endl;
    std::cerr << "DEBUG:     -> length            = " << dma_in.length << std::endl;
    std::cerr << "DEBUG:     -> id                = " << dma_in.id << std::endl;
    std::cerr << "DEBUG:     -> completion_cookie = " << dma_in.completion_cookie << std::endl;
    std::cerr << "DEBUG:     -> First 8 bytes     = " << (ap_uint<32>) dma_in.message << std::endl;
#endif

    // Request or response?
    if (!dma_in.id) {
#ifdef DEBUG
      std::cerr << "DEBUG: This is a request message." << std::endl;
#endif

      rpc = homa_rpc_new_client(output_slot, dma_in.dest_addr);

      // TODO maybe uneccesary as the index in rpcs is the rpc id
      homa_rpc->id = rpc_id;
      homa_rpc->state = RPC_OUTGOING;
      homa_rpc->grants_in_progress = 0;
      homa_rpc->peer = 
	homa_rpc->dport = dma_in.dport;
      homa_rpc->homa_message_out.length = dma_in.length;
  
#ifdef DEBUG
      std::cerr << "DEBUG:     -> Popped RPC     = " << rpc_id << std::endl;
#endif
  
      rpc->completion_cookie = args.completion_cookie;
      //result = homa_message_out_init(rpc, &msg->msg_iter, 1);
  
      args.id = rpc->id;
    } else {
      /* This is a response message. */
      struct in6_addr canonical_dest;
      
      canonical_dest = canonical_ipv6_addr(addr);
      
      //rpc = homa_find_server_rpc(hsk, &canonical_dest,
      //ntohs(addr->in6.sin6_port), args.id);
  
      rpc->state = RPC_OUTGOING;
      
      //result = homa_message_out_init(rpc, &msg->msg_iter, 1);
    }
  
  
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

  // TODO write to DMA buffer upon failure
}
