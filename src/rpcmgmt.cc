#include "rpcmgmt.hh"
#include "hashmap.hh"

using namespace hls;

/**
 * TODO revise
 * rpc_state() - Maintains state associated with an RPC and augments
 *   flows with that state when needed. There are 3 actions taken by
 *   the core: 1) Incorporating new sendmsg requests: New sendmsg
 *   requests arrive on the sendmsg_i stream. Their data is
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
 * @sendmsg_i      - Input for sendmsg requests
 * @data_entry_o   - Output to create entries in the SRPT data queue
 * @fetch_entry_o  - Output to create entries in the SRPT fetch queue
 * @h2c_header_i   - Input for headers to link
 * @h2c_header_o   - Output for headers to link 
 * @c2h_header_i   - Input for headers from link
 * @c2h_header_o   - Output for headers from headers 
 * @grant_update_o - Output for creating entries/updating SRPT grant queue
 * @data_update_o  - Output for creating updates in the SRPT data queue
 * @h2c_pkt_log_o  - Log output for host-to-card events
 * @c2h_pkt_log_o  - Log output for card-to-host events
 * 
 */
void rpc_state(
    // hls::stream<homa_rpc_t> & sendmsg_i,
    // hls::stream<homa_rpc_t> & sendmsg_i,
    hls::stream<msghdr_send_t> & msghdr_send_i,
    hls::stream<msghdr_send_t> & msghdr_send_o,
    hls::stream<srpt_queue_entry_t> & data_queue_o,
    hls::stream<srpt_queue_entry_t> & fetch_queue_o,
    hls::stream<header_t> & h2c_header_i,
    hls::stream<header_t> & h2c_header_o,
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<srpt_grant_new_t> & grant_srpt_o,
    hls::stream<srpt_queue_entry_t> & data_srpt_o,
    hls::stream<srpt_queue_entry> & dbuff_notif_i,
    hls::stream<srpt_queue_entry> & dbuff_notif_o,
    hls::stream<ap_uint<8>> & h2c_pkt_log_o,
    hls::stream<ap_uint<8>> & c2h_pkt_log_o
    ) {

    /* TODO: this is potentially not densely utilized because of paritiy of
     * rpc IDs for client vs server.
     */ 
    static homa_rpc_t client_rpcs[MAX_RPCS];
    static homa_rpc_t server_rpcs[MAX_RPCS];

    static stack_t<local_id_t, MAX_RPCS> client_ids(STACK_EVEN);


#pragma HLS pipeline II=1

    // TODO is this potentially risky with OOO small packets
    // Can maybe do a small sort of rebuffering if needed?
#pragma HLS dependence variable=client_rpcs inter WAR false
#pragma HLS dependence variable=client_rpcs inter RAW false
#pragma HLS dependence variable=server_rpcs inter WAR false
#pragma HLS dependence variable=server_rpcs inter RAW false

    /* For this core to be performant we need to ensure that only a
     * single execution path results in a write to a BRAM. This is
     * neccesary because the BRAMs are limited in the number of write
     * ports. Because we can arbitarily duplicate data we can have as
     * many read only paths as we please.
     *
     * To simplify this logic we break the RPC state into two blocks
     * of BRAMs, ones for clients and ones for servers. The clients
     * BRAMs need only be written when there is a new send message
     * REQUEST initiated by the user. A sendmsg RESPONSE will simply
     * reuse the entry in the server BRAMs. The server BRAMs need only
     * be written when there is unrequested bytes for a message that
     * we have not yet seen before.
     */

    /* read only paths */
    header_t h2c_header;
    if (h2c_header_i.read_nb(h2c_header)) {

	// Is this a REQUEST or RESPONSE
	homa_rpc_t homa_rpc = (IS_CLIENT(h2c_header.id)) ? client_rpcs[h2c_header.local_id] : server_rpcs[h2c_header.local_id];

	h2c_header.daddr = homa_rpc.daddr;
	h2c_header.dport = homa_rpc.dport;
	h2c_header.saddr = homa_rpc.saddr;
	h2c_header.sport = homa_rpc.sport;
	h2c_header.id    = homa_rpc.id;

	// Log the packet type we are sending
	h2c_pkt_log_o.write((h2c_header.type == DATA) ? LOG_DATA_OUT : LOG_GRANT_OUT);

	// Get the location of buffered data and prepare for packet construction
	h2c_header.h2c_buff_id    = homa_rpc.h2c_buff_id;
	h2c_header.incoming       = homa_rpc.buff_size - h2c_header.incoming;
	h2c_header.data_offset    = homa_rpc.buff_size - h2c_header.data_offset;
	h2c_header.message_length = homa_rpc.buff_size;

	h2c_header_o.write_nb(h2c_header);
    }


    // TODO get rid of dbuff id in the queue implementation
    srpt_queue_entry_t dbuff_notif;
    if (dbuff_notif_i.read_nb(dbuff_notif)) {
	// TODO sanitize IDs to make sure they refer to a valid entry??

	homa_rpc_t homa_rpc = (IS_CLIENT(h2c_header.id)) ? client_rpcs[h2c_header.local_id] : server_rpcs[h2c_header.local_id];

	ap_uint<32> dbuff_offset = dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED);

	// ap_uint<32> dbuffered = dbuff_in.msg_len - MIN((ap_uint<32>) (dbuff_in.msg_addr + (ap_uint<32>) DBUFF_CHUNK_SIZE), dbuff_in.msg_len);
	dbuff_notif(SRPT_QUEUE_ENTRY_DBUFF_ID) = homa_rpc.h2c_buff_id;
	dbuff_notif(SRPT_QUEUE_ENTRY_PRIORITY) = SRPT_DBUFF_UPDATE;

	if (dbuff_offset + DBUFF_CHUNK_SIZE < homa_rpc.buff_size) {
	    dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED) = homa_rpc.buff_size - dbuff_offset + DBUFF_CHUNK_SIZE;
	} else {
	    dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
	}

	dbuff_notif_o.write(dbuff_notif);
    }

    /* write paths */
    header_t c2h_header;
    if (c2h_header_i.read_nb(c2h_header)) {

	/* If we are the client then the entry already exists inside
	 * rpc_client. If we are the server then the entry needs to be
	 * created inside rpc_server if this is the first packet received
	 * for a message. If it is not the first message then the entry
	 * already exists in server_rpcs.
	 */
	if (!IS_CLIENT(c2h_header.id)) {

	    if ((c2h_header.packetmap & PMAP_INIT) == PMAP_INIT) {
		homa_rpc_t homa_rpc;

		homa_rpc.saddr = c2h_header.saddr;
		homa_rpc.daddr = c2h_header.daddr;
		homa_rpc.dport = c2h_header.dport;
		homa_rpc.sport = c2h_header.sport;
		homa_rpc.id    = c2h_header.id;

		server_rpcs[c2h_header.local_id] = homa_rpc;
	    } else {
		homa_rpc_t homa_rpc = server_rpcs[c2h_header.local_id];

		c2h_header.id = homa_rpc.id;
	    }
	}

	switch (c2h_header.type) {
	    case DATA: {
		// Will this message ever need grants?
		if (c2h_header.message_length > c2h_header.incoming) {

		    // TODO convert this to srpt_queue_entry
		    srpt_grant_new_t grant_in;
		    grant_in(SRPT_GRANT_NEW_MSG_LEN) = c2h_header.message_length;
		    grant_in(SRPT_GRANT_NEW_RPC_ID)  = c2h_header.local_id;
		    grant_in(SRPT_GRANT_NEW_PEER_ID) = c2h_header.peer_id;
		    grant_in(SRPT_GRANT_NEW_PMAP)    = c2h_header.packetmap;
		    
		    // Notify the grant queue of this receipt
		    grant_srpt_o.write(grant_in); 
		} 
		
		// Instruct the data buffer where to write this message's data
		c2h_header_o.write(c2h_header);

		// Log the receipt of a data packet
		c2h_pkt_log_o.write(LOG_DATA_OUT);
		
		break;
	    }
		
	    case GRANT: {
		srpt_queue_entry_t srpt_grant_notif;
		srpt_grant_notif(SRPT_QUEUE_ENTRY_RPC_ID)   = c2h_header.local_id;
		srpt_grant_notif(SRPT_QUEUE_ENTRY_GRANTED)  = c2h_header.grant_offset;
		srpt_grant_notif(SRPT_QUEUE_ENTRY_PRIORITY) = SRPT_GRANT_UPDATE;

		// Instruct the SRPT data of the new grant
		data_srpt_o.write(srpt_grant_notif);

		// Log the receipt of the grant packet
		c2h_pkt_log_o.write(LOG_GRANT_OUT);
		
		break;
	    }
	} 
    } 

    homa_rpc_t sendmsg;
    if (sendmsg_i.read_nb(sendmsg)) {

	if (IS_CLIENT(sendmsg.id)) {
	    client_rpcs[sendmsg.local_id] = sendmsg;
	}

	// TODO so much type shuffling here.... Some sort of ctor?

      	srpt_queue_entry_t srpt_data_in;
	srpt_data_in(SRPT_QUEUE_ENTRY_RPC_ID)    = sendmsg.local_id;
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = sendmsg.h2c_buff_id;
	srpt_data_in(SRPT_QUEUE_ENTRY_REMAINING) = sendmsg.buff_size;
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFFERED) = sendmsg.buff_size;
	// TODO would be cleaner if we could just set this to RTT_BYTES
	srpt_data_in(SRPT_QUEUE_ENTRY_GRANTED)   = sendmsg.buff_size - ((((ap_uint<32>) RTT_BYTES) > sendmsg.buff_size)
									     ? sendmsg.buff_size : ((ap_uint<32>) RTT_BYTES));
	srpt_data_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;

	// Insert this entry into the SRPT data queue
	data_queue_o.write(srpt_data_in);

	srpt_queue_entry_t srpt_fetch_in = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_RPC_ID)    = sendmsg.local_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = sendmsg.h2c_buff_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_REMAINING) = sendmsg.buff_size;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_GRANTED)   = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;

	// Insert this entry into the SRPT fetch queue
	fetch_queue_o.write(srpt_fetch_in);
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

    header_t header_in;

    if (header_in_i.read_nb(header_in)) {
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
	header_in_o.write(header_in);
    }
}
