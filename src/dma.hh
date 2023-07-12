#ifndef DMA_H
#define DMA_H

#include "homa.hh"
extern "C"{
void homa_sendmsg(hls::stream<sendmsg_t,VERIF_DEPTH> & sendmsg_i,
                  hls::stream<sendmsg_t,VERIF_DEPTH> & sendmsg_o);
}
extern "C"{
void homa_recvmsg(hls::stream<recvmsg_t, VERIF_DEPTH> & recvmsg_i,
                  hls::stream<recvmsg_t, VERIF_DEPTH> & recvmsg_o);
}
#endif
