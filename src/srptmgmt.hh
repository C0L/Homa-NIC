#ifndef SRPTMGMT_H
#define SRPTMGMT_H

#include <iostream>

#include "homa.hh"
#include "rpcmgmt.hh"

extern "C"{
   void srpt_data_pkts(hls::stream<sendmsg_t> & new_rpc_i,
         hls::stream<dbuff_notif_t> & data_notif_i,
         hls::stream<ready_data_pkt_t> & data_pkt_o,
         hls::stream<header_t> & header_in_i);

   void srpt_grant_pkts(hls::stream<ap_uint<58>> & header_in_i,
         hls::stream<ap_uint<51>> & grant_pkt_o);
}
#endif
