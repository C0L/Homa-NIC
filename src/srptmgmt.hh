#ifndef SRPTMGMT_H
#define SRPTMGMT_H

#include "homa.hh"
#include "rpcmgmt.hh"

void srpt_data_pkts(hls::stream<srpt_sendmsg_t> & sendmsg_i,
		    hls::stream<srpt_dbuff_notif_t> & dbuff_notif_i,
		    hls::stream<srpt_pktq_t> & data_pkt_o,
		    hls::stream<srpt_grant_notif_t> & grant_notif_i,
                    hls::stream<srpt_sendq_t> & cache_req_o);

void srpt_grant_pkts(hls::stream<srpt_grant_in_t> & grant_in_i,
      hls::stream<srpt_grant_out_t> & grant_out_o);

#endif
