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

	// TODO THIS SHOULD BE PORT AND NOT RPC ID!!!!!!!!!
	dma_r_req_out(DMA_R_REQ_HOST_ADDR) = h2c_data_map[dma_r_req_in(SRPT_QUEUE_ENTRY_RPC_ID)] + dma_r_req_in(SRPT_QUEUE_ENTRY_DBUFFERED);
	// TODO 
	dma_r_req_out(DMA_R_REQ_BYTES)     = dma_r_req_in(SRPT_QUEUE_ENTRY_REMAINING) > 64 ? 64 : dma_r_req_in(SRPT_QUEUE_ENTRY_REMAINING);
	// dma_r_req_out(DMA_R_REQ_BYTES)     = dma_r_req_in(SRPT_QUEUE_ENTRY_REMAINING) > 256 ? 256 : dma_r_req_in(SRPT_QUEUE_ENTRY_REMAINING);
	dma_r_req_out(DMA_R_REQ_DBUFF_ID)  = dma_r_req_in(SRPT_QUEUE_ENTRY_DBUFF_ID);
	dma_r_req_out(DMA_R_REQ_BUFF_SIZE) = dma_r_req_in(SRPT_QUEUE_ENTRY_GRANTED);
	dma_r_req_out(DMA_R_REQ_MSG_ADDR)  = dma_r_req_in(SRPT_QUEUE_ENTRY_DBUFFERED); 
	dma_r_req_o.write(dma_r_req_out);
    }

    msghdr_send_t dma_w_sendmsg;
    dma_w_req_t dma_w_data;
    if (dma_w_sendmsg_i.read_nb(dma_w_sendmsg)) {
	dma_w_req_t dma_w_req_out;
	dma_w_req_out(DMA_W_REQ_HOST_ADDR) = metadata_map[dma_w_sendmsg(MSGHDR_SPORT)] + (64 * dma_w_sendmsg(MSGHDR_RETURN));
	dma_w_req_out(DMA_W_REQ_DATA)      = dma_w_sendmsg;
	dma_w_req_out(DMA_W_REQ_STROBE)    = 64;

	dma_w_req_o.write(dma_w_req_out);
    } else if (dma_w_data_i.read_nb(dma_w_data)) {
	dma_w_data(DMA_W_REQ_HOST_ADDR) = dma_w_data(DMA_W_REQ_HOST_ADDR) + c2h_data_map[dma_w_data(DMA_W_REQ_COOKIE)];
	dma_w_data(DMA_W_REQ_STROBE) = 64;

	dma_w_req_o.write(dma_w_data);
    }
}

/**
 * h2c_dma() - 
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void h2c_dma(hls::stream<am_cmd_t> & cmd_queue_o,
	     hls::stream<am_status_t> & status_queue_i,
	     hls::stream<srpt_queue_entry_t> & dbuff_notif_o,
	     hls::stream<dma_r_req_t> & dma_r_req_i) {
 
    static dma_r_req_t pending_reqs[256];
    static ap_uint<8> tag = 0;

#pragma HLS interface mode=ap_ctrl_none port=return

#pragma HLS pipeline II=1

#pragma HLS interface axis port=cmd_queue_o
#pragma HLS interface axis port=status_queue_i
#pragma HLS interface axis port=dma_r_req_i
#pragma HLS interface axis port=dbuff_notif_o

    dma_r_req_t dma_req;
    if (dma_r_req_i.read_nb(dma_req)) {

	am_cmd_t am_cmd;
	am_cmd.data(AM_CMD_PCIE_ADDR) = dma_req(DMA_R_REQ_HOST_ADDR);
	am_cmd.data(AM_CMD_SEL)       = 0;
	am_cmd.data(AM_CMD_RAM_ADDR)  = dma_req(DMA_R_REQ_MSG_ADDR);
	am_cmd.data(AM_CMD_LEN)       = dma_req(DMA_R_REQ_BYTES);
	am_cmd.data(AM_CMD_TAG)       = tag;

	cmd_queue_o.write(am_cmd);
	pending_reqs[tag++] = dma_req;
    }

    // Take input chunks and add them to the data buffer
    am_status_t status;
    if (status_queue_i.read_nb(status)) {
	dma_r_req_t new_entry = pending_reqs[status.data(AM_STATUS_TAG)];

	srpt_queue_entry_t dbuff_notif;
	dbuff_notif(SRPT_QUEUE_ENTRY_DBUFF_ID)  = new_entry(DMA_R_REQ_DBUFF_ID);
	dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED) = new_entry(DMA_R_REQ_BUFF_SIZE) - new_entry(DMA_R_REQ_MSG_ADDR) - 256;
	dbuff_notif(SRPT_QUEUE_ENTRY_PRIORITY)  = 1; // TODO SRPT_DBUFF_UPDATE

	dbuff_notif_o.write(dbuff_notif);
    }
}


// TODO
// DO NOT TRIGGER OFF OF THE STATUS OUTPUT WITHOUT ANY BACKPRESSURE (NO TREADY!??!?)



/**
 * dma_write()  - TODO 
 * @maxi        - The MAXI interface connected to DMA space
 * @dam_w_req_i - 64B data chunks for storage in data space
 */

void c2h_dma(hls::stream<ram_cmd_t> & ram_cmd_o,
	     hls::stream<ap_axiu<512,0,0,0>> & ram_data_o,
	     hls::stream<ram_status_t> & ram_status_i,
	     hls::stream<am_cmd_t> & pcie_cmd_o,
	     hls::stream<dma_w_req_t> & dma_w_req_i) {

   static am_cmd_t pending_reqs[256];
   static ap_uint<8> tag = 0;

#pragma HLS interface mode=ap_ctrl_none port=return
#pragma HLS pipeline II=1

#pragma HLS interface axis port=ram_cmd_o
#pragma HLS interface axis port=ram_data_o
#pragma HLS interface axis port=ram_status_i
#pragma HLS interface axis port=pcie_cmd_o
#pragma HLS interface axis port=dma_w_req_i

   static ap_uint<14> ram_head = 0;

    dma_w_req_t dma_req;
    if (dma_w_req_i.read_nb(dma_req)) {

	ram_cmd_t ram_cmd;
	ram_cmd.data(RAMW_CMD_ADDR) = ram_head;
	ram_cmd.data(RAMW_CMD_LEN)  = dma_req(DMA_W_REQ_STROBE);
	ram_cmd.data(RAMW_CMD_TAG)  = tag;

	am_cmd_t am_cmd;
	am_cmd.data(AM_CMD_PCIE_ADDR) = dma_req(DMA_R_REQ_HOST_ADDR);
	am_cmd.data(AM_CMD_SEL)       = 0;
	am_cmd.data(AM_CMD_RAM_ADDR)  = ram_head;
	am_cmd.data(AM_CMD_LEN)       = dma_req(DMA_W_REQ_STROBE);
	am_cmd.data(AM_CMD_TAG)       = tag;

	ram_head += 64;

	ram_cmd_o.write(ram_cmd);

	ap_axiu<512,0,0,0> data_out;
	data_out.data  = dma_req(DMA_W_REQ_DATA);
	data_out.last  = 1;
	data_out.keep = (0xFFFFFFFFFFFFFFFF >> (64 - dma_req(DMA_W_REQ_STROBE)));
	ram_data_o.write(data_out);
	
	pending_reqs[tag++] = am_cmd;
    }

    ram_status_t status;
    if (ram_status_i.read_nb(status)) {
	am_cmd_t pcie_cmd = pending_reqs[status.data(RAMW_STATUS_TAG)];

	pcie_cmd_o.write(pcie_cmd);
    }


/*

   dma_w_req_t dma_req;
   if (dma_w_req_i.read_nb(dma_req)) {
       am_cmd_t am_cmd;
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

*/

   // am_status_t am_status;
   // if (status_queue_i.read_nb(am_status)) {
   //     log_out(15,8) = am_status.data;
   // }

   // if (log_out != 0) dma_w_req_log_o.write(log_out);
}

