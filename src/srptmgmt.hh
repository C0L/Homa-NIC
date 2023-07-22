#ifndef SRPTMGMT_H
#define SRPTMGMT_H

#include <iostream>

#include "homa.hh"
#include "rpcmgmt.hh"

void srpt_data_pkts(hls::stream<srpt_data_in_t> & sendmsg_raw_i,
      hls::stream<srpt_dbuff_notif_t> & dbuff_notif_raw_i,
      hls::stream<srpt_data_out_t> & data_pkt_raw_o,
      hls::stream<srpt_grant_notif_t> & grant_notif_raw_i);

void srpt_grant_pkts(hls::stream<srpt_grant_in_t> & grant_in_i,
      hls::stream<srpt_grant_out_t> & grant_out_o);

#endif
