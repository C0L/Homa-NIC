#include <iostream>

#include "ap_int.h"

#include "ap_axi_sdata.h"
#include "hls_task.h"
#include "hls_stream.h"

#include "homa.hh"
#include "user.hh"
#include "link.hh"
#include "peer.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"
#include "databuff.hh"

using namespace std;

void adapter2(hls::stream<dma_r_req_t> & in, 
      hls::stream<ap_uint<80>> & out) {

   dma_r_req_t i = in.read(); 

   ap_uint<80> msgout;
   msgout(DMA_R_REQ_OFFSET)   = i.offset;
   msgout(DMA_R_REQ_LENGTH)   = i.length;
   msgout(DMA_R_REQ_DBUFF_ID) = i.dbuff_id;

   out.write(msgout);
}

void adapter3(hls::stream<ap_uint<536>> & in,
      hls::stream<dbuff_in_t> & out) {

   ap_uint<536> i = in.read();
   dbuff_in_t msgout;

   msgout.block.data  = i(DBUFF_IN_DATA);
   msgout.dbuff_id    = i(DBUFF_IN_ID);
   msgout.dbuff_chunk = i(DBUFF_IN_CHUNK);

   out.write(msgout);
}

void adapter4(hls::stream<dma_w_req_t> & in,
      hls::stream<ap_uint<544>> & out) {

   dma_w_req_t i = in.read();
   ap_uint<544> msgout;
   msgout(DMA_W_REQ_OFFSET) = i.offset;
   msgout(DMA_W_REQ_BLOCK)  = i.block.data;
   out.write(msgout);
}

/**
 * homa() - Top level homa packet processor
 * @homa:
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @link_egress:  The outgoing AXI Stream of ethernet frames from to the link
 * @dma:   DMA memory space pointer
 */
extern "C"{
   void homa(hls::stream<ap_uint<512>, VERIF_DEPTH> & sendmsg_i,
         hls::stream<ap_uint<416>, VERIF_DEPTH> & recvmsg_i,
         hls::stream<ap_uint<80>, VERIF_DEPTH> & dma_r_req_o,
         hls::stream<ap_uint<536>, VERIF_DEPTH> & dma_r_resp_i,
         hls::stream<ap_uint<544>, VERIF_DEPTH> & dma_w_req_o,
         hls::stream<raw_stream_t> & link_ingress,
         hls::stream<raw_stream_t> & link_egress) {

      /* The depth field specifies the size of the verification adapter for cosimulation
       * Synth will claim that the depth field is ignored for some reason
       */
#pragma HLS interface axis port=sendmsg_i    depth=512
#pragma HLS interface axis port=recvmsg_i    depth=512
#pragma HLS interface axis port=dma_r_req_o  depth=512
#pragma HLS interface axis port=dma_r_resp_i depth=512
#pragma HLS interface axis port=dma_w_req_o  depth=512
#pragma HLS interface axis port=link_ingress depth=512
#pragma HLS interface axis port=link_egress  depth=512

      hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> homa_sendmsg__dbuff_stack       ("homa_sendmsg__dbuff_stack");
      hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> dbuff_stack__peer_map           ("dbuff_stack__peer_map");
      hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> peer_map__rpc_state             ("peer_map__rpc_state");
      hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> rpc_state__srpt_data            ("rpc_state__srpt_data");
      hls_thread_local hls::stream<ready_data_pkt_t> srpt_data__egress_sel                        ("srpt_data__egress_sel");
      hls_thread_local hls::stream<grant_out_t,      VERIF_DEPTH> srpt_grant__egress_sel          ("srpt_grant__egress_sel");
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> egress_sel__rpc_state           ("egress_sel__rpc_state"); 
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> rpc_state__pkt_builder          ("rpc_state__pkt_builder"); 
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> peer_map__rpc_map               ("peer_map__rpc_map"); 
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> rpc_map__rpc_state              ("rpc_map__rpc_state"); 
      hls_thread_local hls::stream<out_chunk_t,      VERIF_DEPTH> chunk_egress__dbuff_egress      ("chunk_egress__dbuff_egress");
      hls_thread_local hls::stream<out_chunk_t,      VERIF_DEPTH> dbuff_egress__pkt_egress        ("dbuff_egress__pkt_egress");
      hls_thread_local hls::stream<dbuff_notif_t,    VERIF_DEPTH> dbuff_egress__srpt_data         ("dbuff_egress__srpt_data");
      hls_thread_local hls::stream<in_chunk_t,       VERIF_DEPTH> chunk_ingress__dbuff_ingress    ("chunk_ingress__dbuff_ingress");
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> rpc_state__dbuff_ingress        ("rpc_state__dbuff_ingress");
      hls_thread_local hls::stream<grant_in_t,       VERIF_DEPTH> rpc_state__srpt_grant           ("rpc_state__srpt_grant");
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> header_in__rpc_state__srpt_data ("header_in__rpc_date__srpt_data");
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> chunk_ingress__rpc_map          ("chunk_ingress__rpc_map");
      hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__peer_map__rpc_state    ("recvmsg__peer_map__rpc_state");
      hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__rpc_state__rpc_map     ("recvmsg__rpc_state__rpc_map");
      hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__peer_map               ("recvmsg__peer_map");
      hls_thread_local hls::stream<dbuff_in_t,       VERIF_DEPTH> dma_r_resp_a;
      hls_thread_local hls::stream<dma_w_req_t,      VERIF_DEPTH> dma_w_req_a;
      hls_thread_local hls::stream<dma_r_req_t,      VERIF_DEPTH> dma_r_req_o_o;

      hls_thread_local hls::task adapter2_task(adapter2, dma_r_req_o_o, dma_r_req_o);
      hls_thread_local hls::task adapter3_task(adapter3, dma_r_resp_i, dma_r_resp_a);
      hls_thread_local hls::task adapter4_task(adapter4, dma_w_req_a, dma_w_req_o);

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
            rpc_state__pkt_builder,
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
            dbuff_egress__srpt_data,
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

      hls_thread_local hls::task pkt_builder_task(
            pkt_builder,
            rpc_state__pkt_builder,
            chunk_egress__dbuff_egress
            );

      hls_thread_local hls::task pkt_chunk_egress_task(
            pkt_chunk_egress,
            dbuff_egress__pkt_egress,
            link_egress
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
            dma_r_req_o_o
            ); 

      hls_thread_local hls::task dbuff_egress_task(
            dbuff_egress,
            dma_r_resp_a,
            dbuff_egress__srpt_data,
            chunk_egress__dbuff_egress,
            dbuff_egress__pkt_egress
            ); 

      hls_thread_local hls::task dbuff_ingress_task(
            dbuff_ingress,
            chunk_ingress__dbuff_ingress,
            dma_w_req_a,
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
   }
}
