#ifndef SRPTMGMT_H
#define SRPTMGMT_H

#include "homa.hh"
#include "rpcmgmt.hh"


void srpt_fetch_queue(hls::stream<srpt_queue_entry_t> & sendmsg_i,
                    hls::stream<srpt_queue_entry_t> & cache_req_o);

void srpt_data_queue(hls::stream<srpt_queue_entry_t> & sendmsg_i,
		    hls::stream<srpt_queue_entry_t> & dbuff_notif_i,
		    hls::stream<srpt_queue_entry_t> & data_pkt_o,
		     hls::stream<srpt_queue_entry_t> & grant_notif_i);

void srpt_grant_pkts(hls::stream<srpt_grant_new_t> & grant_in_i,
		     hls::stream<srpt_grant_send_t> & grant_out_o);

#endif
