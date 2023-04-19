#include <string.h>

#include "ap_int.h"
#include "dma.hh"
#include "rpcmgmt.hh"
#include "peer.hh"
#include "srptmgmt.hh"

// TODO: The first HOMA_MESSAGE_CACHE worth of data could be sent in the initial DMA transaction. I am not sure how much of an effect this would have on performance however.

/**
 * proc_dma_ingress()
 * @homa:
 * @user_req:
 * @dma:
 */
void dma_ingress(homa_t * homa,
		 hls::stream<args_t> & sdma,
		 hls::burst_maxi<ap_uint<512>> mdma) {
		 //char * mdma) {
		 //ap_uint<512> * mdma) {
  #pragma HLS inline off

  // From homa_utils.c
  // TODO this will need to wrap around smartly
  static ap_uint<64> next_outgoing_id = 2;
  
  // TODO This introduces extra DMA operations that are uncessesary
  args_t args;
  
  // Try to read data from the AXI Stream buffer
  if (sdma.read_nb(args)) {
    /*
     * If the input id is 0, we should create a new client (homa_rpc_new_client)
     * If the input id is not 0, we should find the active associated RPC
     */
    //homa_rpc_t * homa_rpc;
    homa_rpc_t * homa_rpc;

    homa_rpc_id_t rec_id;

    homa_rpc = &rpcs[args.id];

    // Initial RPC state configuration depending on request vs response
    if (!args.id) {
      // TODO need rpc stack for a local ID that will be assigned to this server RPC
      homa_rpc = &rpcs[args.id];

      homa_rpc->completion_cookie = args.completion_cookie;

      /* homa_rpc_new_client */
      homa_rpc->id = next_outgoing_id + 2;
      homa_rpc->state = homa_rpc_t::RPC_OUTGOING;
      homa_rpc->flags = 0;
      homa_rpc->grants_in_progress = 0;

      // TODO add the RPC client!!

      // TODO Reintroduce
      //homa_rpc->peer = homa_peer_find(args.dest_addr);
      //homa_rpc->dport = ntphs(args.dest_addr.in6.sin6_port);
      homa_rpc->msgin.total_length = -1;
      homa_rpc->msgout.length = -1;
      homa_rpc->silent_ticks = 0;
      homa_rpc->resend_timer_ticks = homa->timer_ticks;
      homa_rpc->done_timer_ticks = 0;
      homa_rpc->buffout = args.buffout;
      homa_rpc->buffin = args.buffin;

      //hashpack_t hashpack = {homa_rpc->addr.s6_addr, homa_rpc->id, (uint32_t) homa_rpc->dport};
      homa_rpc_id_t test = (homa_rpc_id_t) args.id;
      // TODO rm this
      homa_insert_server_rpc(args.dest_addr, args.id, test);

      rec_id = homa_find_server_rpc(args.dest_addr, args.id);

      homa_insert_server_rpc(args.dest_addr, args.id, rec_id);
    } else {
      // TODO we are assuming input address is always IPv6 and little endian
      //in6_addr_t canonical_dest = canonical_ipv6_addr(args.dest_addr);

      homa_rpc_id_t homa_rpc_id = homa_find_server_rpc(args.dest_addr, args.id);

      /* homa_find_server_rpc */
      homa_rpc = &rpcs[homa_rpc_id];

	//homa_find_server_rpc(args.dest_addr, args.id);


      // Find the active RPC
      //homa_rpc = &rpcs[dma_in.id-1];
      homa_rpc->state = homa_rpc_t::RPC_OUTGOING;
    }

    /* Outgoing message initialization */
    homa_rpc->msgout.length = args.length;
    // TODO copy args.length bytes in if less than MESSAGE cache
    // TODO reintroduce
    // TODO move all message buffer outside of homa_rpc?

    mdma.read_request(args.buffin, HOMA_MESSAGE_CACHE);
    for (int i = 0; i < 235; i++) {
#pragma HLS PIPELINE II=1
      homa_rpc->msgout.message[i] = mdma.read();
    }

    

    //memcpy(homa_rpc->msgout.message, mdma + args.buffin, HOMA_MESSAGE_CACHE);
    //memcpy(homa_rpc->msgout.message, mdma + args.buffin, HOMA_MESSAGE_CACHE);
    //for (int i = 0; i < 235; ++i) {
    //#pragma HLS unroll
    //homa_rpc->msgout.message[i] = *((ap_uint<512>*)mdma + i);
      //homa_rpc->msgout.message[i] = *((ap_uint<512>*)mdma + args.buffin + i);
    //}

    homa_rpc->msgout.next_xmit_offset = 0;
    homa_rpc->msgout.sched_priority = 0;
    homa_rpc->msgout.init_cycles = timer;
  
    int max_pkt_data = homa->mtu - IPV6_HEADER_LENGTH - sizeof(data_header_t);
  
    // Does the message fit in a single packet?
    if (homa_rpc->msgout.length <= max_pkt_data) {
      homa_rpc->msgout.unscheduled = homa_rpc->msgout.length;
    } else {
      homa_rpc->msgout.unscheduled = homa->rtt_bytes;
      if (homa_rpc->msgout.unscheduled > homa_rpc->msgout.length)
  	homa_rpc->msgout.unscheduled = homa_rpc->msgout.length;
    }

    // An RPC is automatically granted the unscheduled number of bytes
    homa_rpc->msgout.granted = homa_rpc->msgout.unscheduled;

    //srpt_queue.push(args.id, homa_rpc->msgout.unscheduled);
  }
}


      //homa_rpc_id_t rpc_id = rpc_stack.pop();
      //homa_rpc = &rpcs[rpc_id-1];
