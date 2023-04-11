#ifndef DMA_H
#define DMA_H

#include <hls_stream.h>

#include "net.hh"
#include "homa.hh"

struct homa_t;

/**
 * user_input_t - Data required to initiate a Homa request or response transaction.
 * This is copied on-chip via an AXI Stream interface from DMA.
 *
 * This fills the role of msghdr in homa_send, but contains no pointer data.
 */
struct dma_in_t {
  // TODO this interface may change slightly
  /**
   * @output_offset: 
   */
  int output_offset;

  int input_offset;
  
  /**
   * @dest_addr: Address of destination
   */
  in6_addr dest_addr;

  /**
   * @ip_header_length: Length of IP headers for this socket (depends
   * on IPv4 vs. IPv6).
   */
  //int ip_header_length;
 
  /**
   * @message: Number of elements in message 
   */
  int length;

  /**
   * @id: 0 for request message, otherwise, ID of the RPC
   */
  ap_uint<64> id;

  /**
   * @completion_cookie - For requests only; value to return
   * along with response.
   */
  ap_uint<64> completion_cookie;

  /**
   * @message: Actual message content
   */
  //char * message;
  //int message_offset;
  //char message[HOMA_MAX_MESSAGE_LENGTH];
};

struct user_output_t {
  int rpc_id;
  char message[HOMA_MAX_MESSAGE_LENGTH];
};

void proc_dma_ingress(homa_t * homa,
		      hls::stream<dma_in_t> & user_req,
		      ap_uint<512> * dma);

#endif
