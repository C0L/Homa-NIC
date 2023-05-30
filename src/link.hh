#ifndef LINK_H
#define LINK_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "rpcmgmt.hh"
#include "dma.hh"
#include "timer.hh"

struct pkt_blocks_t {
  #define FIFO_SIZE 4
  pending_pkt_t buffer[FIFO_SIZE];

  int read_head;

  pkt_blocks_t() {
    read_head = -1;
  }

  void insert(pending_pkt_t value) {
#pragma HLS array_partition variable=buffer type=complete
    for (int i = FIFO_SIZE-2; i > 0; --i) {
#pragma HLS unroll
      buffer[i+1] = buffer[i];
    }
    
    buffer[0] = value;
    
    read_head++;
  }

  pending_pkt_t remove() {
#pragma HLS array_partition variable=buffer type=complete
    pending_pkt_t val = buffer[read_head];
    val.sent_bytes += 64;

    if (val.sent_bytes >= val.total_bytes) {
      read_head--;
    }
    return val;
  }

  bool full() {
    return read_head == MAX_SRPT-1;
  }

  bool empty() {
    return read_head == -1;
  }
};


void pkt_generator(hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_next_in,
		   hls::stream<srpt_grant_entry_t> & srpt_grant_queue_next_in,
		   hls::stream<rexmit_t> & rexmit_rpcs_in,
		   hls::stream<pending_pkt_t> & pending_pkt_out);

void pkt_buffer(hls::stream<pending_pkt_t> & pending_pkt_in,
		hls::stream<pkt_block_t> & pkt_block_out);

void link_egress(hls::stream<pkt_block_t> & pkt_block_in,
		 hls::stream<raw_stream_t> & link_egress_out);

void link_ingress(hls::stream<raw_frame_t> & link_ingress,
		  hls::stream<srpt_grant_entry_t> & srpt_grant_queue_insert,
		  hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_update,
		  hls::stream<rpc_hashpack_t> & rpc_table_request,
		  hls::stream<rpc_id_t> & rpc_table_response,
		  hls::stream<homa_rpc_t> & rpc_table_insert,
		  hls::stream<rpc_id_t> & rpc_buffer_request,   		       
		  hls::stream<homa_rpc_t> & rpc_buffer_response,
		  hls::stream<homa_rpc_t> & rpc_buffer_insert,
		  hls::stream<rpc_id_t> & rpc_stack_next,
		  hls::stream<peer_hashpack_t> & peer_table_request,
		  hls::stream<peer_id_t> & peer_table_response,
		  hls::stream<homa_peer_t> & peer_buffer_insert,
		  hls::stream<homa_peer_t> & peer_table_insert,
		  hls::stream<peer_id_t> & peer_stack_next,
		  hls::stream<touch_t> & rexmit_touch,
		  hls::stream<dma_egress_req_t> & dma_egress_reqs);

#endif
