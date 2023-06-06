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

void pkt_chunk_dispatch(hls::stream<out_pkt_t> & rpc_state__chunk_dispatch,
			hls::stream<out_block_t> & chunk_dispatch__dbuff);

#endif
