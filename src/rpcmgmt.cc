#include "rpcmgmt.hh"
#include "hashmap.hh"

using namespace hls;


/**
 * rpc_state() - Maintains state associated with an RPC and augments
 *   flows with that state when needed. There are 3 actions taken by
 *   the core: 1) Incorporating new sendmsg requests: New sendmsg
 *   requests arrive on the onboard_send_i stream. Their data is
 *   gathered and inserted into the table at the index specified by
 *   the RPC ID. The sendmsg is then passed to the srpt data core so
 *   that it can be scheduled to be sent.
 *
 *   2) Augmenting outgoing headers: Headers that need to be sent onto
 *   the link arrive on the header_out_i stream. The RPC ID of the
 *   header out request is used to locate the associated RPC state,
 *   and that state is loaded into the header. That header is then passed to the next core.
 *
 *   3) Augmenting incoming headers: headers that arrive from the link
 *   need to update the state within this core. Their RPC ID is used
 *   to determine where the data should be placed in the rpc
 *   table. Only the first packet in a sequence causes an insertion
 *   into the table. If the message length is greater than the number
 *   of incoming bytes then that means data should be forwarded to the
 *   grant queue. If the incoming packet was a grant then that updated
 *   is forwarded to the srpt data queue, indicating it may have more
 *   bytes to transmit.
 *
 * @onboard_send_i - Input for sendmsg requests
 * @onboard_send_o - Output for sendmsg requests
 * @header_out_i   - Input for outgoing headers
 * @header_out_o   - Output for outgoing headers 
 * @header_in_i    - Input for incoming headers
 * @header_in_o    - Output for incoming headers 
 * @grant_in_o     - Output for forwarding data to SRPT grant queue
 * @data_srpt_o    - Output for forwarding data to the SRPT data queue
 */
void rpc_state(
    hls::stream<onboard_send_t> & onboard_send_i,
    hls::stream<srpt_queue_entry_t> & data_queue_o,
    hls::stream<srpt_queue_entry_t> & fetch_queue_o,
    hls::stream<header_t> & h2c_header_i,
    hls::stream<header_t> & h2c_header_o,
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<srpt_grant_new_t> & grant_srpt_o,
    hls::stream<srpt_queue_entry_t> & data_srpt_o,
    hls::stream<ap_uint<8>> & h2c_pkt_log_o,
    hls::stream<ap_uint<8>> & c2h_pkt_log_o
    ) {

    // TODO The tables for client and server can be totally independent?
    // This makes the write operations eaisier?
    static homa_rpc_t rpcs[MAX_RPCS];

#pragma HLS pipeline II=1

#pragma HLS dependence variable=rpcs inter WAR false
#pragma HLS dependence variable=rpcs inter RAW false

    onboard_send_t onboard_send;
    if (onboard_send_i.read_nb(onboard_send)) {
	homa_rpc_t homa_rpc;
	homa_rpc.daddr     = onboard_send.daddr;
	homa_rpc.dport     = onboard_send.dport;
	homa_rpc.saddr     = onboard_send.saddr;
	homa_rpc.sport     = onboard_send.sport;
	homa_rpc.buff_addr = onboard_send.buff_addr;
	homa_rpc.buff_size = onboard_send.buff_size;
	homa_rpc.id        = onboard_send.id;

	rpcs[onboard_send.local_id] = homa_rpc;

	srpt_queue_entry_t srpt_data_in;
	srpt_data_in(SRPT_QUEUE_ENTRY_RPC_ID)    = onboard_send.local_id;
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = onboard_send.h2c_buff_id;
	srpt_data_in(SRPT_QUEUE_ENTRY_REMAINING) = onboard_send.buff_size;
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFFERED) = onboard_send.buff_size;
	// TODO would be cleaner if we could just set this to RTT_BYTES
	srpt_data_in(SRPT_QUEUE_ENTRY_GRANTED)  = onboard_send.buff_size - ((((ap_uint<32>) RTT_BYTES) > onboard_send.buff_size)
									 ? onboard_send.buff_size : ((ap_uint<32>) RTT_BYTES));
	srpt_data_in(SRPT_QUEUE_ENTRY_PRIORITY) = SRPT_ACTIVE;


	data_queue_o.write(srpt_data_in);

	srpt_queue_entry_t srpt_fetch_in = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_RPC_ID)    = onboard_send.local_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = onboard_send.h2c_buff_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_REMAINING) = onboard_send.buff_size;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_GRANTED)   = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_PRIORITY)   = SRPT_ACTIVE;
	fetch_queue_o.write(srpt_fetch_in);
    }

    /* RO Paths */
    header_t h2c_header;
    if (h2c_header_i.read_nb(h2c_header)) {
	homa_rpc_t homa_rpc = rpcs[h2c_header.local_id];

	h2c_header.daddr = homa_rpc.daddr;
	h2c_header.dport = homa_rpc.dport;
	h2c_header.saddr = homa_rpc.saddr;
	h2c_header.sport = homa_rpc.sport;
	h2c_header.id    = homa_rpc.id;

	// TODO will need to move
	h2c_header.h2c_buff_id    = homa_rpc.h2c_buff_id;
	h2c_header.incoming       = homa_rpc.buff_size - h2c_header.incoming;
	h2c_header.data_offset    = homa_rpc.buff_size - h2c_header.data_offset;
	h2c_header.message_length = homa_rpc.buff_size;

	h2c_header_o.write_nb(h2c_header);

	if (h2c_header.type == DATA) {
	    h2c_pkt_log_o.write(LOG_DATA_OUT);
	} else if (h2c_header.type == GRANT) {
	    h2c_pkt_log_o.write(LOG_GRANT_OUT);
	}
    }

    header_t c2h_header;
    if (c2h_header_i.read_nb(c2h_header)) {

	std::cerr << "RPC STATE READ HEADER IN\n";
	homa_rpc_t homa_rpc = rpcs[c2h_header.local_id];

	switch (c2h_header.type) {
	    case DATA: {
		if ((c2h_header.packetmap & PMAP_INIT) == PMAP_INIT) {
		    homa_rpc.saddr = c2h_header.saddr;
		    homa_rpc.daddr = c2h_header.daddr;
		    homa_rpc.dport = c2h_header.dport;
		    homa_rpc.sport = c2h_header.sport;
		    homa_rpc.id    = c2h_header.id;
		} else {
		    c2h_header.id = homa_rpc.id;
		}

		if (c2h_header.message_length > c2h_header.incoming) { 
		    srpt_grant_new_t grant_in;
		    // grant_in(SRPT_GRANT_NEW_OFFSET)  = header_in.data_offset;
		    grant_in(SRPT_GRANT_NEW_MSG_LEN) = c2h_header.message_length;
		    grant_in(SRPT_GRANT_NEW_RPC_ID)  = c2h_header.local_id;
		    grant_in(SRPT_GRANT_NEW_PEER_ID) = c2h_header.peer_id;
		    grant_in(SRPT_GRANT_NEW_PMAP)    = c2h_header.packetmap;

		    // Notify the grant queue of this receipt
		    grant_srpt_o.write(grant_in); 
		} 

		// Instruct the data buffer where to write this message's data
		c2h_header_o.write(c2h_header);

		c2h_pkt_log_o.write(LOG_DATA_OUT);

		break;
	    }

	    case GRANT: {
		srpt_queue_entry_t srpt_grant_notif;
		srpt_grant_notif(SRPT_QUEUE_ENTRY_RPC_ID)  = c2h_header.local_id;
		srpt_grant_notif(SRPT_QUEUE_ENTRY_GRANTED) = c2h_header.grant_offset;
		srpt_grant_notif(SRPT_QUEUE_ENTRY_PRIORITY) = SRPT_GRANT_UPDATE;
		data_srpt_o.write(srpt_grant_notif);

		h2c_pkt_log_o.write(LOG_GRANT_OUT);

		break;
	    }
	}

	rpcs[c2h_header.local_id] = homa_rpc;
    } 
}

/**
 * rpc_map() - Maintains a map for incoming packets in which this core is
 * the server from (addr, ID, port) -> (local rpc id). This also maintains
 * the map from (addr) -> (local peer id) for the purpose of determining if
 * RPCs share a peer, which is particularly useful for the SRPT grant. This
 * core manages the unique local RPC IDs (indexes into the RPC state) and
 * the unique local peer IDs.
 * @header_in_i - Input for incoming headers to be looked up
 * @header_in_o - Output for incoming headers with discovered ID
 * @sendmsg_i   - Input for sendmsg request to create entries 
 * @sendmsg_o   - Output for sendmsgs to the next core
 */
void id_map(hls::stream<header_t> & header_in_i,
	    hls::stream<header_t> & header_in_o) {

#pragma HLS pipeline II=1

    // Unique local RPC ID assigned when this core is the server
    static stack_t<local_id_t, MAX_RPCS> server_ids(STACK_ODD);

    // Unique local Peer IDs
    static stack_t<peer_id_t, MAX_PEERS> peer_ids(STACK_ALL);

    // hash(dest addr, sender ID, dest port) -> rpc ID
    static hashmap_t<rpc_hashpack_t, local_id_t, RPC_BUCKETS, RPC_BUCKET_SIZE, RPC_SUB_TABLE_INDEX, RPC_HP_SIZE> rpc_hashmap;
    // hash(dest addr) -> peer ID
    static hashmap_t<peer_hashpack_t, peer_id_t, PEER_BUCKETS, PEER_BUCKET_SIZE, PEER_SUB_TABLE_INDEX, PEER_HP_SIZE> peer_hashmap;

    onboard_send_t onboard_send;
    header_t header_in;

    if (header_in_i.read_nb(header_in)) {
	std::cerr << "map read header in\n";
	/* Perform the RPC ID lookup if we are the server */
	if (!IS_CLIENT(header_in.id)) {
	    // Check if this RPC is already registered
	    rpc_hashpack_t rpc_query = {header_in.saddr, header_in.id, header_in.sport, 0};

	    header_in.local_id = rpc_hashmap.search(rpc_query);

	    // If the rpc is not registered, generate a new RPC ID and register it 
	    if (header_in.local_id == 0) {
		header_in.local_id = server_ids.pop();
		entry_t<rpc_hashpack_t, local_id_t> rpc_entry = {rpc_query, header_in.local_id};
		rpc_hashmap.insert(rpc_entry);
	    }
	} else {
	    header_in.local_id = header_in.id;
	}

	/* Perform the peer ID lookup regardless */
	peer_hashpack_t peer_query = {header_in.saddr};
	header_in.peer_id          = peer_hashmap.search(peer_query);
	
	// If the peer is not registered, generate new ID and register it
	if (header_in.peer_id == 0) {
	    header_in.peer_id = peer_ids.pop();
	    
	    entry_t<peer_hashpack_t, peer_id_t> peer_entry = {peer_query, header_in.peer_id};
	    peer_hashmap.insert(peer_entry);
	}
	std::cerr << "map wrote header out \n";
	header_in_o.write(header_in);
    }
}
