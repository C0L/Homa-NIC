#include "homa.hh"
#include "dma.hh"

/* TODO description of what does this function accomplish
 *
 */
void proc_dma(hls::stream<request_in>  & dma_ingress,
	      hls::stream<request_out> & dma_egress,
	      homa_rpc (&rpcs)[MAX_RPC]) {
  static ;

  // Ensure output stream is open
  if (dma.egress.empty()) {
    // pop from complete messages rpcs
    // if any exist
    // read into message_out struct
    dma.egress.write();

    // mark rpc as open (delete?) add to queue/stack
    //
    
  }
}

