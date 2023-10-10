#ifndef COSHIMS_H
#define COSHIMS_H

void fetch_shim(hls::stream<srpt_queue_entry_t> & sendmsg_i,
		hls::stream<srpt_queue_entry_t> & cache_req_o);

#endif
