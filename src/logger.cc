#include "logger.hh"

void logger(hls::stream<ap_uint<8>> & dma_w_req_log_i,
	    hls::stream<ap_uint<8>> & dma_w_stat_log_i, 
	    hls::stream<ap_uint<8>> & dma_r_req_log_i,
	    hls::stream<ap_uint<8>> & dma_r_read_log_i,
	    hls::stream<ap_uint<8>> & dma_r_stat_log_i,
	    hls::stream<ap_uint<8>> & h2c_pkt_log_i,
	    hls::stream<ap_uint<8>> & c2h_pkt_log_i,
	    hls::stream<ap_uint<8>> & dbuff_notif_log_i,
	    hls::stream<ap_uint<8>> & log_control_i,
	    hls::stream<log_entry_t> & log_out_o) {

    static ap_uint<8> log_state = LOG_RECORD;

    static log_entry_t circular_log[2048];
    static ap_uint<11> read_head  = 0;
    static ap_uint<11> write_head = 0;
    static ap_uint<32> count = 0;

#pragma HLS pipeline II=1

    log_entry_t new_entry;
    new_entry.data = 0;
    new_entry.last = 1;
    bool add_entry = false;

    if (read_head != write_head && log_state == LOG_DRAIN) {
	log_entry_t write_entry = circular_log[read_head++];
	log_out_o.write(write_entry);
    }

    ap_uint<8> dma_w_req_log;
    if (dma_w_req_log_i.read_nb(dma_w_req_log)) {
	new_entry.data(7, 0) = dma_w_req_log;
	add_entry = true;
    }

    ap_uint<8> dma_w_stat_log;
    if (dma_w_stat_log_i.read_nb(dma_w_stat_log)) {
	new_entry.data(15, 8) = dma_w_stat_log;
	add_entry = true;
    }

    ap_uint<8> dma_r_req_log;
    if (dma_r_req_log_i.read_nb(dma_r_req_log)) {
	new_entry.data(23, 16) = dma_r_req_log;
	add_entry = true;
    }

    ap_uint<8> dma_r_read_log;
    if (dma_r_read_log_i.read_nb(dma_r_read_log)) {
	new_entry.data(31, 24) = dma_r_read_log;
	add_entry = true;
    }

    ap_uint<8> dma_r_stat_log;
    if (dma_r_stat_log_i.read_nb(dma_r_stat_log)) {
	new_entry.data(39, 32) = dma_r_stat_log;
	add_entry = true;
    }

    ap_uint<8> h2c_pkt_log;
    if (h2c_pkt_log_i.read_nb(h2c_pkt_log)) {
	new_entry.data(47, 40) = h2c_pkt_log;
	add_entry = true;
    }

    ap_uint<8> c2h_pkt_log;
    if (c2h_pkt_log_i.read_nb(c2h_pkt_log)) {
	new_entry.data(55, 48) = c2h_pkt_log;
	add_entry = true;
    }

    ap_uint<8> dbuff_notif_log;
    if (dbuff_notif_log_i.read_nb(dbuff_notif_log)) {
	new_entry.data(63, 56) = dbuff_notif_log;
	add_entry = true;
    }

    new_entry.data(127, 100) = count;

    if (add_entry) {
	circular_log[write_head++] = new_entry;
	if (write_head == read_head) {
	    read_head++;
	}
    }

    log_control_i.read_nb(log_state);

    count++;
}
