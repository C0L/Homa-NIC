#ifndef LINK_H
#define LINK_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "rpcmgmt.hh"
#include "dma.hh"
#include "timer.hh"

void proc_link_egress(hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_next,
		      hls::stream<srpt_grant_entry_t> & srpt_grant_queue_next,
		      hls::stream<xmit_req_t> & xmit_buffer_request,
		      hls::stream<xmit_mblock_t> & xmit_buffer_response,
		      hls::stream<rpc_id_t> & rpc_buffer_request,
		      hls::stream<homa_rpc_t> & rpc_buffer_response,
		      hls::stream<rexmit_t> & rexmit_rpcs,
		      hls::stream<raw_frame_t> & link_egress);

void proc_link_ingress(hls::stream<raw_frame_t> & link_ingress,
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
		       hls::stream<rexmit_t> & rexmit_touch,
		       hls::stream<dma_egress_req_t> & dma_egress_reqs);

#endif
