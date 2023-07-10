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

/**
 * homa() - Top level homa packet processor
 * @homa:
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @link_egress:  The outgoing AXI Stream of ethernet frames from to the link
 * @dma:   DMA memory space pointer
 */
void homa(const homa_t homa_cfg,
      const sendmsg_t sendmsg,
      const recvmsg_t recvmsg,
      hls::stream<raw_stream_t> & link_ingress,
      hls::stream<raw_stream_t> & link_egress,
      char * maxi_in,
      char * maxi_out) {
   std::cerr << "TOP LEVEL CALL\n";

   // TODO RM?
// #pragma HLS interface mode=ap_ctrl_hs port=return
   // #pragma HLS interface mode=ap_ctrl_none port=return TODO 

#pragma HLS interface s_axilite port=homa_cfg bundle=homa_cfg 

#pragma HLS interface s_axilite port=sendmsg bundle=sendmsg 
   // #pragma HLS interface mode=ap_vld port=sendmsg depth=128

#pragma HLS interface s_axilite port=recvmsg bundle=recvmsg
   // #pragma HLS interface mode=ap_vld port=recvmsg depth=128

   // Depth specifies the maximum number of values streaming in/out for simulation
   //
// TODO depth here seems to be ignored??
#pragma HLS interface axis port=link_ingress depth=512
#pragma HLS interface axis port=link_egress  depth=512

//#pragma HLS stream variable=link_egress depth=512
//#pragma HLS stream variable=link_ingress depth=512

// #pragma HLS interface mode=m_axi port=maxi_in  bundle=maxi_0 latency=70 num_read_outstanding=256  num_write_outstanding=1 max_write_burst_length=2 max_read_burst_length=8  depth=4096
// #pragma HLS interface mode=m_axi port=maxi_out bundle=maxi_1 latency=70 num_write_outstanding=256 num_read_outstanding=1  max_read_burst_length=2  max_write_burst_length=8 depth=4096
//
// #pragma HLS interface mode=m_axi port=maxi_in  bundle=maxi_0 latency=70 num_read_outstanding=256  num_write_outstanding=1 max_write_burst_length=2 max_read_burst_length=8  depth=16000
// #pragma HLS interface mode=m_axi port=maxi_out bundle=maxi_1 latency=70 num_write_outstanding=256 num_read_outstanding=1  max_read_burst_length=2  max_write_burst_length=8 depth=16000

#pragma HLS interface mode=m_axi port=maxi_in  bundle=maxi_0 latency=70 num_read_outstanding=256  num_write_outstanding=1 max_write_burst_length=2 max_read_burst_length=1  depth=16000

#pragma HLS interface mode=m_axi port=maxi_out bundle=maxi_1 latency=70 num_write_outstanding=256 num_read_outstanding=1  max_read_burst_length=2  max_write_burst_length=1 depth=16000

   hls_thread_local hls::stream<dbuff_in_t,       VERIF_DEPTH> dma_read__dbuff_egress          ("dma_read__dbuff_egress");
   hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> homa_sendmsg__dbuff_stack       ("homa_sendmsg__dbuff_stack");
   hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> dbuff_stack__peer_map           ("dbuff_stack__peer_map");
   hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> peer_map__rpc_state             ("peer_map__rpc_state");
   hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> rpc_state__srpt_data            ("rpc_state__srpt_data");
   hls_thread_local hls::stream<ready_data_pkt_t, VERIF_DEPTH> srpt_data__egress_sel           ("srpt_data__egress_sel");
   hls_thread_local hls::stream<ap_uint<51>,      VERIF_DEPTH> srpt_grant__egress_sel          ("srpt_grant__egress_sel");
   hls_thread_local hls::stream<rexmit_t,         VERIF_DEPTH> rexmit__egress_sel              ("rexmit__egress_sel");
   hls_thread_local hls::stream<header_t,         VERIF_DEPTH> egress_sel__rpc_state           ("egress_sel__rpc_state"); 
   hls_thread_local hls::stream<header_t,         VERIF_DEPTH> rpc_state__chunk_egress         ("rpc_state__chunk_egress"); 
   hls_thread_local hls::stream<header_t,         VERIF_DEPTH> peer_map__rpc_map               ("peer_map__rpc_map"); 
   hls_thread_local hls::stream<header_t,         VERIF_DEPTH> rpc_map__rpc_state              ("rpc_map__rpc_state"); 
   hls_thread_local hls::stream<out_chunk_t,      VERIF_DEPTH> chunk_egress__dbuff_ingress     ("chunk_egress__dbuff_ingress");
   hls_thread_local hls::stream<dbuff_notif_t,    VERIF_DEPTH> dbuff_ingress__srpt_data        ("dbuff_ingress__srpt_data");
   hls_thread_local hls::stream<dma_r_req_t,      VERIF_DEPTH> dbuff_stack__dma_read           ("dbuff_stack__dma_read");
   hls_thread_local hls::stream<in_chunk_t,       VERIF_DEPTH> chunk_ingress__dbuff_ingress    ("chunk_ingress__dbuff_ingress");
   hls_thread_local hls::stream<dma_w_req_t,      VERIF_DEPTH> dbuff_ingress__dma_write        ("dbuff_ingress__dma_write");
   hls_thread_local hls::stream<header_t,         VERIF_DEPTH> rpc_state__dbuff_ingress        ("rpc_state__dbuff_ingress");
   hls_thread_local hls::stream<ap_uint<58>,      VERIF_DEPTH> rpc_state__srpt_grant           ("rpc_state__srpt_grant");
   hls_thread_local hls::stream<header_t,         VERIF_DEPTH> header_in__rpc_state__srpt_data ("header_in__rpc_date__srpt_data");
   hls_thread_local hls::stream<header_t,         VERIF_DEPTH> chunk_ingress__rpc_map          ("chunk_ingress__rpc_map");
   hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__peer_map__rpc_state    ("recvmsg__peer_map__rpc_state");
   hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__rpc_state__rpc_map     ("recvmsg__rpc_state__rpc_map");
   hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__peer_map               ("recvmsg__peer_map");

   /* The stable pragma can be used to mark input or output variables of a
    * dataflow region. Its effect is to remove their corresponding task-level
    * synchronizations, assuming that the user guarantees this removal is indeed
    * correct. 
    */
#pragma HLS stable variable=homa_cfg

   // /* "If an unbounded slack between producer and consumer is needed, and internal
   //  * processes can run forever, fully and safely driven by their inputs or
   //  * outputs (FIFOs or PIPOs), these start FIFOs can be removed, at user's risk,
   //  * locally for a given dataflow region with the pragma"
   //  */
// #pragma HLS dataflow disable_start_propagation
#pragma HLS dataflow

   /* Data Driven Region */

   hls_thread_local hls::task peer_map_task(
         peer_map,
         dbuff_stack__peer_map,
         peer_map__rpc_state,
         recvmsg__peer_map,
         recvmsg__peer_map__rpc_state
         );

   hls_thread_local hls::task rpc_state_task(
         rpc_state,
         peer_map__rpc_state,
         rpc_state__srpt_data,
         recvmsg__peer_map__rpc_state,
         recvmsg__rpc_state__rpc_map,
         egress_sel__rpc_state,
         rpc_state__chunk_egress,
         rpc_map__rpc_state,
         rpc_state__dbuff_ingress,
         rpc_state__srpt_grant,
         header_in__rpc_state__srpt_data
         );

   hls_thread_local hls::task rpc_map_task(
         rpc_map,
         chunk_ingress__rpc_map,
         rpc_map__rpc_state,
         recvmsg__rpc_state__rpc_map
         );


   hls_thread_local hls::task srpt_data_pkts_task(
         srpt_data_pkts,
         rpc_state__srpt_data,
         dbuff_ingress__srpt_data,
         srpt_data__egress_sel,
         header_in__rpc_state__srpt_data
         );

   hls_thread_local hls::task srpt_grant_pkts_task(
         srpt_grant_pkts,
         rpc_state__srpt_grant,
         srpt_grant__egress_sel
         );

   hls_thread_local hls::task egress_selector_task(
         egress_selector,
         srpt_data__egress_sel,
         srpt_grant__egress_sel,
         rexmit__egress_sel,
         egress_sel__rpc_state
         );

   hls_thread_local hls::task pkt_chunk_egress_task(
         pkt_chunk_egress,
         rpc_state__chunk_egress,
         chunk_egress__dbuff_ingress
         );

   hls_thread_local hls::task pkt_chunk_ingress_task(
         pkt_chunk_ingress,
         link_ingress,
         chunk_ingress__rpc_map,
         chunk_ingress__dbuff_ingress
         );

   hls_thread_local hls::task dbuff_stack_task(
         dbuff_stack,
         homa_sendmsg__dbuff_stack,
         dbuff_stack__peer_map,
         dbuff_stack__dma_read
         ); 

   hls_thread_local hls::task dbuff_egress_task(
         dbuff_egress,
         dma_read__dbuff_egress,
         dbuff_ingress__srpt_data,
         chunk_egress__dbuff_ingress,
         link_egress
         ); 

   hls_thread_local hls::task dbuff_ingress_task(
         dbuff_ingress,
         chunk_ingress__dbuff_ingress,
         dbuff_ingress__dma_write,
         rpc_state__dbuff_ingress
         );

   // TODO need to make sure these are totaly independent
   // Does not need to execute as 1 unit
   /* Control Driven Region */
   homa_recvmsg(homa_cfg, recvmsg, recvmsg__peer_map);
   homa_sendmsg(homa_cfg, sendmsg, homa_sendmsg__dbuff_stack);
   dma_read(maxi_in, homa_cfg, dbuff_stack__dma_read, dma_read__dbuff_egress);
   dma_write(maxi_out, homa_cfg, dbuff_ingress__dma_write);
}
