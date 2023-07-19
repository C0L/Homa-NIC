#ifndef SRPTMGMT_H
#define SRPTMGMT_H

#include <iostream>

#include "homa.hh"
#include "rpcmgmt.hh"

// extern "C"{
   void srpt_data_pkts(hls::stream<sendmsg_t> & new_rpc_i,
         hls::stream<dbuff_notif_t> & data_notif_i,
         hls::stream<ready_data_pkt_t> & data_pkt_o,
         hls::stream<header_t> & header_in_i);

   void srpt_grant_pkts(hls::stream<grant_in_t> & grant_in_i,
         hls::stream<grant_out_t> & grant_out_o);
// }
#endif
