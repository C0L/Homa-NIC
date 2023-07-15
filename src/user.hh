#ifndef DMA_H
#define DMA_H

#include "homa.hh"

extern "C"{
   void homa_recvmsg(hls::stream<ap_uint<416>> & recvmsg_i,
         hls::stream<recvmsg_t> & recvmsg_o);

   void homa_sendmsg(hls::stream<ap_uint<512>> & sendmsg_i,
         hls::stream<sendmsg_t> & sendmsg_o);
}

#endif
