#ifndef DMA_H
#define DMA_H

#include "homa.hh"
extern "C"{
void homa_sendmsg(hls::stream<sendmsg_t> & sendmsg_i,
                  hls::stream<sendmsg_t> & sendmsg_o);
}
extern "C"{
void homa_recvmsg(hls::stream<recvmsg_t> & recvmsg_i,
                  hls::stream<recvmsg_t> & recvmsg_o);
}
#endif
