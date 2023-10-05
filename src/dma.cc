#include "dma.hh"

#include "stdio.h"
#include "string.h"

#include <iostream>
#include <fstream>

using namespace std;


/* TODO CAREFUL
   In the absence of any S2MM command, AXI DataMover will pull the s_axis_s2mm_tready signal to Low after taking in four beats of streaming data. This will throttle the input data stream. To have a minimum amount of throttling, ensure that a valid command is issued to the S2MM interface much before the actual data arrives. */

/**
 * dma_read() - TODO
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_read(hls::stream<am_cmd_t> & cmd_queue_o,
	      hls::stream<ap_axiu<512,0,0,0>> & data_queue_i,
	      hls::stream<am_status_t> & status_queue_i,
	      hls::stream<dma_r_req_t> & dma_req_i,
	      hls::stream<h2c_dbuff_t> & dbuff_in_o,
              hls::stream<ap_uint<64>> & dma_read_log_o) {

    static dma_r_req_t pending_reqs[128];
    static ap_uint <7> read_head  = 0;
    static ap_uint<7>  write_head = 0;

#pragma HLS dependence variable=pending_reqs inter WAR false
#pragma HLS dependence variable=pending_reqs inter RAW false

#pragma HLS dependence variable=pending_reqs intra WAR false
#pragma HLS dependence variable=pending_reqs intra RAW false

#pragma HLS pipeline II=1

    // The sendmsg buffer ID needs to come from the user sendmsg request
    // The output buffer is assigned on packet ingress. And that mapping must be stored 
    
    dma_r_req_t dma_req;
    if (dma_req_i.read_nb(dma_req)) {
	am_cmd_t am_cmd;

	am_cmd.data(AM_CMD_TYPE)  = 1;
	am_cmd.data(AM_CMD_DSA)   = 0;
	am_cmd.data(AM_CMD_EOF)   = 0;
	am_cmd.data(AM_CMD_DRR)   = 0;
	am_cmd.data(AM_CMD_TAG)   = 0xf;
	am_cmd.data(AM_CMD_RSVD)  = 0;
	am_cmd.data(AM_CMD_BTT)   = 64;
	am_cmd.data(AM_CMD_SADDR) = dma_req(DMA_R_REQ_HOST_ADDR);

	dma_read_log_o.write(am_cmd.data(AM_CMD_SADDR));

	cmd_queue_o.write(am_cmd);
	pending_reqs[write_head++] = dma_req;
    }

    ap_axiu<512,0,0,0> chunk;
    if (data_queue_i.read_nb(chunk)) {
	dma_r_req_t dma_req = pending_reqs[read_head++];

	h2c_dbuff_t dbuff_in;

	dbuff_in.data     = chunk.data;
	dbuff_in.dbuff_id = dma_req(DMA_R_REQ_DBUFF_ID);
	dbuff_in.local_id = dma_req(DMA_R_REQ_RPC_ID);

	// HACK: This should be handled in RPC state
	dbuff_in.msg_addr = dma_req(DMA_R_REQ_MSG_LEN) - dma_req(DMA_R_REQ_REMAINING);
	dbuff_in.msg_len  = dma_req(DMA_R_REQ_MSG_LEN);

	std::cerr << "DBUFF ADDR "  << dbuff_in.msg_addr << std::endl;
	std::cerr << "DBUFF MSG LEN "  << dbuff_in.msg_len << std::endl;

	chunk.data(63,60) = 4;

	dma_read_log_o.write(chunk.data(63,0));

	// dbuff_in.last = dma_req.last; // TODO unused
	dbuff_in_o.write(dbuff_in);
    }

    am_status_t am_status;
    if (status_queue_i.read_nb(am_status)) {
	ap_uint<64> status = am_status.data;
	status(63,60) = 0xF;

	dma_read_log_o.write(status);
	// TODO do nothing?
    }
}

/**
 * dma_write() - TODO 
 * @maxi        - The MAXI interface connected to DMA space
 * @dam_w_req_i - 64B data chunks for storage in data space
 */
void dma_write(hls::stream<am_cmd_t> & cmd_queue_o,
	       hls::stream<ap_axiu<512,0,0,0>> & data_queue_o,
	       hls::stream<am_status_t> & status_queue_i,
	       hls::stream<dma_w_req_t> & dma_w_req_i,
	       hls::stream<ap_uint<64>> & dma_write_log_o) {

#pragma HLS pipeline II=1

   dma_w_req_t dma_req;

   if (dma_w_req_i.read_nb(dma_req)) {
       // TODO describe and check this

       std::cerr << "DMA WRITE OPERATION!!!!!\n";
       
       am_cmd_t am_cmd;
       am_cmd.data(AM_CMD_TYPE)  = 1;
       am_cmd.data(AM_CMD_DSA)   = 0;
       am_cmd.data(AM_CMD_EOF)   = 0;
       am_cmd.data(AM_CMD_DRR)   = 0;
       am_cmd.data(AM_CMD_TAG)   = 0xf;
       am_cmd.data(AM_CMD_RSVD)  = 0;
       am_cmd.data(AM_CMD_BTT)   = dma_req.strobe;
       am_cmd.data(AM_CMD_SADDR) = dma_req.offset;

       ap_uint<64> log_entry = am_cmd.data(AM_CMD_SADDR);
       log_entry(63,60) = 1;

       // TODO remove test offset
       dma_write_log_o.write(log_entry);

       log_entry = dma_req.data(63,0);
       log_entry(63,60) = 2;

       dma_write_log_o.write(log_entry);

       cmd_queue_o.write(am_cmd);

       ap_axiu<512,0,0,0> data_out;
       data_out.data = dma_req.data;
       data_out.last = 1;

       data_queue_o.write(data_out);
   }

   am_status_t am_status;
   if (status_queue_i.read_nb(am_status)) {
       // TODO do nothing?
   }
}
