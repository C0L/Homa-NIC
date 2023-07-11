#include <iostream>

#include "ap_int.h"

#include "ap_axi_sdata.h"
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

//void test(char * maxi, sendmsg_t sendmsg, hls::stream<sendmsg_t> & dummy) {
//   if (sendmsg.valid) { 
//      sendmsg_t d = dummy.read();
//      *maxi = d.rtt_bytes;
//   }
//}
// void adapter(hls::stream<hls::axis<sendmsg_t,0,0,0>> & sendmsg_i,
//       	hls::stream<hls::axis<recvmsg_t,0,0,0>> & recvmsg_i,
//       	hls::stream<hls::axis<dma_r_req_t,0,0,0>> & dma_r_req_o,
//       	hls::stream<hls::axis<dbuff_in_t,0,0,0>> & dma_r_resp_i,
//       	hls::stream<hls::axis<dma_w_req_t,0,0,0>> & dma_w_req_o) {
// 	sendmsg_i
// }

/**
 * homa() - Top level homa packet processor
 * @homa:
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @link_egress:  The outgoing AXI Stream of ethernet frames from to the link
 * @dma:   DMA memory space pointer
 */
extern "C"{
void homa(hls::stream<hls::axis<sendmsg_t,0,0,0>> & sendmsg_i,
      hls::stream<hls::axis<recvmsg_t,0,0,0>> & recvmsg_i,
      hls::stream<hls::axis<dma_r_req_t,0,0,0>> & dma_r_req_o,
      hls::stream<hls::axis<dbuff_in_t,0,0,0>> & dma_r_resp_i,
      hls::stream<hls::axis<dma_w_req_t,0,0,0>> & dma_w_req_o,
      hls::stream<raw_stream_t> & link_ingress,
      hls::stream<raw_stream_t> & link_egress) {

// #pragma HLS s_axilite mode=ap_ctrl_chain port=return 
//#pragma HLS interface ap_fifo port=sendmsg_i depth=512
//#pragma HLS interface ap_fifo port=recvmsg_i depth=512
//#pragma HLS interface ap_fifo port=dma_r_req_o depth=512
//#pragma HLS interface ap_fifo port=dma_r_resp_i depth=512
//#pragma HLS interface ap_fifo port=dma_w_req_o depth=512
//
// TODO try creating an adapter so that only non-blocking operations occur on top level ports?

// TODO return to 2022.1 and make sure this is specified on the other side- the client?
// #pragma HLS aggregate variable=sendmsg_i compact=bit
// #pragma HLS aggregate variable=recvmsg_i compact=bit
// #pragma HLS aggregate variable=dma_r_req_o compact=bit
// #pragma HLS aggregate variable=dma_r_resp_i compact=bit
// #pragma HLS aggregate variable=dma_w_req_o compact=bit
// #pragma HLS aggregate variable=link_ingress compact=bit
// #pragma HLS aggregate variable=link_egress compact=bit

//#pragma HLS interface axis port=sendmsg_i both register
//#pragma HLS interface axis port=recvmsg_i both register
//#pragma HLS interface axis port=dma_r_req_o both register
//#pragma HLS interface axis port=dma_r_resp_i both register
//#pragma HLS interface axis port=dma_w_req_o both register
//#pragma HLS interface axis port=link_ingress both register
//#pragma HLS interface axis port=link_egress both register

//#pragma HLS interface axis port=sendmsg_i
//#pragma HLS interface axis port=recvmsg_i
//#pragma HLS interface axis port=dma_r_req_o
//#pragma HLS interface axis port=dma_r_resp_i
//#pragma HLS interface axis port=dma_w_req_o
//#pragma HLS interface axis port=link_ingress
//#pragma HLS interface axis port=link_egress



// #pragma HLS interface axis port=sendmsg_i depth=512
// #pragma HLS interface axis port=recvmsg_i depth=512
// #pragma HLS interface axis port=dma_r_req_o depth=512
// #pragma HLS interface axis port=dma_r_resp_i depth=512
// #pragma HLS interface axis port=dma_w_req_o depth=512
// #pragma HLS interface axis port=link_ingress depth=512
// #pragma HLS interface axis port=link_egress depth=512


//#pragma HLS interface axis port=sendmsg_i register_mode=reverse register depth=512
//#pragma HLS interface axis port=recvmsg_i register_mode=reverse register depth=512
//#pragma HLS interface axis port=dma_r_req_o register_mode=forward register depth=512
//#pragma HLS interface axis port=dma_r_resp_i register_mode=reverse register depth=512
//#pragma HLS interface axis port=dma_w_req_o register_mode=forward register depth=512
//#pragma HLS interface axis port=link_ingress register_mode=reverse register depth=512
//#pragma HLS interface axis port=link_egress register_mode=forward register depth=512


// #pragma HLS interface axis port=link_ingress depth=512
// #pragma HLS interface axis port=link_egress  depth=512

   hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> homa_sendmsg__dbuff_stack       ("homa_sendmsg__dbuff_stack");
   hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> dbuff_stack__peer_map           ("dbuff_stack__peer_map");
   hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> peer_map__rpc_state             ("peer_map__rpc_state");
   hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> rpc_state__srpt_data            ("rpc_state__srpt_data");
   hls_thread_local hls::stream<ready_data_pkt_t, VERIF_DEPTH> srpt_data__egress_sel           ("srpt_data__egress_sel");
   hls_thread_local hls::stream<ap_uint<51>,      VERIF_DEPTH> srpt_grant__egress_sel          ("srpt_grant__egress_sel");
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
         recvmsg_i, 
         recvmsg__peer_map
         );

   hls_thread_local hls::task homa_sendmsg_task(
         homa_sendmsg, 
         sendmsg_i,
         homa_sendmsg__dbuff_stack);
         // dummy);
}
}
