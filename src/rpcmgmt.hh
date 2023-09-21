#ifndef RPCMGMT_H
#define RPCMGMT_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "databuff.hh"
#include "stack.hh"


void rpc_state(hls::stream<onboard_send_t> & onboard_send_i,
	       hls::stream<srpt_data_new_t> & onboard_send_o,
	       hls::stream<header_t> & header_out_i, 
	       hls::stream<header_t> & header_out_o,
	       hls::stream<header_t> & header_in_i,
	       hls::stream<header_t> & header_in_dbuff_o,
	       hls::stream<srpt_grant_new_t> & grant_srpt_o,
	       hls::stream<srpt_data_grant_notif_t> & data_srpt_o);

void id_map(hls::stream<header_t> & header_in_i,
	    hls::stream<header_t> & header_in_o);

#endif
