#include <iostream>
#include <fstream>

#include "ap_int.h"

#include "ap_axi_sdata.h"
#include "hls_task.h"
#include "hls_stream.h"

#include "homa.hh"
#include "user.hh"
#include "link.hh"
#include "rpcmgmt.hh"
#include "srptmgmt.hh"
#include "databuff.hh"
#include "packetmap.hh"
#include "dma.hh"
#include "logger.hh"

using namespace hls;

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
void homa(
    hls::stream<msghdr_send_t> & msghdr_send_i,
    //hls::stream<msghdr_recv_t> & msghdr_recv_i,
    //hls::stream<msghdr_recv_t> & msghdr_recv_o,
    hls::stream<am_cmd_t> & w_cmd_queue_o,
    hls::stream<ap_axiu<512,0,0,0>> & w_data_queue_o,
    hls::stream<am_status_t> & w_status_queue_i,
    hls::stream<am_cmd_t> & r_cmd_queue_o,
    hls::stream<ap_axiu<512,0,0,0>> & r_data_queue_i,
    hls::stream<am_status_t> & r_status_queue_i,
    hls::stream<raw_stream_t> & link_ingress_i,
    hls::stream<raw_stream_t> & link_egress_o,
    hls::stream<port_to_phys_t> & h2c_port_to_msgbuff_i,
    hls::stream<port_to_phys_t> & c2h_port_to_msgbuff_i,
    hls::stream<port_to_phys_t> & c2h_port_to_metasend_i,
    hls::stream<port_to_phys_t> & c2h_port_to_metarecv_i,
    hls::stream<ap_uint<8>> & log_control_i,
    hls::stream<log_entry_t> & log_out_o
    ) {

#pragma HLS interface axis port=msghdr_send_i
  // #pragma HLS interface axis port=msghdr_recv_i
#pragma HLS interface axis port=w_status_queue_i
#pragma HLS interface axis port=r_data_queue_i
#pragma HLS interface axis port=r_status_queue_i
#pragma HLS interface axis port=link_ingress_i
  // #pragma HLS interface axis port=msghdr_recv_o
#pragma HLS interface axis port=w_cmd_queue_o
#pragma HLS interface axis port=w_data_queue_o
#pragma HLS interface axis port=r_cmd_queue_o 
#pragma HLS interface axis port=link_egress_o
#pragma HLS interface axis port=h2c_port_to_msgbuff_i
#pragma HLS interface axis port=c2h_port_to_msgbuff_i
#pragma HLS interface axis port=c2h_port_to_metasend_i
#pragma HLS interface axis port=c2h_port_to_metarecv_i
#pragma HLS interface axis port=log_control_i
#pragma HLS interface axis port=log_out_o

#pragma HLS interface mode=ap_ctrl_none port=return

    /* https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/HLS-Stream-Library The
     * verification depth controls the size of the "RTL verification adapter" This
     * needs to be configured to be the maximum size of any stream encountered
     * through the execution of the testbench. This should not effect the actual
     * generation of the RTL however.
     *
     * Synth will claim that this is ignored, but cosim needs it to work.
     */
#pragma HLS dataflow disable_start_propagation 

    /* Naming scheme: {flow}__{source kernel}__{dest kernel} */

    hls_thread_local hls::stream<dbuff_id_t, STREAM_DEPTH> free_dbuff__h2c_databuff__rpc_state;

    /* header_out streams */
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_out__egress_sel__rpc_state;
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_out__rpc_state__pkt_builder;

    /* header_in streams */
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__chunk_ingress__rpc_state;
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__rpc_state__dbuff_ingress;
    // hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__packetmap__dbuff_ingress;
    // hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__address_map__dbuff_ingress;

    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__dbuff_ingress__homa_recvmsg;
   
    hls_thread_local hls::stream<dma_w_req_t, STREAM_DEPTH> sendmsg__rpc_state__dma_write;
    hls_thread_local hls::stream<dma_w_req_t, STREAM_DEPTH> recvmsg__rpc_state__dma_write;

    /* out chunk streams */
    hls_thread_local hls::stream<h2c_chunk_t, STREAM_DEPTH> out_chunk__chunk_egress__dbuff_egress;
    hls_thread_local hls::stream<h2c_chunk_t, STREAM_DEPTH> out_chunk__dbuff_egress__pkt_egress;

    /* in chunk streams */
    /* The depth of this stream is configured based on how many cycles
     * it takes from a new header to be processed and returned to the msg
     * spool
     */
    hls_thread_local hls::stream<c2h_chunk_t, 8> in_chunk__chunk_ingress__dbuff_ingress;

    /* grant SRPT streams */
    hls_thread_local hls::stream<srpt_grant_send_t, STREAM_DEPTH> grant__srpt_grant__egress_sel;

    /* h2c databuffer streams */
    hls_thread_local hls::stream<srpt_queue_entry_t, STREAM_DEPTH> ready_data_pkt__srpt_data__egress_sel;
    hls_thread_local hls::stream<srpt_queue_entry_t, STREAM_DEPTH> dbuff_notif__h2c_databuff__rpc_state;
    hls_thread_local hls::stream<srpt_queue_entry_t, STREAM_DEPTH> dbuff_notif__rpc_state__srpt_data;

    /* dma streams */
    hls_thread_local hls::stream<srpt_queue_entry_t, STREAM_DEPTH> dma_req__srpt_data__address_map;
    hls_thread_local hls::stream<dma_r_req_t, STREAM_DEPTH> dma_req__address_map__dma_read;
    hls_thread_local hls::stream<h2c_dbuff_t, STREAM_DEPTH> dbuff_in__dma_read__msg_cache;
    hls_thread_local hls::stream<srpt_queue_entry_t, STREAM_DEPTH> dma_req__srpt_data__dma_read;
    hls_thread_local hls::stream<dma_w_req_t, STREAM_DEPTH> dma_req__dbuff_ingress__dma_write;


    hls_thread_local hls::stream<dma_w_req_t, STREAM_DEPTH> dma_req__dbuff_ingress__address_map;
    hls_thread_local hls::stream<dma_w_req_t, STREAM_DEPTH> dma_req__address_map__dma_write;
    hls_thread_local hls::stream<srpt_queue_entry_t, STREAM_DEPTH> sendmsg__rpc_state__srpt_data;
    hls_thread_local hls::stream<srpt_queue_entry_t, STREAM_DEPTH> sendmsg__rpc_state__srpt_fetch;

    // hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__rpc_state__packetmap;
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__packetmap__rpc_state;
    hls_thread_local hls::stream<srpt_grant_new_t, STREAM_DEPTH> grant__rpc_state__srpt_grant;
    hls_thread_local hls::stream<srpt_queue_entry_t, STREAM_DEPTH> header_in__rpc_state__srpt_data;
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__rpc_state__packetmap;

    /* log streams */
    hls_thread_local hls::stream<ap_uint<8>, STREAM_DEPTH> dma_w_req_log;
    hls_thread_local hls::stream<ap_uint<8>, STREAM_DEPTH> dma_w_stat_log;
    hls_thread_local hls::stream<ap_uint<8>, STREAM_DEPTH> dma_r_req_log;
    hls_thread_local hls::stream<ap_uint<8>, STREAM_DEPTH> dma_r_read_log;
    hls_thread_local hls::stream<ap_uint<8>, STREAM_DEPTH> dma_r_stat_log;
    hls_thread_local hls::stream<ap_uint<8>, STREAM_DEPTH> h2c_pkt_log;
    hls_thread_local hls::stream<ap_uint<8>, STREAM_DEPTH> c2h_pkt_log;
    hls_thread_local hls::stream<ap_uint<8>, STREAM_DEPTH> dbuff_notif_log;

    // TODO need to selectively route to these in both the sendmsg core and the link ingress core
 
    hls_thread_local hls::task rpc_state_task(
	rpc_state,
	msghdr_send_i,
	sendmsg__rpc_state__dma_write,
	recvmsg__rpc_state__dma_write,
	sendmsg__rpc_state__srpt_data,
	sendmsg__rpc_state__srpt_fetch,
	grant__rpc_state__srpt_grant,
	header_out__egress_sel__rpc_state, 
	header_out__rpc_state__pkt_builder,
	header_in__chunk_ingress__rpc_state,
	header_in__rpc_state__packetmap,
	header_in__packetmap__rpc_state,
	header_in__rpc_state__dbuff_ingress,
	header_in__dbuff_ingress__homa_recvmsg,
	header_in__rpc_state__srpt_data,
	dbuff_notif__h2c_databuff__rpc_state,
	dbuff_notif__rpc_state__srpt_data,
	c2h_port_to_msgbuff_i,
	dma_req__dbuff_ingress__address_map,
	dma_req__address_map__dma_write,
	h2c_port_to_msgbuff_i,
	dma_req__srpt_data__address_map, // TODO name change
	dma_req__address_map__dma_read,  // TODO name change
	c2h_port_to_metasend_i,
	c2h_port_to_metarecv_i,
	free_dbuff__h2c_databuff__rpc_state,
	h2c_pkt_log,
	c2h_pkt_log
	);

    hls_thread_local hls::task srpt_grant_pkts_task(
	srpt_grant_pkts,
	grant__rpc_state__srpt_grant, // grant_in_i
	grant__srpt_grant__egress_sel // grant_out_o
	);

    hls_thread_local hls::task srpt_data_queue_task(
	srpt_data_queue,
	sendmsg__rpc_state__srpt_data,         // sendmsg_i
	dbuff_notif__rpc_state__srpt_data,     // dbuff_notif_i
	ready_data_pkt__srpt_data__egress_sel, // data_pkt_o  // TODO tidy this name up
	header_in__rpc_state__srpt_data        // header_in_i
	);

    hls_thread_local hls::task srpt_fetch_queue_task(
	srpt_fetch_queue,
	sendmsg__rpc_state__srpt_fetch, // sendmsg_i
	dma_req__srpt_data__address_map // 
	);

    hls_thread_local hls::task packetmap_task(
	packetmap, 
	header_in__rpc_state__packetmap,    
	header_in__packetmap__rpc_state
	);

    hls_thread_local hls::task dma_read_task(
	dma_read,
	r_cmd_queue_o,
	r_data_queue_i,
	r_status_queue_i,
	dma_req__address_map__dma_read,
	dbuff_in__dma_read__msg_cache,
	dma_r_req_log,
	dma_r_read_log,
	dma_r_stat_log
        );

    hls_thread_local hls::task dma_write_task(
	dma_write,
	w_cmd_queue_o,
	w_data_queue_o,
	w_status_queue_i,
	dma_req__address_map__dma_write,
	sendmsg__rpc_state__dma_write,
	recvmsg__rpc_state__dma_write,
	dma_w_req_log,
	dma_w_stat_log
	);
 
    hls_thread_local hls::task h2c_databuff_task(
	h2c_databuff,
	dbuff_in__dma_read__msg_cache,
	dbuff_notif__h2c_databuff__rpc_state,  // dbuff_notif_o
	out_chunk__chunk_egress__dbuff_egress, // out_chunk_i
	out_chunk__dbuff_egress__pkt_egress,   // out_chunk_o
	free_dbuff__h2c_databuff__rpc_state,
	dbuff_notif_log
	);

    // rpc_state -> packetmap -> databuff -> rpc_state
    hls_thread_local hls::task c2h_databuff_task(
	c2h_databuff,
	in_chunk__chunk_ingress__dbuff_ingress, // chunk_in_i
	dma_req__dbuff_ingress__address_map,    // dma_w_req_o
	header_in__rpc_state__dbuff_ingress,    // header_in_i
	header_in__dbuff_ingress__homa_recvmsg  // header_in_o
	);

    hls_thread_local hls::task pkt_chunk_ingress_task(
	pkt_chunk_ingress,
	link_ingress_i,
	header_in__chunk_ingress__rpc_state,
	in_chunk__chunk_ingress__dbuff_ingress
	);

    hls_thread_local hls::task next_pkt_selector_task(
	next_pkt_selector,
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
	link_egress_o                        // link_egress
	);

    hls_thread_local hls::task homa_logger(
    	logger,
	dma_w_req_log,
	dma_w_stat_log,
	dma_r_req_log,
	dma_r_read_log,
	dma_r_stat_log,
	h2c_pkt_log,
	c2h_pkt_log,
	dbuff_notif_log,
	log_control_i,
    	log_out_o
    	);
}
