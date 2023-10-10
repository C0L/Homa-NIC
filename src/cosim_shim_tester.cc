#include "homa.hh"
#include "cosim_shims.hh"

int main () {
    hls::stream<srpt_queue_entry_t> sendmsg_i;
    hls::stream<srpt_queue_entry_t> cache_req_o;

    fetch_shim(sendmsg_i, cache_req_o);

    srpt_queue_entry_t new_fetch;
    new_fetch(SRPT_QUEUE_ENTRY_RPC_ID) = 1;
    new_fetch(SRPT_QUEUE_ENTRY_DBUFF_ID) = 1;
    new_fetch(SRPT_QUEUE_ENTRY_REMAINING) = 512;
    new_fetch(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
    new_fetch(SRPT_QUEUE_ENTRY_GRANTED) = 0;
    new_fetch(SRPT_QUEUE_ENTRY_PRIORITY) = SRPT_ACTIVE;

    sendmsg_i.write(new_fetch);

    for (int i = 0; i < 8; ++i) {
	srpt_queue_entry_t fetch_out = cache_req_o.read();
	std::cerr << "REMAINING: " << fetch_out(SRPT_QUEUE_ENTRY_REMAINING) << std::endl;
	std::cerr << "DBUFFERED: " << fetch_out(SRPT_QUEUE_ENTRY_DBUFFERED) << std::endl;
    }
}
