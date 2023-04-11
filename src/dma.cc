#include <string.h>

#include "dma.hh"
#include "rpcmgmt.hh"
#include "peer.hh"
#include "srptmgmt.hh"

/**
 * proc_dma_ingress() - Process user submited Homa requests and responses to the core
 * @homa:        Homa runtime configuration parameters
 * @dma_ingress: Incoming DMA transfers
 * @ddr_ram:     On PCB RAM pointer
 * @dma_egress:  DMA memory space pointer
 *
 * This is roughly equivilent to a homa_sendmsg call in HomaModule. Requests are passed via the dma_ingress stream.
 * No socket is neccesary anymore.
 * 
 */
void proc_dma_ingress(homa_t * homa,
		      hls::stream<dma_in_t> & user_req,
		      ap_uint<512> * dma) {
  dma_in_t dma_in;
  
  // Try to read data from the AXI Stream buffer
  if (user_req.read_nb(dma_in)) {
  
    // Request or response?
    homa_rpc_t & homa_rpc = (!dma_in.id) ?
      homa_rpc_new_client(homa, dma_in.output_offset, dma_in.dest_addr) :
      homa_rpc_find(dma_in.id);
  
    // Requests need initial state configuration
    if (!dma_in.id) {
      /** General RPC information */
      homa_rpc.id = dma_in.id;
  
      homa_rpc.grants_in_progress = 0;
      homa_rpc.peer = homa_peer_find(dma_in.dest_addr);
      homa_rpc.completion_cookie = dma_in.completion_cookie;
    }
  
    homa_rpc.state = homa_rpc_t::RPC_OUTGOING;
  
    /** Outgoing message initialization  */
    homa_rpc.msgout.length = dma_in.length;

    // AXI Burst
    // TODO copy dma_in.length bytes in if less than MESSAGE cache
    memcpy(homa_rpc.msgout.message, dma + dma_in.input_offset, HOMA_MESSAGE_CACHE);
  
    homa_rpc.msgout.next_xmit_offset = 0;
    homa_rpc.msgout.sched_priority = 0;
    homa_rpc.msgout.init_cycles = timer;
    homa_rpc.msgout.granted = homa_rpc.msgout.unscheduled;
  
    // TODO should just be ipv6_header_length constant
    int max_pkt_data = homa->mtu - IPV6_HEADER_LENGTH - sizeof(data_header_t);
  
    // Does the message fit in a single packet
    if (homa_rpc.msgout.length <= max_pkt_data) {
      homa_rpc.msgout.unscheduled = homa_rpc.msgout.length;
    } else {
      homa_rpc.msgout.unscheduled = homa->rtt_bytes;
      if (homa_rpc.msgout.unscheduled > homa_rpc.msgout.length)
	homa_rpc.msgout.unscheduled = homa_rpc.msgout.length;
    }
  
    srpt_queue.push(dma_in.id, homa_rpc.msgout.unscheduled);
  }
}
