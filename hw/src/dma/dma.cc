#include "dma.hh"
#include "hls_burst_maxi.h"

using namespace std;

void addr_map(ap_uint<64> metadata_map[NUM_PORTS],
	      ap_uint<64> c2h_data_map[NUM_PORTS],
	      ap_uint<64> h2c_data_map[NUM_PORTS],
	      hls::stream<msghdr_send_t> & dma_w_sendmsg_i,
	      hls::stream<dma_w_req_t> & dma_w_data_i,
	      hls::stream<srpt_queue_entry_t> & dma_r_req_i,
	      hls::stream<dma_w_req_t> & dma_w_req_o,
	      hls::stream<dma_r_req_t> & dma_r_req_o) {

#pragma HLS interface mode=ap_ctrl_none port=return

#pragma HLS pipeline II=1

#pragma HLS interface bram port=metadata_map
#pragma HLS interface bram port=c2h_data_map
#pragma HLS interface bram port=h2c_data_map

#pragma HLS interface axis port=dma_w_sendmsg_i
#pragma HLS interface axis port=dma_w_data_i
#pragma HLS interface axis port=dma_r_req_i
#pragma HLS interface axis port=dma_w_req_o
#pragma HLS interface axis port=dma_r_req_o

    srpt_queue_entry_t dma_r_req_in;
    if (dma_r_req_i.read_nb(dma_r_req_in)) {
	dma_r_req_t dma_r_req_out;
	// TODO misnomer. This is actually the port and address offset
	dma_r_req_out(DMA_R_REQ_HOST_ADDR) = h2c_data_map[dma_r_req_in(SRPT_QUEUE_ENTRY_RPC_ID)] + dma_r_req_in(SRPT_QUEUE_ENTRY_GRANTED);
	//dma_r_req_out(DMA_R_REQ_BYTES)     = dma_r_req_in(SRPT_QUEUE_ENTRY_REMAINING) > 2048 ? 2048 : dma_r_req_in(SRPT_QUEUE_ENTRY_REMAINING);

	dma_r_req_out(DMA_R_REQ_BYTES)     = dma_r_req_in(SRPT_QUEUE_ENTRY_REMAINING) > 256 ? 256 : dma_r_req_in(SRPT_QUEUE_ENTRY_REMAINING);
	dma_r_req_out(DMA_R_REQ_COOKIE)    = dma_r_req_in(SRPT_QUEUE_ENTRY_DBUFF_ID);
	dma_r_req_o.write(dma_r_req_out);
    }

    msghdr_send_t dma_w_sendmsg;
    dma_w_req_t dma_w_data;
    if (dma_w_sendmsg_i.read_nb(dma_w_sendmsg)) {
	dma_w_req_t dma_w_req_out;
	dma_w_req_out(DMA_W_REQ_HOST_ADDR) = metadata_map[dma_w_sendmsg(MSGHDR_SPORT)] + (64 * dma_w_sendmsg(MSGHDR_RETURN));
	dma_w_req_out(DMA_W_REQ_DATA)   = dma_w_sendmsg;
	dma_w_req_out(DMA_W_REQ_STROBE) = 64;

	dma_w_req_o.write(dma_w_req_out);
    } else if (dma_w_data_i.read_nb(dma_w_data)) {
	dma_w_data(DMA_W_REQ_HOST_ADDR) = dma_w_data(DMA_W_REQ_HOST_ADDR) + c2h_data_map[dma_w_data(DMA_W_REQ_COOKIE)];
	dma_w_data(DMA_W_REQ_STROBE) = 64;

	dma_w_req_o.write(dma_w_data);
    }
}

/* TODO CAREFUL
   In the absence of any S2MM command, AXI DataMover will pull the s_axis_s2mm_tready signal to Low after taking in four beats of streaming data. This will throttle the input data stream. To have a minimum amount of throttling, ensure that a valid command is issued to the S2MM interface much before the actual data arrives. */

/**
 * dma_read() - 
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void h2c_dma(hls::stream<am_cmd_t> & cmd_queue_o,
	     hls::stream<dma_r_req_t> & dma_r_req_i) {
    // hls::stream<ap_uint<32>> & dma_r_req_log_o) {
 
    // static dma_r_req_t pending_reqs[256];
    // static ap_uint <7> read_head  = 0;
    // static ap_uint<7>  write_head = 0;

     static ap_uint<8> tag = 0;

#pragma HLS interface mode=ap_ctrl_none port=return

#pragma HLS pipeline II=1

#pragma HLS interface axis port=cmd_queue_o
// #pragma HLS interface axis port=status_queue_i
#pragma HLS interface axis port=dma_r_req_i
// #pragma HLS interface axis port=dma_r_req_o

    dma_r_req_t dma_req;
    if (dma_r_req_i.read_nb(dma_req)) {

	am_cmd_t am_cmd;
	am_cmd.data(AM_CMD_PCIE_ADDR) = dma_req(DMA_R_REQ_HOST_ADDR);
	am_cmd.data(AM_CMD_AXI_ADDR)  = 0;
	am_cmd.data(AM_CMD_LEN)       = dma_req(DMA_R_REQ_BYTES);
	am_cmd.data(AM_CMD_TAG)       = tag++;

	cmd_queue_o.write(am_cmd);
	// pending_reqs[write_head++] = dma_req;
    }
}

/**
 * dma_write()  - TODO 
 * @maxi        - The MAXI interface connected to DMA space
 * @dam_w_req_i - 64B data chunks for storage in data space
 */
/*
void c2h_dma(hls::stream<am_cmd_t> & cmd_queue_o,
	       hls::stream<ap_axiu<512,0,0,0>> & data_queue_o,
	       hls::stream<am_status_t> & status_queue_i,
	       hls::stream<dma_w_req_t> & dma_w_req_i,
	       hls::stream<ap_uint<32>> & dma_w_req_log_o) {

   static ap_uint<4> tag = 0;

#pragma HLS interface mode=ap_ctrl_none port=return
#pragma HLS pipeline II=1

#pragma HLS interface axis port=cmd_queue_o
#pragma HLS interface axis port=data_queue_o
#pragma HLS interface axis port=status_queue_i
#pragma HLS interface axis port=dma_w_req_i
// #pragma HLS interface axis port=dma_w_req_log_o

   ap_uint<32> log_out = 0;

   dma_w_req_t dma_req;
   if (dma_w_req_i.read_nb(dma_req)) {
       am_cmd_t am_cmd;
       // am_cmd.data(AM_CMD_TYPE)  = 1;
       // am_cmd.data(AM_CMD_DSA)   = 0;
       // am_cmd.data(AM_CMD_EOF)   = 1;
       // am_cmd.data(AM_CMD_DRR)   = 0;
       // am_cmd.data(AM_CMD_TAG)   = tag++;
       // am_cmd.data(AM_CMD_RSVD)  = 0;
       // am_cmd.data(AM_CMD_CACHE) = 1;
       // am_cmd.data(AM_CMD_USER)  = 0;
       // am_cmd.data(AM_CMD_BTT)   = dma_req(DMA_W_REQ_STROBE);
       // am_cmd.data(AM_CMD_SADDR) = dma_req(DMA_W_REQ_HOST_ADDR);

       am_cmd.data(AM_CMD_SADDR) = dma_req(DMA_W_REQ_HOST_ADDR);
       am_cmd.data(AM_CMD_BTT)   = dma_req(DMA_W_REQ_STROBE);
       am_cmd.data(AM_CMD_TAG)   = tag++;

       log_out(7,0) = LOG_DATA_W_REQ | tag;

       cmd_queue_o.write(am_cmd);

       ap_axiu<512,0,0,0> data_out;
       data_out.data  = dma_req(DMA_W_REQ_DATA);
       data_out.last  = 1;
       data_out.keep = (0xFFFFFFFFFFFFFFFF >> (64 - dma_req(DMA_W_REQ_STROBE)));

       data_queue_o.write(data_out);
   }

   am_status_t am_status;
   if (status_queue_i.read_nb(am_status)) {
       log_out(15,8) = am_status.data;
   }

   // if (log_out != 0) dma_w_req_log_o.write(log_out);
}
*/
