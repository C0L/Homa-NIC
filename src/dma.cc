#include "dma.hh"

#include "stdio.h"
#include "string.h"

#include <iostream>
#include <fstream>

using namespace std;

/**
 * dma_read() - TODO
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_read(hls::stream<am_cmd_t> & cmd_queue_o,
	      hls::stream<ap_uint<512>> & data_queue_i,
	      hls::stream<am_status_t> & status_queue_i,
	      hls::stream<srpt_data_fetch_t> & dma_req_i,
	      hls::stream<h2c_dbuff_t> & dbuff_in_o,
              hls::stream<log_status_t> & dma_read_log_o) {

    static srpt_data_fetch_t pending_reqs[128];
    static ap_uint <7> read_head  = 0;
    static ap_uint<7>  write_head = 0;

#pragma HLS dependence variable=pending_reqs inter WAR false
#pragma HLS dependence variable=pending_reqs inter RAW false

#pragma HLS dependence variable=pending_reqs intra WAR false
#pragma HLS dependence variable=pending_reqs intra RAW false

#pragma HLS pipeline II=1

    // The sendmsg buffer ID needs to come from the user sendmsg request
    // The output buffer is assigned on packet ingress. And that mapping must be stored 
    
    srpt_data_fetch_t dma_req;
    if (dma_req_i.read_nb(dma_req)) {
	am_cmd_t am_cmd;

	am_cmd(AM_CMD_TYPE)  = 1;
	am_cmd(AM_CMD_DSA)   = 0;
	am_cmd(AM_CMD_EOF)   = 1;
	am_cmd(AM_CMD_DRR)   = 1;
	am_cmd(AM_CMD_TAG)   = 0;
	am_cmd(AM_CMD_RSVD)  = 0;
	am_cmd(AM_CMD_BTT)   = 64;
	am_cmd(AM_CMD_SADDR) = dma_req(SRPT_DATA_FETCH_HOST_ADDR);

	cmd_queue_o.write(am_cmd);
	pending_reqs[write_head++] = dma_req;
    }

    ap_uint<512> chunk;
    if (data_queue_i.read_nb(chunk)) {
	srpt_data_fetch_t dma_req = pending_reqs[read_head++];

	h2c_dbuff_t dbuff_in;

	dbuff_in.data     = chunk;
	dbuff_in.dbuff_id = dma_req(SRPT_DATA_FETCH_DBUFF_ID);
	dbuff_in.local_id = dma_req(SRPT_DATA_FETCH_RPC_ID);
	dbuff_in.msg_addr = dma_req(SRPT_DATA_FETCH_HOST_ADDR);
	dbuff_in.msg_len  = dma_req(SRPT_DATA_FETCH_MSG_LEN);
	// dbuff_in.last = dma_req.last; // TODO unused
	dbuff_in_o.write(dbuff_in);
    }

    am_status_t am_status;
    if (status_queue_i.read_nb(am_status)) {
	// TODO do nothing?
    }
}

/**
 * dma_write() - TODO 
 * @maxi        - The MAXI interface connected to DMA space
 * @dam_w_req_i - 64B data chunks for storage in data space
 */
void dma_write(hls::stream<am_cmd_t> & cmd_queue_o,
	       hls::stream<ap_uint<512>> & data_queue_o,
	       hls::stream<am_status_t> & status_queue_i,
	       hls::stream<dma_w_req_t> & dma_w_req_i,
	       hls::stream<log_status_t> & dma_write_log_o) {

#pragma HLS pipeline II=1

   dma_w_req_t dma_req;

   if (dma_w_req_i.read_nb(dma_req)) {
       // TODO describe and check this
       
       am_cmd_t am_cmd;
       am_cmd(AM_CMD_TYPE)  = 1;
       am_cmd(AM_CMD_DSA)   = 0;
       am_cmd(AM_CMD_EOF)   = 1;
       am_cmd(AM_CMD_DRR)   = 1;
       am_cmd(AM_CMD_TAG)   = 0;
       am_cmd(AM_CMD_RSVD)  = 0;
       am_cmd(AM_CMD_BTT)   = dma_req.strobe;
       am_cmd(AM_CMD_SADDR) = dma_req.offset;

       cmd_queue_o.write(am_cmd);
       data_queue_o.write(dma_req.data);
   }

   am_status_t am_status;
   if (status_queue_i.read_nb(am_status)) {
       // TODO do nothing?
   }
}
