#ifndef DMA_H
#define DMA_H

#include <hls_stream.h>
#include <stdint.h>

#include "homa.hh"

void homa_sendmsg(hls::stream<sendmsg_t> sendmsg,
                  hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_o);

void homa_recvmsg(hls::stream<recvmsg_t> recvmsg,
                  hls::stream<recvmsg_t, VERIF_DEPTH> & recvmsg_o);

void dma_read(hls::stream<dma_r_req_t, VERIF_DEPTH> & rpc_ingress__dma_read,
              hls::stream<  
               hls::stream<dbuff_in_t, VERIF_DEPTH> & dma_requests__dbuff);

void dma_write( const homa_t homa,
               hls::stream<dma_w_req_t, VERIF_DEPTH> & dbuff_ingress__dma_write);

#endif
