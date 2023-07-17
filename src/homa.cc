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

      /* header_out streams */
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> header_out__egress_sel__rpc_state  (""); 
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> header_out__rpc_state__pkt_builder (""); 

      /* header_in streams */
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> header_in__rpc_map__rpc_state     (""); 
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> header_in__rpc_state__srpt_data   ("");
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> header_in__chunk_ingress__rpc_map ("");
      hls_thread_local hls::stream<header_t,         VERIF_DEPTH> header_in__rpc_state__dbuff_ingress    ("");

      /* recvmsg streams */
      hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__rpc_map__rpc_state ("");
      hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__homa_recvmsg__rpc_map ("");
      hls_thread_local hls::stream<recvmsg_t,        VERIF_DEPTH> recvmsg__peer_map            ("");

      /* sendmsg streams */
      hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> sendmsg__homa_sendmsg__dbuff_stack ("");
      hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> sendmsg__dbuff_stack__rpc_map     ("");
      hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> sendmsg__rpc_map__rpc_state       ("");
      hls_thread_local hls::stream<sendmsg_t,        VERIF_DEPTH> sendmsg__rpc_state__srpt_data      ("");

      /* dma streams */ 
      // TODO probably rename if remove adapter
      hls_thread_local hls::stream<dbuff_in_t,       VERIF_DEPTH> dma_r_resp_a  ("");
      hls_thread_local hls::stream<dma_w_req_t,      VERIF_DEPTH> dma_w_req_a   ("");
      hls_thread_local hls::stream<dma_r_req_t,      VERIF_DEPTH> dma_r_req_o_o ("");

      /* out chunk streams */
      hls_thread_local hls::stream<out_chunk_t,      VERIF_DEPTH> out_chunk__chunk_egress__dbuff_egress      ("");
      hls_thread_local hls::stream<out_chunk_t,      VERIF_DEPTH> out_chunk__dbuff_egress__pkt_egress        ("");

      /* in chunk streams */
      hls_thread_local hls::stream<in_chunk_t,       VERIF_DEPTH> in_chunk__chunk_ingress__dbuff_ingress    ("");

      /* grant SRPT streams */
      hls_thread_local hls::stream<grant_in_t,       VERIF_DEPTH> grant__rpc_state__srpt_grant           ("");
      hls_thread_local hls::stream<grant_out_t,      VERIF_DEPTH> grant__srpt_grant__egress_sel          ("");

      /* data SRPT streams */
      hls_thread_local hls::stream<ready_data_pkt_t, VERIF_DEPTH> ready_data_pkt__srpt_data__egress_sel           ("");
      hls_thread_local hls::stream<dbuff_notif_t,    VERIF_DEPTH> dbuff_notif__dbuff_egress__srpt_data         ("");


      
      hls_thread_local hls::task adapter2_task(adapter2, dma_r_req_o_o, dma_r_req_o);
      hls_thread_local hls::task adapter3_task(adapter3, dma_r_resp_i, dma_r_resp_a);
      hls_thread_local hls::task adapter4_task(adapter4, dma_w_req_a, dma_w_req_o);

      /* Naming scheme: {flow}__{source kernel}__{dest kernel} */

      hls_thread_local hls::task rpc_state_task(
            rpc_state,
            sendmsg__rpc_map__rpc_state,         // sendmsg_i
            sendmsg__rpc_state__srpt_data,       // sendmsg_o
            recvmsg__rpc_map__rpc_state,         // recvmsg_i
            header_out__egress_sel__rpc_state,   // header_out_i
            header_out__rpc_state__pkt_builder,  // header_out_o
            header_in__rpc_map__rpc_state,       // header_in_i
            header_in__rpc_state__dbuff_ingress, // header_in_dbuff_o
            grant__rpc_state__srpt_grant,        // grant_in_o
            header_in__rpc_state__srpt_data      // header_in_srpt_o
            );

      hls_thread_local hls::task rpc_map_task(
            rpc_map,
            header_in__chunk_ingress__rpc_map, // header_in_i
            header_in__rpc_map__rpc_state,     // header_in_o
            recvmsg__homa_recvmsg__rpc_map,    // recvmsg_i
            recvmsg__rpc_map__rpc_state,       // recvmsg_o
            sendmsg__dbuff_stack__rpc_map,     // sendmsg_i
            sendmsg__rpc_map__rpc_state        // sendmsg_o
            );
      
      hls_thread_local hls::task srpt_data_pkts_task(
            srpt_data_pkts,
            sendmsg__rpc_state__srpt_data,         // sendmsg_i
            dbuff_notif__dbuff_egress__srpt_data,  // dbuff_notif_i
            ready_data_pkt__srpt_data__egress_sel, // data_pkt_o  // TODO tidy this name up
            header_in__rpc_state__srpt_data        // header_in_i
            );

      hls_thread_local hls::task srpt_grant_pkts_task(
            srpt_grant_pkts,
            grant__rpc_state__srpt_grant, // grant_in_i
            grant__srpt_grant__egress_sel // grant_out_o
            );

      hls_thread_local hls::task egress_selector_task(
            egress_selector,
            ready_data_pkt__srpt_data__egress_sel,   // data_pkt_i
            grant__srpt_grant__egress_sel,           // grant_pkt_i
            header_out__egress_sel__rpc_state        // header_out_o
            );

      hls_thread_local hls::task pkt_builder_task(
            pkt_builder,
            header_out__rpc_state__pkt_builder,   // header_out_i
            out_chunk__chunk_egress__dbuff_egress // chunk_out_o
            );

      hls_thread_local hls::task pkt_chunk_egress_task(
            pkt_chunk_egress,
            out_chunk__dbuff_egress__pkt_egress, // out_chunk_i
            link_egress                          // link_egress
            );

      hls_thread_local hls::task pkt_chunk_ingress_task(
            pkt_chunk_ingress,
            link_ingress,                           // link_ingress
            header_in__chunk_ingress__rpc_map,      // header_in_o 
            in_chunk__chunk_ingress__dbuff_ingress  // chunk_in_o
            );

      hls_thread_local hls::task dbuff_stack_task(
            dbuff_stack,
            sendmsg__homa_sendmsg__dbuff_stack, // sendmsg_i
            sendmsg__dbuff_stack__rpc_map,      // sendmsg_o
            dma_r_req_o_o                       // dma_read_o
            ); 

      hls_thread_local hls::task dbuff_egress_task(
            dbuff_egress,
            dma_r_resp_a,                          // dbuff_egress_i
            dbuff_notif__dbuff_egress__srpt_data,  // dbuff_notif_o
            out_chunk__chunk_egress__dbuff_egress, // out_chunk_i
            out_chunk__dbuff_egress__pkt_egress    // out_chunk_o
            ); 

      hls_thread_local hls::task dbuff_ingress_task(
            dbuff_ingress,
            in_chunk__chunk_ingress__dbuff_ingress, // chunk_in_o
            dma_w_req_a,                            // dma_w_req_o
            header_in__rpc_state__dbuff_ingress     // header_in_i
            );

      hls_thread_local hls::task homa_recvmsg_task(
            homa_recvmsg, 
            recvmsg_i,                      // recvmsg_i
            recvmsg__homa_recvmsg__rpc_map  // recvmsg_o
            );

      hls_thread_local hls::task homa_sendmsg_task(
            homa_sendmsg, 
            sendmsg_i,                           // sendmsg_i
            sendmsg__homa_sendmsg__dbuff_stack   // sendmsg_o
            );
   }
}
