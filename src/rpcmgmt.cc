#include "rpcmgmt.hh"
#include "hashmap.hh"

using namespace hls;


void client_rpc_state(
    stream<sendmsg_t> & onboard_send_i,
    stream<srpt_data_new_t> & onboard_send_o,
    stream<header_t> & h2c_header_i,
    stream<header_t> & h2c_header_o,
    ) {

    static homa_rpc_t rpcs[MAX_RPCS];
 
#pragma HLS pipeline II=1

#pragma HLS bind_storage variable=rpcs type=RAM_1WNR

#pragma HLS dependence variable=rpcs inter WAR false
#pragma HLS dependence variable=rpcs inter RAW false

    /* Client sendmsg */
    if (!onboard_send_i.empty()) {
	onboard_send_t onboard_send = onboard_send_i.read();

	homa_rpc_t homa_rpc;
	homa_rpc.daddr        = onboard_send.daddr;
	homa_rpc.dport        = onboard_send.dport;
	homa_rpc.saddr        = onboard_send.saddr;
	homa_rpc.sport        = onboard_send.sport;
	homa_rpc.buff_size    = onboard_send.buff_size;
	homa_rpc.h2c_buff_id  = onboard_send.h2c_buff_id;
	homa_rpc.id           = onboard_send.id;

	rpcs[onboard_send.local_id]         = homa_rpc;

	srpt_data_new_t srpt_data_in;
	srpt_data_in(SRPT_DATA_NEW_RPC_ID)   = onboard_send.local_id;
	srpt_data_in(SRPT_DATA_NEW_MSG_LEN)  = onboard_send.buff_size;
	// TODO would be cleaner if we could just set this to RTT_BYTES
	srpt_data_in(SRPT_DATA_NEW_GRANTED)  = RTT_BYTES;
	//onboard_send.buff_size - ((((ap_uint<32>) RTT_BYTES) > onboard_send.buff_size)
	// ? onboard_send.buff_size : ((ap_uint<32>) RTT_BYTES));
	srpt_data_in(SRPT_DATA_NEW_DBUFF_ID)  = onboard_send.h2c_buff_id;

	onboard_send_o.write(srpt_data_in);
    }

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
    }

    header_t c2h_header;
    if (c2h_header_i.read_nb(c2h_header)) {

	homa_rpc_t homa_rpc = rpcs[c2h_header.local_id];

	switch (c2h_header.type) {
	case DATA: {
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

	    header_in_o.write(c2h_header); 

	    break;
	}

	case GRANT: {
	    srpt_data_grant_notif_t srpt_grant_notif;
	    srpt_grant_notif(SRPT_DATA_GRANT_NOTIF_RPC_ID)   = c2h_header.local_id;
	    srpt_grant_notif(SRPT_DATA_GRANT_NOTIF_MSG_ADDR) = c2h_header.grant_offset;
	    data_srpt_o.write(srpt_grant_notif); 
	    break;
	}
	}
    }
}

void client_rpc_state(
    stream<onboard_send_t> & onboard_send_i,
    stream<srpt_data_new_t> & onboard_send_o,
    stream<header_t> & c2h_header_i,
    stream<header_t> & c2h_header_o
    ) {

    static homa_rpc_t  rpcs[MAX_RPCS];

#pragma HLS pipeline II=1

#pragma HLS dependence variable=rpcs inter WAR false
#pragma HLS dependence variable=rpcs inter RAW false

    if (onboard_send_i.read_nb(onboard_send)) {
	// Update the rpc_to_offset if ever needed again
	// h2c_rpc_to_offset[onboard_send.id] = onboard_send.buff_addr;

	srpt_data_new_t srpt_data_in;
	srpt_data_in(SRPT_DATA_NEW_RPC_ID)   = onboard_send.local_id;
	srpt_data_in(SRPT_DATA_NEW_MSG_LEN)  = onboard_send.buff_size;
	// TODO would be cleaner if we could just set this to RTT_BYTES
	srpt_data_in(SRPT_DATA_NEW_GRANTED)  = onboard_send.buff_size - ((((ap_uint<32>) RTT_BYTES) > onboard_send.buff_size)
									 ? onboard_send.buff_size : ((ap_uint<32>) RTT_BYTES));
	srpt_data_in(SRPT_DATA_NEW_DBUFF_ID)  = onboard_send.h2c_buff_id;
	srpt_data_in(SRPT_DATA_NEW_HOST_ADDR) = 

	    onboard_send_o.write(srpt_data_in);
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
    }

    header_t c2h_header;
    if (c2h_header_i.read_nb(c2h_header)) {

	homa_rpc_t homa_rpc = rpcs[c2h_header.local_id];

	switch (c2h_header.type) {
	case DATA: {
	    if ((c2h_header.packetmap & PMAP_INIT) == PMAP_INIT) {
		homa_rpc.saddr = c2h_header.saddr;
		homa_rpc.daddr = c2h_header.daddr;
		homa_rpc.dport = c2h_header.dport;
		homa_rpc.sport = c2h_header.sport;
		homa_rpc.id    = c2h_header.id;

		// Get the physical address of this ports entire buffer
		host_addr_t phys_addr = c2h_port_to_phys[c2h_header.dport];
		msg_addr_t buff_offset = c2h_buff_ids[c2h_header.id] * HOMA_MAX_MESSAGE_LENGTH;

		c2h_rpc_to_offset[c2h_header.dport] = buff_offset;
		c2h_buff_ids[c2h_header.id]++;

		c2h_header.c2h_addr = phys_addr + buff_offset;
	    } else {
		host_addr_t phys_addr = c2h_port_to_phys[c2h_header.dport];
		msg_addr_t buff_offset = c2h_rpc_to_offset[homa_rpc.id];

		c2h_header.id = homa_rpc.id;

		c2h_header.c2h_addr = phys_addr + buff_offset;
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
	    header_in_o.write(c2h_header); 

	    break;
	}

	case GRANT: {
	    srpt_data_grant_notif_t srpt_grant_notif;
	    srpt_grant_notif(SRPT_DATA_GRANT_NOTIF_RPC_ID)   = c2h_header.local_id;
	    srpt_grant_notif(SRPT_DATA_GRANT_NOTIF_MSG_ADDR) = c2h_header.grant_offset;
	    data_srpt_o.write(srpt_grant_notif); 
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

void rpc_mgmt(hls::stream<onboard_send_t> & sendmsg__homa_sendmsg__rpc_state,
	      hls::stream<header_t> & header_out__egress_sel__rpc_state,
	      hls::stream<header_t> & header_out__rpc_state__pkt_builder,
	      hls::stream<header_t> & header_in__rpc_state__dbuff_ingress,
	      hls::stream<header_t> & header_in__chunk_ingress__rpc_map,
	      hls::stream<srpt_grant_send_t> & grant__srpt_grant__egress_sel,
	      hls::stream<srpt_data_dbuff_notif_t> & dbuff_notif__dbuff_egress__srpt_data,
	      hls::stream<srpt_data_send_t> & ready_data_pkt__srpt_data__egress_sel,
	      hls::stream<srpt_data_fetch_t> & dma_req__srpt_data__dma_read
	      ) {

    hls_thread_local hls::stream<srpt_data_new_t, STREAM_DEPTH> sendmsg__rpc_state__srpt_data;
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__packetmap__rpc_state;
    hls_thread_local hls::stream<srpt_grant_new_t, STREAM_DEPTH> grant__rpc_state__srpt_grant;
    hls_thread_local hls::stream<srpt_data_grant_notif_t, STREAM_DEPTH> header_in__rpc_state__srpt_data;
    hls_thread_local hls::stream<header_t, STREAM_DEPTH> header_in__rpc_map__packetmap;

    hls_thread_local hls::task rpc_state_task(
	rpc_state,
	sendmsg__homa_sendmsg__rpc_state,    // sendmsg_i
	sendmsg__rpc_state__srpt_data,       // sendmsg_o
	header_out__egress_sel__rpc_state,   // header_out_i
	header_out__rpc_state__pkt_builder,  // header_out_o
	header_in__packetmap__rpc_state,     // header_in_i
	header_in__rpc_state__dbuff_ingress, // header_in_dbuff_o
	grant__rpc_state__srpt_grant,        // grant_in_o
	header_in__rpc_state__srpt_data      // header_in_srpt_o
	);
 
    hls_thread_local hls::task id_map_task(
	id_map,
	header_in__chunk_ingress__rpc_map, // header_in_i
	header_in__rpc_map__packetmap      // header_in_o
	);

    hls_thread_local hls::task srpt_grant_pkts_task(
	srpt_grant_pkts,
	grant__rpc_state__srpt_grant, // grant_in_i
	grant__srpt_grant__egress_sel // grant_out_o
	);

    hls_thread_local hls::task srpt_data_pkts_task(
	srpt_data_pkts,
	sendmsg__rpc_state__srpt_data,         // sendmsg_i
	dbuff_notif__dbuff_egress__srpt_data,  // dbuff_notif_i
	ready_data_pkt__srpt_data__egress_sel, // data_pkt_o  // TODO tidy this name up
	header_in__rpc_state__srpt_data,       // header_in_i
	dma_req__srpt_data__dma_read           // 
	);

    hls_thread_local hls::task packetmap_task(
	packetmap, 
	header_in__rpc_map__packetmap,  // header_in
	header_in__packetmap__rpc_state // complete_messages
	);
}
