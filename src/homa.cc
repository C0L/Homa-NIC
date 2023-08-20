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
#include "packetmap.hh"
#include "dma.hh"

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
#ifdef STEPPED
void homa(bool dma_write_en, bool dma_read_en,
	  bool egress_en, bool ingress_en,
	  bool send_in_en, bool send_out_en,
	  bool recv_in_en, bool recv_out_en,
	  hls::stream<msghdr_send_t> & msghdr_send_i,
	  hls::stream<msghdr_send_t> & msghdr_send_o,
	  hls::stream<msghdr_recv_t> & msghdr_recv_i,
	  hls::stream<msghdr_recv_t> & msghdr_recv_o,
	  ap_uint<512> * maxi_in, ap_uint<512> * maxi_out,
	  hls::stream<raw_stream_t> & link_ingress,
	  hls::stream<raw_stream_t> & link_egress) {
#pragma HLS interface mode=ap_ctrl_hs port=return
#else
void homa(hls::stream<msghdr_send_t> & msghdr_send_i,
	  hls::stream<msghdr_send_t> & msghdr_send_o,
	  hls::stream<msghdr_recv_t> & msghdr_recv_i,
	  hls::stream<msghdr_recv_t> & msghdr_recv_o,
	  ap_uint<512> * maxi_in, ap_uint<512> * maxi_out,
	  hls::stream<raw_stream_t> & link_ingress,
	  hls::stream<raw_stream_t> & link_egress) {
#pragma HLS interface mode=ap_ctrl_none port=return
#endif

    /* https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/HLS-Stream-Library The
     * verification depth controls the size of the "RTL verification adapter" This
     * needs to be configured to be the maximum size of any stream encountered
     * through the execution of the testbench. This should not effect the actual
     * generation of the RTL however.
     *
     * Synth will claim that this is ignored, but cosim needs it to work.
     */

#ifdef STEPPED
#pragma HLS interface ap_hs port=dma_write_en  depth=256
#pragma HLS interface ap_hs port=dma_read_en   depth=256 
#pragma HLS interface ap_hs port=egress_en     depth=256
#pragma HLS interface ap_hs port=ingress_en    depth=256
#pragma HLS interface ap_hs port=send_in_en    depth=256
#pragma HLS interface ap_hs port=send_out_en   depth=256
#pragma HLS interface ap_hs port=recv_in_en    depth=256
#pragma HLS interface ap_hs port=recv_out_en   depth=256
#endif

#pragma HLS interface axis port=msghdr_send_i depth=256 
#pragma HLS interface axis port=msghdr_send_o depth=256
#pragma HLS interface axis port=msghdr_recv_i depth=256
#pragma HLS interface axis port=msghdr_recv_o depth=256
#pragma HLS interface axis port=link_ingress  depth=256
#pragma HLS interface axis port=link_egress   depth=256
#pragma HLS interface mode=m_axi offset=direct port=maxi_in bundle=MAXI latency=70 num_read_outstanding=1 depth=256 
#pragma HLS interface mode=m_axi offset=direct port=maxi_out bundle=MAXI latency=70 num_write_outstanding=1 depth=256


#pragma HLS dataflow disable_start_propagation 

    /* Naming scheme: {flow}__{source kernel}__{dest kernel} */

    /* header_out streams */
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_out__egress_sel__rpc_state  ("header_out__egress_sel__rpc_state"); 
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_out__rpc_state__pkt_builder ("header_out__rpc_state__pkt_builder"); 

    /* header_in streams */
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__rpc_map__rpc_state       ("header_in__rpc_map__rpc_state"); 
    hls_thread_local hls::stream<srpt_grant_notif_t, STREAM_DEPTH> header_in__rpc_state__srpt_data ("header_in__rpc_state__srpt_data");
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__chunk_ingress__rpc_map   ("header_in__chunk_ingress__rpc_map");
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__rpc_state__dbuff_ingress ("header_in__rpc_state__dbuff_ingress");
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__dbuff_ingress__packetmap ("header_in__dbuff_ingress__packetmap");
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__packetmap__homa_recvmsg ("header_in__packetmap__homa_recvmsg");
   
    /* sendmsg streams */
    hls_thread_local hls::stream<onboard_send_t, STREAM_DEPTH> sendmsg__homa_sendmsg__rpc_map("sendmsg__homa_sendmsg__msg_cache");
    hls_thread_local hls::stream<onboard_send_t, STREAM_DEPTH> sendmsg__rpc_map__rpc_state   ("sendmsg__rpc_map__rpc_state");
    hls_thread_local hls::stream<srpt_sendmsg_t, STREAM_DEPTH> sendmsg__rpc_state__srpt_data ("sendmsg__rpc_state__srpt_data");

    /* out chunk streams */
    hls_thread_local hls::stream<out_chunk_t, STREAM_DEPTH> out_chunk__chunk_egress__dbuff_egress ("out_chunk__chunk_egress__dbuff_egress");
    hls_thread_local hls::stream<out_chunk_t, STREAM_DEPTH> out_chunk__dbuff_egress__pkt_egress   ("out_chunk__dbuff_egress__pkt_egress");

    /* in chunk streams */
    hls_thread_local hls::stream<in_chunk_t, STREAM_DEPTH> in_chunk__chunk_ingress__dbuff_ingress ("in_chunk__chunk_ingress__dbuff_ingress");

    /* grant SRPT streams */
    hls_thread_local hls::stream<srpt_grant_in_t, STREAM_DEPTH> grant__rpc_state__srpt_grant   ("grant__rpc_state__srpt_grant");
    hls_thread_local hls::stream<srpt_grant_out_t, STREAM_DEPTH> grant__srpt_grant__egress_sel ("grant__srpt_grant__egress_sel");

    /* data SRPT streams */
    hls_thread_local hls::stream<srpt_pktq_t, STREAM_DEPTH> ready_data_pkt__srpt_data__egress_sel   ("ready_data_pkt__srpt_data__egress_sel");
    hls_thread_local hls::stream<srpt_dbuff_notif_t, STREAM_DEPTH> dbuff_notif__dbuff_egress__srpt_data ("dbuff_notif__dbuff_egress__srpt_data");

    /* dma streams */
    hls_thread_local hls::stream<dbuff_in_t, STREAM_DEPTH> dbuff_in__dma_read__msg_cache      ("dbuff_in__dma_read__msg_cache");
    hls_thread_local hls::stream<srpt_sendq_t, STREAM_DEPTH> dma_req__srpt_data__dma_read      ("dma_req__srpt_data__dma_read");
    hls_thread_local hls::stream<dma_w_req_t, STREAM_DEPTH> dma_req__dbuff_ingress__dma_write ("dma_req__dbuff_ingress__dma_write");

    /* Tasks must be declared after the processes or regions that
     * produce their input streams, and before the processes or regions
     * that consume their output streams.
     */

#ifdef STEPPED
    pkt_chunk_ingress(
	ingress_en,
	link_ingress,                           // link_ingress
	header_in__chunk_ingress__rpc_map,      // header_in_o 
	in_chunk__chunk_ingress__dbuff_ingress  // chunk_in_o
	);
#else
    hls_thread_local hls::task pkt_chunk_ingress_task(
	pkt_chunk_ingress,
	link_ingress,                           // link_ingress
	header_in__chunk_ingress__rpc_map,      // header_in_o 
	in_chunk__chunk_ingress__dbuff_ingress  // chunk_in_o
	);
#endif

#ifdef STEPPED
    homa_sendmsg(
	send_in_en,
	send_out_en,
	msghdr_send_i,                    // sendmsg_i
	msghdr_send_o,                    // sendmsg_o
	sendmsg__homa_sendmsg__rpc_map    // sendmsg_o
	);
#else
    hls_thread_local hls::task homa_sendmsg_task(
	homa_sendmsg, 
	msghdr_send_i,                     // sendmsg_i
	msghdr_send_o,                     // sendmsg_o
	sendmsg__homa_sendmsg__rpc_map    // sendmsg_o
	);
#endif

    hls_thread_local hls::task rpc_state_task(
	rpc_state,
	sendmsg__rpc_map__rpc_state,         // sendmsg_i
	sendmsg__rpc_state__srpt_data,       // sendmsg_o
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
	sendmsg__homa_sendmsg__rpc_map,    // sendmsg_i
	sendmsg__rpc_map__rpc_state        // sendmsg_o
	);
      
    hls_thread_local hls::task srpt_data_pkts_task(
	srpt_data_pkts,
	sendmsg__rpc_state__srpt_data,         // sendmsg_i
	dbuff_notif__dbuff_egress__srpt_data,  // dbuff_notif_i
	ready_data_pkt__srpt_data__egress_sel, // data_pkt_o  // TODO tidy this name up
	header_in__rpc_state__srpt_data,       // header_in_i
	dma_req__srpt_data__dma_read           // 
	);

#ifdef STEPPED
    dma_read(
	dma_read_en,
	maxi_in,
	dma_req__srpt_data__dma_read,
	dbuff_in__dma_read__msg_cache
	);
#else
    dma_read(
	maxi_in,
	dma_req__srpt_data__dma_read,
	dbuff_in__dma_read__msg_cache
	);
#endif

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

    hls_thread_local hls::task msg_cache_task(
	msg_cache,
	dbuff_in__dma_read__msg_cache,
	dbuff_notif__dbuff_egress__srpt_data,  // dbuff_notif_o
	out_chunk__chunk_egress__dbuff_egress, // out_chunk_i
	out_chunk__dbuff_egress__pkt_egress    // out_chunk_o
	); 

    hls_thread_local hls::task dbuff_ingress_task(
	dbuff_ingress,
	in_chunk__chunk_ingress__dbuff_ingress, // chunk_in_o
	dma_req__dbuff_ingress__dma_write,      // dma_w_req_o
	header_in__rpc_state__dbuff_ingress,    // header_in_i
	header_in__dbuff_ingress__packetmap     // header_in_i
	);

    hls_thread_local hls::task packetmap_task(
	packetmap, 
	header_in__dbuff_ingress__packetmap, // header_in
	header_in__packetmap__homa_recvmsg   // complete_messages
	);
#ifdef STEPPED
    pkt_chunk_egress(
	egress_en,
	out_chunk__dbuff_egress__pkt_egress, // out_chunk_i
	link_egress                          // link_egress
	);
#else
    hls_thread_local hls::task pkt_chunk_egress_task(
	pkt_chunk_egress,
	out_chunk__dbuff_egress__pkt_egress, // out_chunk_i
	link_egress                          // link_egress
	);
#endif

#ifdef STEPPED
    dma_write(
	dma_write_en,
	maxi_out,
	dma_req__dbuff_ingress__dma_write
	);
#else
    dma_write(
	maxi_out,
	dma_req__dbuff_ingress__dma_write
	);
#endif


#ifdef STEPPED
    homa_recvmsg(
	recv_in_en,
	recv_out_en,
	msghdr_recv_i,                  // recvmsg_i
	msghdr_recv_o,                  // recvmsg_o
	header_in__packetmap__homa_recvmsg 
	);
#else
    hls_thread_local hls::task homa_recvmsg_task(
	homa_recvmsg, 
	msghdr_recv_i,                  // recvmsg_i
	msghdr_recv_o,                  // recvmsg_o
	header_in__packetmap__homa_recvmsg 
	);
#endif


}
