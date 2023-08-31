#include "rpcmgmt.hh"
#include "hashmap.hh"

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
void rpc_state(hls::stream<onboard_send_t> & onboard_send_i,
	       hls::stream<srpt_sendmsg_t> & onboard_send_o,
	       hls::stream<header_t> & header_out_i, 
	       hls::stream<header_t> & header_out_o,
	       hls::stream<header_t> & header_in_i,
	       hls::stream<header_t> & header_in_dbuff_o,
	       hls::stream<srpt_grant_in_t> & grant_srpt_o,
	       hls::stream<srpt_grant_notif_t> & data_srpt_o) {

    // TODO should probably move
    static stack_t<local_id_t, MAX_RPCS> ingress_dma_stack(true);

    static homa_rpc_t rpcs[MAX_RPCS];

#pragma HLS pipeline II=1

#pragma HLS bind_storage variable=rpcs type=RAM_1WNR

#pragma HLS dependence variable=rpcs inter WAR false
#pragma HLS dependence variable=rpcs inter RAW false

    /* RO Paths */
    header_t header_out;
    if (header_out_i.read_nb(header_out)) {
	homa_rpc_t homa_rpc = rpcs[SEND_INDEX_FROM_RPC_ID(header_out.local_id)];

	header_out.daddr           = homa_rpc.daddr;
	header_out.dport           = homa_rpc.dport;
	header_out.saddr           = homa_rpc.saddr;
	header_out.sport           = homa_rpc.sport;
	header_out.id              = homa_rpc.id;

	// TODO will need to move
	header_out.egress_buff_id  = homa_rpc.egress_buff_id;

	header_out.incoming       = homa_rpc.iov_size - header_out.incoming;
	header_out.data_offset    = homa_rpc.iov_size - header_out.data_offset;
	header_out.message_length = homa_rpc.iov_size;

	header_out_o.write_nb(header_out);
    } 

    /* R/W Paths */
    header_t header_in;
    onboard_send_t onboard_send;
    if (header_in_i.read_nb(header_in)) {

	homa_rpc_t homa_rpc = rpcs[RECV_INDEX_FROM_RPC_ID(header_in.local_id)];

	switch (header_in.type) {
	    case DATA: {
		if ((header_in.packetmap & PMAP_INIT) == PMAP_INIT) {
		    homa_rpc.saddr = header_in.saddr;
		    homa_rpc.daddr = header_in.daddr;
		    homa_rpc.dport = header_in.dport;
		    homa_rpc.sport = header_in.sport;
		    homa_rpc.id    = header_in.id;

		    // TODO will need to move
		    homa_rpc.ingress_dma_id  = ingress_dma_stack.pop();
		    header_in.ingress_dma_id = homa_rpc.ingress_dma_id;
		} else {
		    header_in.id = homa_rpc.id;

		    // TODO will need to move
		    header_in.ingress_dma_id = homa_rpc.ingress_dma_id;
		}

		if (header_in.message_length > header_in.incoming) { 
		    srpt_grant_in_t grant_in;
		    grant_in(SRPT_GRANT_IN_OFFSET)  = header_in.data_offset;
		    grant_in(SRPT_GRANT_IN_MSG_LEN) = header_in.message_length;
		    grant_in(SRPT_GRANT_IN_RPC_ID)  = header_in.local_id;
		    grant_in(SRPT_GRANT_IN_PEER_ID) = header_in.peer_id;
		    grant_in(SRPT_GRANT_IN_PMAP)    = header_in.packetmap;

		    // Notify the grant queue of this receipt
		    grant_srpt_o.write(grant_in); 
		} 

		// Instruct the data buffer where to write this message's data
		header_in_dbuff_o.write(header_in); 

		break;
	    }

	    case GRANT: {
		srpt_grant_notif_t srpt_grant_notif;
		srpt_grant_notif(SRPT_GRANT_NOTIF_RPC_ID) = header_in.local_id;
		srpt_grant_notif(SRPT_GRANT_NOTIF_OFFSET) = header_in.grant_offset;
		data_srpt_o.write(srpt_grant_notif); 
		break;
	    }
	}

	rpcs[RECV_INDEX_FROM_RPC_ID(header_in.local_id)] = homa_rpc;
    } else if (onboard_send_i.read_nb(onboard_send)) {
	homa_rpc_t homa_rpc;
	homa_rpc.daddr           = onboard_send.daddr;
	homa_rpc.dport           = onboard_send.dport;
	homa_rpc.saddr           = onboard_send.saddr;
	homa_rpc.sport           = onboard_send.sport;
	homa_rpc.iov             = onboard_send.iov;
	homa_rpc.iov_size        = onboard_send.iov_size;
	homa_rpc.egress_buff_id  = onboard_send.egress_buff_id;
	homa_rpc.id              = onboard_send.id;

	rpcs[SEND_INDEX_FROM_RPC_ID(onboard_send.local_id)] = homa_rpc;

	srpt_sendmsg_t srpt_data_in;
	srpt_data_in(SRPT_SENDMSG_RPC_ID)   = onboard_send.local_id;
	srpt_data_in(SRPT_SENDMSG_MSG_LEN)  = onboard_send.iov_size;
	srpt_data_in(SRPT_SENDMSG_GRANTED)  = onboard_send.iov_size - ((((ap_uint<32>) RTT_BYTES) > onboard_send.iov_size)
	    ? onboard_send.iov_size : ((ap_uint<32>) RTT_BYTES));

	onboard_send_o.write(srpt_data_in);
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
    static stack_t<local_id_t, MAX_CLIENT_IDS> server_ids(true);
    // Unique local Peer IDs
    static stack_t<peer_id_t, MAX_PEERS> peer_ids(true);

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
		header_in.local_id = RECV_RPC_ID_FROM_INDEX(server_ids.pop());
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
	    header_in.peer_id = PEER_ID_FROM_INDEX(peer_ids.pop());
	    
	    entry_t<peer_hashpack_t, peer_id_t> peer_entry = {peer_query, header_in.peer_id};
	    peer_hashmap.insert(peer_entry);
	}

	header_in_o.write(header_in);
    }
}
