#ifndef DMA_H
#define DMA_H

#include "homa.hh"
extern "C"{
void homa_sendmsg(hls::stream<hls::axis<sendmsg_t,0,0,0>> & sendmsg_i,
                  hls::stream<sendmsg_t> & sendmsg_o);
}
extern "C"{
void homa_recvmsg(hls::stream<hls::axis<recvmsg_t,0,0,0>> & recvmsg_i,
                  hls::stream<recvmsg_t> & recvmsg_o);
}
#endif
