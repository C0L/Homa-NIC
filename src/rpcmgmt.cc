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
    hls::stream<srpt_queue_entry_t> & dbuff_notif_i,
    hls::stream<srpt_queue_entry_t> & dbuff_notif_o,
    hls::stream<port_to_phys_t> & c2h_port_to_phys_i,
    hls::stream<dma_w_req_t> & dma_w_req_i,
    hls::stream<dma_w_req_t> & dma_w_req_o,
    hls::stream<port_to_phys_t> & h2c_port_to_phys_i,
    hls::stream<srpt_queue_entry_t> & dma_r_req_i,
    hls::stream<dma_r_req_t> & dma_r_req_o,
    hls::stream<ap_uint<8>> & h2c_pkt_log_o,
    hls::stream<ap_uint<8>> & c2h_pkt_log_o
    ) {
// #pragma HLS latency max=6

    // TODO start thrrowing some of this stuff in functions

    /* TODO: this is potentially not densely utilized because of paritiy of
     * rpc IDs for client vs server.
     */
    /* Long-lived RPC data */
    static homa_rpc_t client_rpcs[MAX_RPCS];
    static homa_rpc_t server_rpcs[MAX_RPCS];

#pragma HLS bind_storage variable=client_rpcs type=RAM_1WNR
#pragma HLS bind_storage variable=server_rpcs type=RAM_1WNR

    /* Resource management */
    static stack_t<local_id_t, MAX_RPCS> client_ids(STACK_EVEN);
    static stack_t<local_id_t, NUM_EGRESS_BUFFS> msg_cache_ids(STACK_ALL);

    // Unique local RPC ID assigned when this core is the server
    static stack_t<local_id_t, MAX_RPCS> server_ids(STACK_ODD);
    // Unique local Peer IDs
    static stack_t<peer_id_t, MAX_PEERS> peer_ids(STACK_ALL);

    /* Port/RPC mappings for DMA */
    static host_addr_t c2h_port_to_phys[MAX_PORTS]; // Port -> large c2h buffer space 
    static msg_addr_t  c2h_rpc_to_offset[MAX_RPCS]; // RPC -> offset in that buffer space
    static ap_uint<7>  c2h_buff_ids[MAX_PORTS];     // Next availible offset in buffer space

    static host_addr_t h2c_port_to_phys[MAX_PORTS]; // Port -> large h2c buffer space
    static msg_addr_t  h2c_rpc_to_offset[MAX_RPCS]; // RPC -> offset in that buffer space

    /* Maps */
    // hash(dest addr, sender ID, dest port) -> rpc ID
    static hashmap_t<rpc_hashpack_t, local_id_t, RPC_TABLE_SIZE, RPC_TABLE_INDEX, RPC_HP_SIZE, 8> rpc_hashmap;

    // hash(dest addr) -> peer ID
    static hashmap_t<peer_hashpack_t, peer_id_t, PEER_TABLE_SIZE, PEER_TABLE_INDEX, PEER_HP_SIZE, 8> peer_hashmap;

//#pragma HLS dependence variable=rpc_hashmap inter WAR false
//#pragma HLS dependence variable=rpc_hashmap inter RAW false
//#pragma HLS dependence variable=peer_hashmap inter WAR false
//#pragma HLS dependence variable=peer_hashmap inter RAW false
//
//#pragma HLS dependence variable=rpc_hashmap intra WAR false
//#pragma HLS dependence variable=rpc_hashmap intra RAW false
//#pragma HLS dependence variable=peer_hashmap intra WAR false
//#pragma HLS dependence variable=peer_hashmap intra RAW false

    // TODO this core handles long term state and resource mgmt

#pragma HLS pipeline II=1

    // TODO is this potentially risky with OOO small packets
    // Can maybe do a small sort of rebuffering if needed?
#pragma HLS dependence variable=client_rpcs inter WAR false
#pragma HLS dependence variable=client_rpcs inter RAW false
#pragma HLS dependence variable=server_rpcs inter WAR false
#pragma HLS dependence variable=server_rpcs inter RAW false

    // TODO is this a guarantee??
#pragma HLS dependence variable=c2h_buff_ids inter RAW false
#pragma HLS dependence variable=c2h_buff_ids inter WAR false

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

    srpt_queue_entry_t dma_fetch;
    if (dma_r_req_i.read_nb(dma_fetch)) {
	local_id_t id = dma_fetch(SRPT_QUEUE_ENTRY_RPC_ID);
	homa_rpc_t homa_rpc = (IS_CLIENT(id)) ? client_rpcs[id] : server_rpcs[id];

	// Get the physical address of this ports entire buffer

	msg_addr_t msg_addr   = h2c_rpc_to_offset[id];
	host_addr_t phys_addr = h2c_port_to_phys[homa_rpc.sport];

	dma_r_req_t dma_r_req;
	dma_r_req(SRPT_QUEUE_ENTRY_SIZE-1, 0) = dma_fetch;
	dma_r_req(DMA_R_REQ_MSG_LEN)          = homa_rpc.buff_size;

	msg_addr >>= 1;
	dma_r_req(DMA_R_REQ_HOST_ADDR) = (homa_rpc.buff_size - dma_fetch(SRPT_QUEUE_ENTRY_REMAINING)) + (phys_addr + msg_addr);

	dma_r_req_o.write(dma_r_req);
    }


    dma_w_req_t dma_w_req;
    if (dma_w_req_i.read_nb(dma_w_req)) {
	// TODO should error log here if the entry does not exist

	// Get the physical address of this ports entire buffer
	host_addr_t phys_addr = c2h_port_to_phys[dma_w_req.port];
	msg_addr_t msg_addr   = c2h_rpc_to_offset[dma_w_req.rpc_id];

	msg_addr >>= 1;
	dma_w_req.offset += phys_addr + msg_addr;

	dma_w_req_o.write(dma_w_req);
    }

    port_to_phys_t new_h2c_port_to_phys;
    if (h2c_port_to_phys_i.read_nb(new_h2c_port_to_phys)) {
	h2c_port_to_phys[new_h2c_port_to_phys(PORT_TO_PHYS_PORT)] = new_h2c_port_to_phys(PORT_TO_PHYS_ADDR);
    }

    port_to_phys_t new_c2h_port_to_phys;
    if (c2h_port_to_phys_i.read_nb(new_c2h_port_to_phys)) {
	c2h_port_to_phys[new_c2h_port_to_phys(PORT_TO_PHYS_PORT)] = new_c2h_port_to_phys(PORT_TO_PHYS_ADDR);
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
	    rpc_hashpack_t rpc_query = {c2h_header.saddr, c2h_header.id, c2h_header.sport, 0};

	    c2h_header.local_id = rpc_hashmap.search(rpc_query);
 
            // If the rpc is not registered, generate a new RPC ID and register it 
 	    if (c2h_header.local_id == 0) {
 		c2h_header.local_id = server_ids.pop();
		entry_t<rpc_hashpack_t, local_id_t> rpc_entry = {rpc_query, c2h_header.local_id};
		rpc_hashmap.insert(rpc_entry);

		// Store data associated with this RPC
		homa_rpc_t homa_rpc;
		homa_rpc.saddr = c2h_header.saddr;
		homa_rpc.daddr = c2h_header.daddr;
		homa_rpc.dport = c2h_header.dport;
		homa_rpc.sport = c2h_header.sport;
		homa_rpc.id    = c2h_header.id;

		server_rpcs[c2h_header.local_id] = homa_rpc;

		msg_addr_t msg_addr = ((c2h_buff_ids[c2h_header.sport] * HOMA_MAX_MESSAGE_LENGTH) << 1);
		c2h_rpc_to_offset[c2h_header.local_id] = msg_addr;
		c2h_buff_ids[c2h_header.sport]++;
 	    }

	    // TODO should log errors here for hash table misses

	    /* Perform the peer ID lookup regardless */
	    peer_hashpack_t peer_query = {c2h_header.saddr};
	    c2h_header.peer_id          = peer_hashmap.search(peer_query);
 	
	    // If the peer is not registered, generate new ID and register it
	    if (c2h_header.peer_id == 0) {
	    	c2h_header.peer_id = peer_ids.pop();

		entry_t<peer_hashpack_t, peer_id_t> peer_entry = {peer_query, c2h_header.peer_id};
		peer_hashmap.insert(peer_entry);
 	    
	    	// peer_entry = {peer_query, c2h_header.peer_id};
	    	// peer_hashmap.insert(peer_entry);
	    }
	} else {
	    homa_rpc_t homa_rpc = client_rpcs[h2c_header.id];

	    // If we are the client then a network ID is a local ID
	    c2h_header.local_id = c2h_header.id;
	    c2h_header.peer_id  = homa_rpc.peer_id;
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

		    // TODO the header has not gone through the packetmap yet here
		    // TODO this can be determined by the hashmap instead of packetmap
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
    } else {
	rpc_hashmap.process();
    }

    /* New Request or Response */
    msghdr_send_t msghdr_send;
    if (msghdr_send_i.read_nb(msghdr_send)) {
	homa_rpc_t new_rpc;
	new_rpc.saddr        = msghdr_send.data(MSGHDR_SADDR);
	new_rpc.daddr        = msghdr_send.data(MSGHDR_DADDR);
	new_rpc.sport        = msghdr_send.data(MSGHDR_SPORT);
	new_rpc.dport        = msghdr_send.data(MSGHDR_DPORT);
	new_rpc.buff_addr    = msghdr_send.data(MSGHDR_BUFF_ADDR);
	new_rpc.buff_size    = msghdr_send.data(MSGHDR_BUFF_SIZE);
	new_rpc.cc           = msghdr_send.data(MSGHDR_SEND_CC);
	new_rpc.h2c_buff_id  = msg_cache_ids.pop();

	local_id_t id = client_ids.pop();
 
	/* If the caller provided an ID of 0 this is a request message and we
	 * need to generate a new local ID. Otherwise, this is a response
	 * message and the ID is already valid in homa_rpc buffer
	 */
	if (msghdr_send.data(MSGHDR_SEND_ID) == 0) {
	    // Generate a new local ID, and set the RPC ID to be that
	    new_rpc.id = id;
	    msghdr_send.data(MSGHDR_SEND_ID) = id;
	}
	
	msghdr_send.last = 1;
	msghdr_send_o.write(msghdr_send);
	
	if (IS_CLIENT(new_rpc.id)) {
	    client_rpcs[id] = new_rpc;
	}

	h2c_rpc_to_offset[id] = (new_rpc.buff_addr << 1) | 1;
	
	// TODO so much type shuffling here.... 
    	srpt_queue_entry_t srpt_data_in;
	srpt_data_in(SRPT_QUEUE_ENTRY_RPC_ID)    = id;
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = new_rpc.h2c_buff_id;
	srpt_data_in(SRPT_QUEUE_ENTRY_REMAINING) = new_rpc.buff_size;
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFFERED) = new_rpc.buff_size;
	// TODO would be cleaner if we could just set this to RTT_BYTES
	srpt_data_in(SRPT_QUEUE_ENTRY_GRANTED)   = new_rpc.buff_size - ((((ap_uint<32>) RTT_BYTES) > new_rpc.buff_size)
									? new_rpc.buff_size : ((ap_uint<32>) RTT_BYTES));
	srpt_data_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;
	
	// Insert this entry into the SRPT data queue
	data_queue_o.write(srpt_data_in);
	
	srpt_queue_entry_t srpt_fetch_in = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_RPC_ID)    = id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = new_rpc.h2c_buff_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_REMAINING) = new_rpc.buff_size;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_GRANTED)   = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;
	
	// Insert this entry into the SRPT fetch queue
	fetch_queue_o.write(srpt_fetch_in);
    }
}
