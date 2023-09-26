#include "logger.hh"

void logger(hls::stream<log_status_t> & dma_read_log_i,
	    hls::stream<log_status_t> & dma_write_log_i,
	    hls::stream<log_entry_t>  & log_out_o) {

    static log_entry_t circular_log[2048];
    static ap_uint<11> read_head  = 0;
    static ap_uint<11> write_head = 0;

    log_entry_t new_entry;
    bool write_entry;

    log_status_t dma_read_log;
    if (dma_read_log_i.read_nb(dma_read_log)) {
	new_entry(7, 0) = dma_read_log;
    }

    log_status_t dma_write_log;
    if (dma_write_log_i.read_nb(dma_write_log)) {
	new_entry(15, 8) = dma_write_log;
    }

    if (log_out_o.write_nb(circular_log[read_head])) {
	read_head++;
    }

    if (new_entry) {
	circular_log[write_head++] = new_entry;
	if (write_head == read_head) {
	    read_head++;
	}
    }
}
