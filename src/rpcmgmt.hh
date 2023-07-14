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
   void rpc_stack(hls::stream<rpc_id_t> & new_rpc_id_0_o,
         hls::stream<rpc_id_t> & new_rpc_id_1_o);

   void rpc_state(hls::stream<sendmsg_t> & sendmsg_i,
         hls::stream<sendmsg_t> & sendmsg_o,
         hls::stream<recvmsg_t> & recvmsg_i,
         hls::stream<recvmsg_t> & recvmsg_o,
         hls::stream<header_t> & header_out_i, 
         hls::stream<header_t> & header_out_o,
         hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_dbuff_o,
         hls::stream<ap_uint<58>> & header_in_grant_o,
         hls::stream<header_t> & header_in_srpt_o);

   void rpc_map(hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_o,
         hls::stream<recvmsg_t> & recvmsg_i);
}
#endif
