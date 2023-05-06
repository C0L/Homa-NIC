#ifndef LINK_H
#define LINK_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "rpcmgmt.hh"
#include "dma.hh"

void proc_link_egress(hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_next,
		      hls::stream<srpt_grant_entry_t> & srpt_grant_queue_next,
		      hls::stream<xmit_req_t> & xmit_buffer_request,
		      hls::stream<xmit_mblock_t> & xmit_buffer_response,
		      hls::stream<rpc_id_t> & rpc_buffer_request,
		      hls::stream<homa_rpc_t> & rpc_buffer_response,
		      hls::stream<raw_frame_t> & link_egress);

void proc_link_ingress(hls::stream<raw_frame_t> & link_ingress,
		       hls::stream<srpt_grant_entry_t> & srpt_grant_queue_insert,
		       hls::stream<srpt_xmit_entry_t> & srpt_xmit_queue_update,
		       hls::stream<dma_egress_req_t> & dma_egress_reqs);

#endif
