#ifndef DMA_H
#define DMA_H

#include <hls_stream.h>
#include <stdint.h>

#include "net.hh"
#include "homa.hh"


struct homa_t;

/**
 * dma_in_t - Data required to initiate a Homa request or response transaction.
 * This is copied on-chip via an AXI Stream interface from DMA.
 *
 * This fills the role of msghdr in homa_send, but contains no pointer data.
 */
struct args_t {
  // TODO this interface may change slightly
  /**
   * @buffout: Offset in DMA space to place result of request/response
   */
  uint32_t buffout;

  /**
   * @buffin: Offset in DMA space to pull message to send
   */
  uint32_t buffin;

  /**
   * @message: Number of elements in message 
   */
  int length;
 
  /**
   * @dest_addr: Address of destination
   */
  sockaddr_in6_t dest_addr;

  /**
   * @id: 0 for request message, otherwise, ID of the RPC
   */
  uint64_t id;

  /**
   * @completion_cookie - For requests only; value to return
   * along with response.
   */
  uint64_t completion_cookie;
};

void dma_ingress(homa_t * homa,
		 hls::stream<args_t> & sdma,
		 hls::burst_maxi<ap_uint<512>> mdma);
		 //char * mdma);
		 //ap_uint<512> * dma);

#endif
