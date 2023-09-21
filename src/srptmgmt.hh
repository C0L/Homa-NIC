#ifndef SRPTMGMT_H
#define SRPTMGMT_H

#include "homa.hh"
#include "rpcmgmt.hh"

void srpt_data_pkts(hls::stream<srpt_data_new_t> & sendmsg_i,
		    hls::stream<srpt_data_dbuff_notif_t> & dbuff_notif_i,
		    hls::stream<srpt_data_send_t> & data_pkt_o,
		    hls::stream<srpt_data_grant_notif_t> & grant_notif_i,
                    hls::stream<srpt_data_fetch_t> & cache_req_o);

void srpt_grant_pkts(hls::stream<srpt_grant_new_t> & grant_in_i,
		     hls::stream<srpt_grant_send_t> & grant_out_o);

#endif
