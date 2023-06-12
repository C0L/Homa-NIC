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
	  sendmsg_t * sendmsg,
	  recvmsg_t * recvmsg,
	  hls::stream<raw_stream_t> & link_ingress,
	  hls::stream<raw_stream_t> & link_egress,
	  dbuff_chunk_t * maxi_in,
	  dbuff_chunk_t * maxi_out) {

#pragma HLS interface mode=ap_ctrl_none port=return

#pragma HLS interface s_axilite port=homa bundle=homa
#pragma HLS interface s_axilite port=sendmsg bundle=sendmsg
#pragma HLS interface mode=ap_vld port=sendmsg
#pragma HLS interface s_axilite port=recvmsg bundle=recvmsg
#pragma HLS interface mode=ap_vld port=recvmsg

#pragma HLS interface axis port=link_ingress
#pragma HLS interface axis port=link_egress

#pragma HLS interface mode=m_axi port=maxi_in bundle=maxi_0 latency=60 num_read_outstanding=60 num_write_outstanding=1 
#pragma HLS interface mode=m_axi port=maxi_out bundle=maxi_1 latency=60 num_read_outstanding=1 num_write_outstanding=80 


  // https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Tasks-and-Channels critical
  // Dataflow required for m_axi https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/Tasks-and-Dataflow

  /*
   * Tasks are effectively free-running threads that are infinitely looping
   * This more explicitly defines the parallelism of the processor
   * However, the way in which tasks interact is restricted—they can only comunicate via streams
   */
  hls_thread_local hls::stream<dbuff_id_t> freed_dbuffs;
  hls_thread_local hls::stream<dbuff_in_t> new_rpc__dbuff_ingress;

  hls_thread_local hls::stream<sendmsg_t> new_rpc__peer_map;
  hls_thread_local hls::stream<sendmsg_t> peer_map__rpc_state;
  hls_thread_local hls::stream<sendmsg_t> rpc_state__srpt_data;

  hls_thread_local hls::stream<ready_data_pkt_t> srpt_data__egress_sel;
  hls_thread_local hls::stream<ready_grant_pkt_t> srpt_grant__egress_sel;
  hls_thread_local hls::stream<rexmit_t> rexmit__egress_sel;

  hls_thread_local hls::stream<header_t> egress_sel__rpc_state; // header_out
  hls_thread_local hls::stream<header_t> rpc_state__chunk_egress; // header_out
  hls_thread_local hls::stream<header_t> peer_map__rpc_map; // header_in
  hls_thread_local hls::stream<header_t> rpc_map__rpc_state; //header_in

  hls_thread_local hls::stream<out_chunk_t> chunk_egress__dbuff_ingress;
  hls_thread_local hls::stream<dbuff_notif_t> dbuff_ingress__srpt_data;
  hls_thread_local hls::stream<dma_r_req_t> new_rpc__dma_read;
  hls_thread_local hls::stream<in_chunk_t> chunk_ingress__dbuff_ingress;
  hls_thread_local hls::stream<dma_w_req_t> dbuff_ingress__dma_write;
  hls_thread_local hls::stream<header_t> rpc_state__dbuff_ingress;
  hls_thread_local hls::stream<srpt_grant_t> rpc_state__srpt_grant;
  hls_thread_local hls::stream<header_t> chunk_ingress__peer_map;
  
  hls_thread_local hls::stream<recvmsg_t> recvmsg__peer_map__rpc_map;
  hls_thread_local hls::stream<recvmsg_t> recvmsg__rpc_map__rpc_state;
  hls_thread_local hls::stream<recvmsg_t> recvmsg__peer_map;

#pragma HLS dataflow

  /* Data Driven Region */

  hls_thread_local hls::task peer_map_task(peer_map,
					   new_rpc__peer_map, peer_map__rpc_state,
					   recvmsg__peer_map, recvmsg__peer_map__rpc_map,
					   chunk_ingress__peer_map, peer_map__rpc_map);

  hls_thread_local hls::task rpc_state_task(rpc_state,
					    peer_map__rpc_state, rpc_state__srpt_data, // sendmsg
					    recvmsg__rpc_map__rpc_state, // recvmsg
					    egress_sel__rpc_state, rpc_state__chunk_egress, // header_out
					    rpc_state__dbuff_ingress, rpc_map__rpc_state, // header_in
					    rpc_state__srpt_grant);

  hls_thread_local hls::task rpc_map_task(rpc_map,
					  peer_map__rpc_map,
					  rpc_map__rpc_state,
					  recvmsg__peer_map__rpc_map,
					  recvmsg__rpc_map__rpc_state);


  hls_thread_local hls::task srpt_data_pkts_task(srpt_data_pkts,
						 rpc_state__srpt_data,
						 dbuff_ingress__srpt_data,
						 srpt_data__egress_sel);

  hls_thread_local hls::task srpt_grant_pkts_task(srpt_grant_pkts,
						  rpc_state__srpt_grant,
						  srpt_grant__egress_sel);

  hls_thread_local hls::task egress_selector_task(egress_selector,
						  srpt_data__egress_sel,
						  srpt_grant__egress_sel,
						  rexmit__egress_sel,
						  egress_sel__rpc_state);

  hls_thread_local hls::task pkt_chunk_egress_task(pkt_chunk_egress,
						   rpc_state__chunk_egress, chunk_egress__dbuff_ingress);

  hls_thread_local hls::task pkt_chunk_ingress_task(pkt_chunk_ingress,
						    link_ingress,
						    chunk_ingress__peer_map,
						    chunk_ingress__dbuff_ingress);

  hls_thread_local hls::task dbuff_egress_task(dbuff_egress,
					       new_rpc__dbuff_ingress,
					       dbuff_ingress__srpt_data,
					       chunk_egress__dbuff_ingress, link_egress); 

  hls_thread_local hls::task dbuff_ingress_task(dbuff_ingress,
						chunk_ingress__dbuff_ingress,
						dbuff_ingress__dma_write,
						rpc_state__dbuff_ingress);


  hls_thread_local hls::task tmp_task(tmp, freed_dbuffs);

  /* Control Driven Region */
  homa_sendmsg(homa, sendmsg, new_rpc__dma_read, freed_dbuffs, new_rpc__peer_map);

  homa_recvmsg(homa, recvmsg, recvmsg__peer_map);

  dma_read(maxi_in, new_rpc__dma_read, new_rpc__dbuff_ingress);

  dma_write(maxi_out, dbuff_ingress__dma_write);
}

