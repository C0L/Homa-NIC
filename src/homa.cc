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

/**
 * homa() - Top level homa packet processor
 * @sendmsg_i    - Incoming requests to send a message (either request or response)
 * @recvmsg_i    - Incoming requests to accept a new message from a specified peer
 * @dma_r_req_o  - Outgoing DMA read requests for more data
 * @dma_r_resp_i - Incoming DMA read responses with 512 bit chunks of data
 * @dma_w_req_o  - Outgoing DMA write requests to place 512 but chunks of data in DMA
 * @link_ingress: The incoming AXI Stream of ethernet frames from the link
 * @link_egress:  The outgoing AXI Stream of ethernet frames from to the link
 */
extern "C"{
   void homa(hls::stream<sendmsg_raw_t> & sendmsg_i,
         hls::stream<recvmsg_raw_t>     & recvmsg_i,
         hls::stream<dma_r_req_raw_t>   & dma_r_req_o,
         hls::stream<dbuff_in_raw_t>    & dma_r_resp_i,
         hls::stream<dma_w_req_raw_t>   & dma_w_req_o,
         hls::stream<raw_stream_t>      & link_ingress,
         hls::stream<raw_stream_t>      & link_egress) {

      /* https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/HLS-Stream-Library The
       * verification depth controls the size of the "RTL verification adapter" This
       * needs to be configured to be the maximum size of any stream encountered
       * through the execution of the testbench. This should not effect the actual
       * generation of the RTL however.
       *
       * Synth will claim that this is ignored, but cosim needs it to work.
       */
#pragma HLS interface axis port=sendmsg_i   register_mode=off register depth=512
#pragma HLS interface axis port=recvmsg_i   register_mode=off register depth=512
#pragma HLS interface axis port=dma_r_req_o register_mode=off register depth=512
#pragma HLS interface axis port=dma_r_resp_i register_mode=off register depth=512
#pragma HLS interface axis port=dma_w_req_o register_mode=off register depth=512
#pragma HLS interface axis port=link_ingress register_mode=off register depth=512
#pragma HLS interface axis port=link_egress register_mode=off register depth=512

// Seems to work but sizes not being decred
// #pragma HLS interface axis port=sendmsg_i    depth=512
// #pragma HLS interface axis port=recvmsg_i    depth=512
// #pragma HLS interface axis port=dma_r_req_o  depth=512
// #pragma HLS interface axis port=dma_r_resp_i depth=512
// #pragma HLS interface axis port=dma_w_req_o  depth=512
// #pragma HLS interface axis port=link_ingress depth=512
// #pragma HLS interface axis port=link_egress  depth=512


      /* Naming scheme: {flow}__{source kernel}__{dest kernel} */

      /* header_out streams */
      hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_out__egress_sel__rpc_state  ("header_out__egress_sel__rpc_state"); 
      hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_out__rpc_state__pkt_builder ("header_out__rpc_state__pkt_builder"); 

      /* header_in streams */
      hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__rpc_map__rpc_state       ("header_in__rpc_map__rpc_state"); 
      hls_thread_local hls::stream<srpt_grant_notif_t, STREAM_DEPTH> header_in__rpc_state__srpt_data     ("header_in__rpc_state__srpt_data");
      hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__chunk_ingress__rpc_map   ("header_in__chunk_ingress__rpc_map");
      hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__rpc_state__dbuff_ingress ("header_in__rpc_state__dbuff_ingress");

      /* recvmsg streams */
      hls_thread_local hls::stream<recvmsg_t, STREAM_DEPTH> recvmsg__rpc_map__rpc_state    ("recvmsg__rpc_map__rpc_state");
      hls_thread_local hls::stream<recvmsg_t, STREAM_DEPTH> recvmsg__homa_recvmsg__rpc_map ("recvmsg__homa_recvmsg__rpc_map");
      hls_thread_local hls::stream<recvmsg_t, STREAM_DEPTH> recvmsg__peer_map              ("recvmsg__peer_map");

      /* sendmsg streams */
      hls_thread_local hls::stream<sendmsg_t, STREAM_DEPTH> sendmsg__homa_sendmsg__dbuff_stack ("sendmsg__homa_sendmsg__dbuff_stack");
      hls_thread_local hls::stream<sendmsg_t, STREAM_DEPTH> sendmsg__dbuff_stack__rpc_map      ("sendmsg__dbuff_stack__rpc_map");
      hls_thread_local hls::stream<sendmsg_t, STREAM_DEPTH> sendmsg__rpc_map__rpc_state        ("sendmsg__rpc_map__rpc_state");
      hls_thread_local hls::stream<srpt_data_in_t, STREAM_DEPTH> sendmsg__rpc_state__srpt_data ("sendmsg__rpc_state__srpt_data");

      /* out chunk streams */
      hls_thread_local hls::stream<out_chunk_t, STREAM_DEPTH> out_chunk__chunk_egress__dbuff_egress ("out_chunk__chunk_egress__dbuff_egress");
      hls_thread_local hls::stream<out_chunk_t, STREAM_DEPTH> out_chunk__dbuff_egress__pkt_egress   ("out_chunk__dbuff_egress__pkt_egress");

      /* in chunk streams */
      hls_thread_local hls::stream<in_chunk_t, STREAM_DEPTH> in_chunk__chunk_ingress__dbuff_ingress ("in_chunk__chunk_ingress__dbuff_ingress");

      /* grant SRPT streams */
      hls_thread_local hls::stream<srpt_grant_in_t, STREAM_DEPTH> grant__rpc_state__srpt_grant   ("grant__rpc_state__srpt_grant");
      hls_thread_local hls::stream<srpt_grant_out_t, STREAM_DEPTH> grant__srpt_grant__egress_sel ("grant__srpt_grant__egress_sel");

      /* data SRPT streams */
      hls_thread_local hls::stream<srpt_data_out_t, STREAM_DEPTH> ready_data_pkt__srpt_data__egress_sel   ("ready_data_pkt__srpt_data__egress_sel");
      hls_thread_local hls::stream<srpt_dbuff_notif_t, STREAM_DEPTH> dbuff_notif__dbuff_egress__srpt_data ("dbuff_notif__dbuff_egress__srpt_data");

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

      hls_thread_local hls::task dbuff_egress_task(
            dbuff_egress,
            sendmsg__homa_sendmsg__dbuff_stack,
            sendmsg__dbuff_stack__rpc_map,
            dma_r_resp_i,                          // dbuff_egress_i
            dbuff_notif__dbuff_egress__srpt_data,  // dbuff_notif_o
            out_chunk__chunk_egress__dbuff_egress, // out_chunk_i
            out_chunk__dbuff_egress__pkt_egress,   // out_chunk_o
            dma_r_req_o                         
            ); 

      hls_thread_local hls::task dbuff_ingress_task(
            dbuff_ingress,
            in_chunk__chunk_ingress__dbuff_ingress, // chunk_in_o
            dma_w_req_o,                            // dma_w_req_o
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
