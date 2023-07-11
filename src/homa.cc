#include <iostream>

#include "hls_task.h"
#include "hls_stream.h"

#include "homa.hh"
#include "dma.hh"
#include "link.hh"
#include "peer.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"
#include "databuff.hh"

using namespace std;

void test(char * maxi, hls::stream<sendmsg_t> & dummy) {
   sendmsg_t d = dummy.read();
   *maxi = d.rtt_bytes;
}

/**
 * homa() - Top level homa packet processor
 * @homa:
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @link_egress:  The outgoing AXI Stream of ethernet frames from to the link
 * @dma:   DMA memory space pointer
 */
// TODO ap_fifo did not appear to be the issue
void homa(char * maxi,
      hls::stream<hls::axis<sendmsg_t, 0, 0, 0>> & sendmsg_i_a,
      hls::stream<hls::axis<recvmsg_t, 0, 0, 0>> & recvmsg_i_a,
      hls::stream<hls::axis<dma_r_req_t, 0, 0, 0>> & dma_r_req_o,
      hls::stream<hls::axis<dbuff_in_t, 0, 0, 0>> & dma_r_resp_i,
      hls::stream<hls::axis<dma_w_req_t, 0, 0, 0>> & dma_w_req_o,
      hls::stream<raw_stream_t> & link_ingress,
      hls::stream<raw_stream_t> & link_egress) {

// Did not seem to make a difference
// #pragma HLS interface mode=ap_ctrl_chain port=return

// #pragma HLS interface ap_fifo port=sendmsg_i depth=512
// #pragma HLS interface ap_fifo port=recvmsg_i depth=512
// #pragma HLS interface ap_fifo port=dma_r_req_o depth=512
// #pragma HLS interface ap_fifo port=dma_r_resp_i depth=512
// #pragma HLS interface ap_fifo port=dma_w_req_o depth=512

// #pragma HLS stream variable=sendmsg_i type=fifo
// #pragma HLS stream variable=recvmsg_i type=fifo
// #pragma HLS stream variable=dma_r_req_o type=fifo
// #pragma HLS stream variable=dma_r_resp_i type=fifo
// #pragma HLS stream variable=dma_w_req_o type=fifo
//
#pragma HLS interface mode=m_axi port=maxi bundle=maxi_0 latency=70 num_read_outstanding=256  num_write_outstanding=1 max_write_burst_length=2 max_read_burst_length=1 depth=16000

#pragma HLS interface axis port=sendmsg_i_a depth=512
#pragma HLS interface axis port=recvmsg_i_a depth=512
#pragma HLS interface axis port=dma_r_req_o depth=512
#pragma HLS interface axis port=dma_r_resp_i depth=512
#pragma HLS interface axis port=dma_w_req_o depth=512
#pragma HLS interface axis port=link_ingress depth=512
#pragma HLS interface axis port=link_egress  depth=512

#pragma HLS dataflow

   hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> dummy ("dummy");



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
   hls_thread_local hls::stream<in_chunk_t,       VERIF_DEPTH> chunk_ingress__dbuff_ingress    ("chunk_ingress__dbuff_ingress");
   hls_thread_local hls::stream<header_t,         VERIF_DEPTH> rpc_state__dbuff_ingress        ("rpc_state__dbuff_ingress");
   hls_thread_local hls::stream<ap_uint<58>,      VERIF_DEPTH> rpc_state__srpt_grant           ("rpc_state__srpt_grant");
   hls_thread_local hls::stream<header_t,         VERIF_DEPTH> header_in__rpc_state__srpt_data ("header_in__rpc_date__srpt_data");
   hls_thread_local hls::stream<header_t,         VERIF_DEPTH> chunk_ingress__rpc_map          ("chunk_ingress__rpc_map");
   hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__peer_map__rpc_state    ("recvmsg__peer_map__rpc_state");
   hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__rpc_state__rpc_map     ("recvmsg__rpc_state__rpc_map");
   hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__peer_map               ("recvmsg__peer_map");


   


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
         dma_r_req_o 
         ); 

   hls_thread_local hls::task dbuff_egress_task(
         dbuff_egress,
         dma_r_resp_i,
         dbuff_ingress__srpt_data,
         chunk_egress__dbuff_ingress,
         link_egress
         ); 

   hls_thread_local hls::task dbuff_ingress_task(
         dbuff_ingress,
         chunk_ingress__dbuff_ingress,
         dma_w_req_o,
         rpc_state__dbuff_ingress
         );

   hls_thread_local hls::task homa_recvmsg_task(
         homa_recvmsg, 
         recvmsg_i_a, 
         recvmsg__peer_map
         );

   hls_thread_local hls::task homa_sendmsg_task(
         homa_sendmsg, 
         sendmsg_i_a,
         homa_sendmsg__dbuff_stack,
         dummy
         );

   test(maxi, dummy);
}

