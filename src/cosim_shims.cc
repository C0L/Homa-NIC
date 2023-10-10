#include "homa.hh"
#include "srptmgmt.hh"

void sender(hls::stream<srpt_queue_entry_t> & sendmsg_top_i,
	    hls::stream<srpt_queue_entry_t> & cache_req_top_o,
	    hls::stream<srpt_queue_entry_t> & sendmsg_i,
	    hls::stream<srpt_queue_entry_t> & cache_req_o) {

#pragma HLS pipeline II=1

    static bool test = false;

    // TODO for now add an only once statement

    srpt_queue_entry_t sendmsg;
    if (test == false && sendmsg_top_i.read_nb(sendmsg)) {
	test = true;
	sendmsg_i.write(sendmsg);
    }
 
    srpt_queue_entry_t cache_req;
    if (cache_req_o.read_nb(cache_req)) {
	cache_req_top_o.write(cache_req);
    }   
}

void fetch_shim(hls::stream<srpt_queue_entry_t> & sendmsg_top_i,
		hls::stream<srpt_queue_entry_t> & cache_req_top_o) {

#pragma HLS interface axis port=sendmsg_top_i
#pragma HLS interface axis port=cache_req_top_o

#pragma HLS interface mode=ap_ctrl_none port=return

    hls_thread_local hls::stream<srpt_queue_entry_t, STREAM_DEPTH> sendmsg_i;
    hls_thread_local hls::stream<srpt_queue_entry_t, STREAM_DEPTH> cache_req_o;

    hls_thread_local hls::task sender_task(
	sender,
	sendmsg_top_i,  
	cache_req_top_o,
	sendmsg_i, 
	cache_req_o 
	);

    hls_thread_local hls::task srpt_fetch_queue_task(
	srpt_fetch_queue,
	sendmsg_i, // sendmsg_i
	cache_req_o // 
	);
}


