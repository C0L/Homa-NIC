#include "logger.hh"

void logger(hls::stream<ap_uint<64>> & dma_read_log_i,
	    hls::stream<ap_uint<64>> & dma_write_log_i,
	    hls::stream<log_entry_t>  & log_out_o) {

    static log_entry_t circular_log[2048];
    static ap_uint<11> read_head  = 0;
    static ap_uint<11> write_head = 0;

    log_entry_t new_entry;
    new_entry.data = 0;
    new_entry.last = 1;
    bool add_entry = false;

    if (read_head != write_head) {
	log_entry_t write_entry = circular_log[read_head++];
	write_entry.last = 1;
	log_out_o.write(write_entry);
    }

    ap_uint<64> dma_read_log;
    if (dma_read_log_i.read_nb(dma_read_log)) {
	std::cerr << "LOG READ " << dma_read_log << std::endl;
	new_entry.data(63, 0) = dma_read_log;
	add_entry = true;
    }

    ap_uint<64> dma_write_log;
    if (dma_write_log_i.read_nb(dma_write_log)) {
	std::cerr << "LOG WRITE" << dma_write_log << std::endl;
	new_entry.data(127, 64) = dma_write_log;
	add_entry = true;
    }

    if (add_entry) {
	circular_log[write_head++] = new_entry;
	if (write_head == read_head) {
	    read_head++;
	}
    }
}
