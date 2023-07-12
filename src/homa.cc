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
void adapter(hls::stream<sendmsg_t> & sendmsg_i,
      hls::stream<recvmsg_t> & recvmsg_i,
      hls::stream<dma_r_req_t> & dma_r_req_o,
      hls::stream<dbuff_in_t> & dma_r_resp_i,
      hls::stream<dma_w_req_t> & dma_w_req_o,
      hls::stream<sendmsg_t> & sendmsg_i_o,
      hls::stream<recvmsg_t> & recvmsg_i_o,
      hls::stream<dma_r_req_t> & dma_r_req_o_o,
      hls::stream<dbuff_in_t> & dma_r_resp_i_o,
      hls::stream<dma_w_req_t> & dma_w_req_o_o) {

   sendmsg_t sendmsg_convert;
   if (sendmsg_i.read_nb(sendmsg_convert)) {
      std::cerr << "SENDMSG\n";
      sendmsg_i_o.write(sendmsg_convert);
   }

   recvmsg_t recvmsg_convert;
   if (recvmsg_i.read_nb(recvmsg_convert)){ 
      std::cerr << "RECVMSG\n";
      recvmsg_i_o.write(recvmsg_convert);
   }

   dbuff_in_t dma_r_resp_convert;
   if (dma_r_resp_i.read_nb(dma_r_resp_convert)){ 
      std::cerr << "dbuff in\n";
      dma_r_resp_i_o.write(dma_r_resp_convert);
   }

   dma_w_req_t dma_w_req_convert;
   if (dma_w_req_o_o.read_nb(dma_w_req_convert)){ 
      std::cerr << "dma_w_req\n";
      dma_w_req_o.write(dma_w_req_convert);
   }

   dma_r_req_t dma_r_req_convert;
   if (dma_r_req_o_o.read_nb(dma_r_req_convert)){ 
      std::cerr << "dma r req\n";
      dma_r_req_o.write(dma_r_req_convert);
   }
}

/**
 * homa() - Top level homa packet processor
 * @homa:
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @link_egress:  The outgoing AXI Stream of ethernet frames from to the link
 * @dma:   DMA memory space pointer
 */
extern "C"{
   void homa(hls::stream<sendmsg_t> & sendmsg_i,
         hls::stream<recvmsg_t> & recvmsg_i,
         hls::stream<dma_r_req_t> & dma_r_req_o,
         hls::stream<dbuff_in_t> & dma_r_resp_i,
         hls::stream<dma_w_req_t> & dma_w_req_o,
         hls::stream<raw_stream_t> & link_ingress,
         hls::stream<raw_stream_t> & link_egress) {

#pragma HLS interface axis port=sendmsg_i depth=512
#pragma HLS interface axis port=recvmsg_i depth=512
#pragma HLS interface axis port=dma_r_req_o depth=512
#pragma HLS interface axis port=dma_r_resp_i depth=512
#pragma HLS interface axis port=dma_w_req_o depth=512
#pragma HLS interface axis port=link_ingress depth=512
#pragma HLS interface axis port=link_egress depth=512

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

      hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> sendmsg_a;
      hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg_a;
      hls_thread_local hls::stream<dma_r_req_t,        VERIF_DEPTH> dma_r_req_a;
      hls_thread_local hls::stream<dbuff_in_t,        VERIF_DEPTH> dma_r_resp_a;
      hls_thread_local hls::stream<dma_w_req_t,        VERIF_DEPTH> dma_w_req_a;

      hls_thread_local hls::task adapter_task(
            adapter,
            sendmsg_i,
            recvmsg_i,
            dma_r_req_o,
            dma_r_resp_i,
            dma_w_req_o,
            sendmsg_a,
            recvmsg_a,
            dma_r_req_a,
            dma_r_resp_a,
            dma_w_req_a
            );




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
            dma_r_req_a 
            ); 

      hls_thread_local hls::task dbuff_egress_task(
            dbuff_egress,
            dma_r_resp_a,
            dbuff_ingress__srpt_data,
            chunk_egress__dbuff_ingress,
            link_egress
            ); 

      hls_thread_local hls::task dbuff_ingress_task(
            dbuff_ingress,
            chunk_ingress__dbuff_ingress,
            dma_w_req_a,
            rpc_state__dbuff_ingress
            );

      hls_thread_local hls::task homa_recvmsg_task(
            homa_recvmsg, 
            recvmsg_a, 
            recvmsg__peer_map
            );

      hls_thread_local hls::task homa_sendmsg_task(
            homa_sendmsg, 
            sendmsg_a,
            homa_sendmsg__dbuff_stack);
      // dummy);
   }
}
