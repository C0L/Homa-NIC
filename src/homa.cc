#include <iostream>
#include <ap_int.h>

#include "ap_axi_sdata.h"
#include "hls_task.h"
#include "hls_stream.h"

#include "homa.hh"
#include "dma.hh"
#include "link.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"
#include "databuff.hh"
#include "timer.hh"

using namespace std;

void tmp(hls::stream<dbuff_id_t> & freed_dbuffs) {
  if (freed_dbuffs.size() == 0) {
    freed_dbuffs.write(0);
  }
}

/**
 * homa() - Top level homa packet processor
 * @homa:
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @link_egress:  The outgoing AXI Stream of ethernet frames from to the link
 * @dma:   DMA memory space pointer
 */
void homa(homa_t * homa,
	  params_t * params,
	  //hls::stream<raw_frame_t> & link_ingress, // TODO May need more explicit stream interface for this as well?
	  hls::stream<raw_stream_t> & link_ingress,
	  hls::stream<raw_stream_t> & link_egress,
	  hls::burst_maxi<dbuff_chunk_t> maxi_in, // TODO use same interface for both axi then remove bundle
	  dbuff_chunk_t * maxi_out) {

#pragma HLS interface s_axilite port=homa bundle=config
#pragma HLS interface s_axilite port=params bundle=onboard

#pragma HLS interface axis port=link_ingress
#pragma HLS interface axis port=link_egress

#pragma HLS interface mode=m_axi port=maxi_in bundle=maxi_0 latency=60 num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16
#pragma HLS interface mode=m_axi port=maxi_out bundle=maxi_1 latency=60 num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16

  // https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Tasks-and-Channels critical
  // Dataflow required for m_axi https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Tasks-and-Dataflow

  /*
   * Tasks are effectively free-running threads that are infinitely looping
   * This more explicitly defines the parallelism of the processor
   * However, the way in which tasks interact is restricted—they can only comunicate via streams
   * As a result we are effectively turning every function call in the linux implementation into a stream interaction
   */
  hls_thread_local hls::stream<dbuff_id_t> freed_dbuffs;
  hls_thread_local hls::stream<dbuff_in_t> dma_ingress__dbuff;
  hls_thread_local hls::stream<new_rpc_t> dma_ingress__peer_map;
  hls_thread_local hls::stream<new_rpc_t> peer_map__rpc_state;
  hls_thread_local hls::stream<new_rpc_t> rpc_state__srpt_data;
  hls_thread_local hls::stream<srpt_data_t> srpt_data__egress_sel;
  hls_thread_local hls::stream<srpt_grant_t> srpt_grant__egress_sel;
  hls_thread_local hls::stream<rexmit_t> rexmit__egress_sel;
  hls_thread_local hls::stream<out_pkt_t> egress_sel__rpc_state;
  hls_thread_local hls::stream<out_pkt_t> rpc_state__chunk_dispatch;
  hls_thread_local hls::stream<out_block_t> chunk_dispatch__dbuff;
  hls_thread_local hls::stream<dbuff_notif_t> dbuff__srpt_data;

#pragma HLS dataflow


  /* Data Driven Region */

  hls_thread_local hls::task peer_map_task(peer_map,
					   dma_ingress__peer_map, peer_map__rpc_state);

  hls_thread_local hls::task rpc_state_task(rpc_state,
					    peer_map__rpc_state, rpc_state__srpt_data,
					    egress_sel__rpc_state, rpc_state__chunk_dispatch);

  hls_thread_local hls::task srpt_data_pkts_task(srpt_data_pkts,
						 rpc_state__srpt_data,
						 dbuff__srpt_data,
						 srpt_data__egress_sel);

  hls_thread_local hls::task egress_selector_task(egress_selector,
						  srpt_data__egress_sel,
						  srpt_grant__egress_sel,
						  rexmit__egress_sel,
						  egress_sel__rpc_state);

  hls_thread_local hls::task pkt_chunk_dispatch_task(pkt_chunk_dispatch,
						     rpc_state__chunk_dispatch, chunk_dispatch__dbuff);

  hls_thread_local hls::task update_dbuff_task(update_dbuff,
					       dma_ingress__dbuff,
					       dbuff__srpt_data,
					       chunk_dispatch__dbuff, link_egress); 

  hls_thread_local hls::task tmp_task(tmp, freed_dbuffs);

  /* Control Driven Region */
  dma_ingress(homa, params, maxi_in, freed_dbuffs, dma_ingress__dbuff, dma_ingress__peer_map);
}

