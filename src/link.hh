#ifndef LINK_H
#define LINK_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "rpcmgmt.hh"
#include "dma.hh"
#include "timer.hh"

void egress_selector(hls::stream<srpt_data_t> & data_pkt_i,
		     hls::stream<srpt_grant_t> & grant_pkt_i,
		     hls::stream<rexmit_t> & rexmit_pkt_i,
		     hls::stream<out_pkt_t> & out_pkt_o);

void pkt_chunk_dispatch(stream_t<out_pkt_t, out_block_t> out_pkt_s);


//void pkt_generator(hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_next_in,
//		   hls::stream<srpt_grant_entry_t> & srpt_grant_queue_next_in,
//		   hls::stream<rexmit_t> & rexmit_rpcs_in,
//		   hls::stream<pending_pkt_t> & pending_pkt_out);
//
//void pkt_buffer(hls::stream<pending_pkt_t> & pending_pkt_in,
//		hls::stream<pkt_block_t> & pkt_block_out);
//
//void link_egress(hls::stream<pkt_block_t> & pkt_block_in,
//		 hls::stream<raw_stream_t> & link_egress_out);
//
//void link_ingress(hls::stream<raw_frame_t> & link_ingress,
//		  hls::stream<srpt_grant_entry_t> & srpt_grant_queue_insert,
//		  hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_update,
//		  hls::stream<rpc_hashpack_t> & rpc_table_request,
//		  hls::stream<rpc_id_t> & rpc_table_response,
//		  hls::stream<homa_rpc_t> & rpc_table_insert,
//		  hls::stream<rpc_id_t> & rpc_buffer_request,   		       
//		  hls::stream<homa_rpc_t> & rpc_buffer_response,
//		  hls::stream<homa_rpc_t> & rpc_buffer_insert,
//		  hls::stream<rpc_id_t> & rpc_stack_next,
//		  hls::stream<peer_hashpack_t> & peer_table_request,
//		  hls::stream<peer_id_t> & peer_table_response,
//		  hls::stream<homa_peer_t> & peer_buffer_insert,
//		  hls::stream<homa_peer_t> & peer_table_insert,
//		  hls::stream<peer_id_t> & peer_stack_next,
//		  hls::stream<touch_t> & rexmit_touch,
//		  hls::stream<dma_egress_req_t> & dma_egress_reqs);

#endif
