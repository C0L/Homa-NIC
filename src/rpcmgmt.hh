#ifndef RPCMGMT_H
#define RPCMGMT_H

#include "homa.hh"
#include "srptmgmt.hh"
#include "peer.hh"
#include "databuff.hh"
#include "stack.hh"

struct rpc_hashpack_t {
   ap_uint<128> s6_addr;
   ap_uint<64> id;
   ap_uint<16> port;
   ap_uint<16> empty;

   bool operator==(const rpc_hashpack_t & other) const {
      return (s6_addr == other.s6_addr && id == other.id && port == other.port);
   }
};

extern "C"{

   void rpc_state(hls::stream<onboard_send_t> & onboard_send_i,
         hls::stream<srpt_data_in_t> & onboard_send_o,
         hls::stream<header_t> & header_out_i, 
         hls::stream<header_t> & header_out_o,
         hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_dbuff_o,
         hls::stream<srpt_grant_in_t> & grant_srpt_o,
         hls::stream<srpt_grant_notif_t> & data_srpt_o);

   void rpc_map(hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_o,
         hls::stream<onboard_send_t> & sendmsg_i,
         hls::stream<onboard_send_t> & sendmsg_o);
}
#endif
