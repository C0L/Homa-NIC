#include "rpcmgmt.hh"
#include "hashmap.hh"

using namespace hls;

void c2h_header_hashmap(
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<entry_t<rpc_hashpack_t, local_id_t>> & new_rpcmap_entry,
    hls::stream<entry_t<peer_hashpack_t, peer_id_t>> & new_peermap_entry
    ) {

    static hashmap_t<rpc_hashpack_t, local_id_t> rpc_hashmap;
    static hashmap_t<peer_hashpack_t, peer_id_t> peer_hashmap;

#pragma HLS pipeline II=1

    header_t c2h_header;
    if (c2h_header_i.read_nb(c2h_header)) {
	/* Perform the peer ID lookup regardless */
	peer_hashpack_t peer_query = {c2h_header.saddr};
	c2h_header.peer_id         = peer_hashmap.search(peer_query);

	entry_t<peer_hashpack_t, peer_id_t> peer_ins;
	if (!IS_CLIENT(c2h_header.id)) {
	    //rpc_hashpack_t query = {c2h_header.saddr, c2h_header.id, c2h_header.sport, 0};

	    //c2h_header.local_id = rpc_hashmap.search(query);
	} else {
	    c2h_header.local_id = c2h_header.id;
	}

	c2h_header_o.write(c2h_header);
    }

    static entry_t<rpc_hashpack_t, local_id_t> rpc_ins;
    if (new_rpcmap_entry.read_nb(rpc_ins)) {
	rpc_hashmap.insert(rpc_ins);
    }

    static entry_t<peer_hashpack_t, local_id_t> peer_ins;
    if (new_peermap_entry.read_nb(peer_ins)) {
	peer_hashmap.insert(peer_ins);
    }
}

void c2h_header_cam(
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<entry_t<rpc_hashpack_t, local_id_t>> & new_rpcmap_entry,
    hls::stream<entry_t<peer_hashpack_t, peer_id_t>> & new_peermap_entry,
    hls::stream<local_id_t> & new_server,
    hls::stream<peer_id_t> & new_peer
    ) {

    /* hash(dest addr, sender ID, dest port) -> rpc ID */
    static cam_t<rpc_hashpack_t, local_id_t, 16, 4> rpc_cam;

    /* hash(dest addr) -> peer ID */
    static cam_t<peer_hashpack_t, peer_id_t, 16, 4> peer_cam;

#pragma HLS pipeline II=1

    header_t c2h_header;
    if (c2h_header_i.read_nb(c2h_header)) {
	if (c2h_header.peer_id == 0) {

	    /* Perform the peer ID lookup regardless */
	    peer_hashpack_t peer_query = {c2h_header.saddr};
	    c2h_header.peer_id          = peer_cam.search(peer_query);
	    
	    // If the peer is not registered, generate new ID and register it
	    if (c2h_header.peer_id == 0) {
		c2h_header.peer_id = new_peer.read();

		entry_t<peer_hashpack_t, local_id_t> new_entry = {peer_query, c2h_header.peer_id};

		peer_cam.insert(new_entry);
		new_peermap_entry.write(new_entry);
	    }
	}

	if (c2h_header.local_id == 0) {
	    if (!IS_CLIENT(c2h_header.id)) {
		/* If we are the client then the entry already exists inside
		 * rpc_client. If we are the server then the entry needs to be
		 * created inside rpc_server if this is the first packet received
		 * for a message. If it is not the first message then the entry
		 * already exists in server_rpcs.
		 */
		rpc_hashpack_t rpc_query = {c2h_header.saddr, c2h_header.id, c2h_header.sport, 0};

		c2h_header.local_id = rpc_cam.search(rpc_query);

		if (c2h_header.local_id == 0) {
		    c2h_header.local_id = new_server.read();

		    entry_t<rpc_hashpack_t, local_id_t> new_entry = {rpc_query, c2h_header.local_id};

		    // If the rpc is not registered, generate a new RPC ID and register it 
		    rpc_cam.insert(new_entry);
		    new_rpcmap_entry.write(new_entry);
		} 
	    }
	}

	c2h_header_o.write(c2h_header);
    }
}

/* Maintain long-lived state associated with RPCs/NIC, including:
 *   - Memory management: client/server IDs, peer IDs, dbuff IDs
 *   - Userspace mappings
 *   - RPC setup info: ip addrs, ports, msg size, location, ....
 */
void rpc_state(
    hls::stream<homa_rpc_t> & new_rpc_i,
    hls::stream<srpt_queue_entry_t> & data_queue_o,
    hls::stream<srpt_queue_entry_t> & fetch_queue_o,
    hls::stream<srpt_grant_new_t> & grant_queue_o,
    hls::stream<header_t> & h2c_header_i,
    hls::stream<header_t> & h2c_header_o,
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<srpt_queue_entry_t> & data_srpt_o,
    hls::stream<srpt_queue_entry_t> & dbuff_notif_i,
    hls::stream<srpt_queue_entry_t> & dbuff_notif_o,
    hls::stream<port_to_phys_t> & c2h_port_to_msgbuff_i,
    hls::stream<dma_w_req_t> & dma_w_req_i,
    hls::stream<dma_w_req_t> & dma_w_req_o,
    hls::stream<port_to_phys_t> & h2c_port_to_msgbuff_i,
    hls::stream<srpt_queue_entry_t> & dma_r_req_i,
    hls::stream<dma_r_req_t> & dma_r_req_o,
    hls::stream<dbuff_id_t> & free_dbuff_i,
    hls::stream<dbuff_id_t> & new_dbuff_o,
    hls::stream<local_id_t> & new_client_o,
    hls::stream<local_id_t> & new_server_o,
    hls::stream<local_id_t> & new_peer_o,
    hls::stream<ap_uint<8>> & h2c_pkt_log_o,
    hls::stream<ap_uint<8>> & c2h_pkt_log_o
    ) {

    /* Long-lived RPC data */
    static homa_rpc_t send_rpcs[MAX_RPCS];
    static homa_rpc_t recv_rpcs[MAX_RPCS];

#pragma HLS bind_storage variable=send_rpcs type=RAM_1WNR
#pragma HLS bind_storage variable=recv_rpcs type=RAM_1WNR

    /* Port/RPC to user databuffer mapping DMA */
    static host_addr_t c2h_port_to_msgbuff[MAX_PORTS];  // Port -> large c2h buffer space 
    static msg_addr_t  c2h_rpc_to_offset[MAX_RPCS];     // RPC -> offset in that buffer space
    static ap_uint<7>  c2h_buff_ids[MAX_PORTS];         // Next availible offset in buffer space

    /* Unique local RPC ID assigned when this core is the client */
    static stack_t<local_id_t, MAX_RPCS> client_ids(STACK_EVEN);

    /* Unique local databuffer IDs assigned for outgoing data */
    static stack_t<local_id_t, NUM_EGRESS_BUFFS> msg_cache_ids(STACK_ALL);

    /* Unique local RPC ID assigned when this core is the server */
    static stack_t<local_id_t, MAX_RPCS> server_ids(STACK_ODD);

    /* Unique Peer IDs */
    static stack_t<peer_id_t, MAX_PEERS> peer_ids(STACK_ALL);

    static host_addr_t h2c_port_to_msgbuff[MAX_PORTS];  // Port -> large h2c buffer space
    static msg_addr_t  h2c_rpc_to_offset[MAX_RPCS];     // RPC -> offset in that buffer space

    /* Port to metadata mapping DMA */
    static host_addr_t c2h_port_to_metadata[MAX_PORTS]; // Port -> metadata buffer

    // TODO is this potentially risky with OOO small packets
    // Can maybe do a small sort of rebuffering if needed?
#pragma HLS dependence variable=send_rpcs inter WAR false
#pragma HLS dependence variable=send_rpcs inter RAW false
#pragma HLS dependence variable=recv_rpcs inter WAR false
#pragma HLS dependence variable=recv_rpcs inter RAW false

    // TODO is this a guarantee??
#pragma HLS dependence variable=c2h_buff_ids inter RAW false
#pragma HLS dependence variable=c2h_buff_ids inter WAR false

#pragma HLS pipeline II=1

    static dbuff_id_t new_dbuff = 0;
    static dbuff_id_t free_dbuff = 0;
    if (new_dbuff == 0 && !msg_cache_ids.empty()) {
	new_dbuff = msg_cache_ids.pop();
    } else if (free_dbuff_i.read_nb(free_dbuff)) {
	msg_cache_ids.push(free_dbuff);
    } else if (new_dbuff_o.write_nb(new_dbuff)) {
	new_dbuff = 0;
    } 

    static local_id_t new_client = 0;
    if (new_client == 0 && !client_ids.empty()) {
	new_client = client_ids.pop();
    } else if (new_client_o.write_nb(new_client)) {
	new_client = 0;
    }

    static local_id_t new_server = 0;
    if (new_server == 0 && !server_ids.empty()) {
	new_server = server_ids.pop();
    } else if (new_server_o.write_nb(new_server)) {
	new_server = 0;
    }

    static local_id_t new_peer = 0;
    if (new_peer == 0 && !peer_ids.empty()) {
	new_peer = peer_ids.pop();
    } else if (new_peer_o.write_nb(new_peer)) {
	new_peer = 0;
    }

    port_to_phys_t new_h2c_port_to_msgbuff;
    if (h2c_port_to_msgbuff_i.read_nb(new_h2c_port_to_msgbuff)) {
	h2c_port_to_msgbuff[new_h2c_port_to_msgbuff(PORT_TO_PHYS_PORT)] = new_h2c_port_to_msgbuff(PORT_TO_PHYS_ADDR);
    }

    port_to_phys_t new_c2h_port_to_msgbuff;
    if (c2h_port_to_msgbuff_i.read_nb(new_c2h_port_to_msgbuff)) {
	c2h_port_to_msgbuff[new_c2h_port_to_msgbuff(PORT_TO_PHYS_PORT)] = new_c2h_port_to_msgbuff(PORT_TO_PHYS_ADDR);
    }

    homa_rpc_t rpc;
    if (new_rpc_i.read_nb(rpc)) {
	if (rpc.id == 0) {
	    rpc.id = recv_rpcs[rpc.local_id].id;
	}

	send_rpcs[rpc.local_id]   = rpc;
	h2c_rpc_to_offset[rpc.id] = (rpc.buff_addr << 1) | 1;
	
	// TODO so much type shuffling here.... 
    	srpt_queue_entry_t srpt_data_in = 0;
	srpt_data_in(SRPT_QUEUE_ENTRY_RPC_ID)    = rpc.local_id;
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = rpc.h2c_buff_id;
	srpt_data_in(SRPT_QUEUE_ENTRY_REMAINING) = rpc.buff_size;
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFFERED) = rpc.buff_size;
	// TODO would be cleaner if we could just set this to RTT_BYTES
	srpt_data_in(SRPT_QUEUE_ENTRY_GRANTED)   = rpc.buff_size - ((((ap_uint<32>) RTT_BYTES) > rpc.buff_size)
								    ? rpc.buff_size : ((ap_uint<32>) RTT_BYTES));
	srpt_data_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;

	// Insert this entry into the SRPT data queue
	data_queue_o.write(srpt_data_in);
	
	srpt_queue_entry_t srpt_fetch_in = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_RPC_ID)    = rpc.local_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = rpc.h2c_buff_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_REMAINING) = rpc.buff_size;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_GRANTED)   = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;
	
	// Insert this entry into the SRPT fetch queue
	fetch_queue_o.write(srpt_fetch_in);
    }


    srpt_queue_entry_t dbuff_notif;
    if (dbuff_notif_i.read_nb(dbuff_notif)) {
	local_id_t id = dbuff_notif(SRPT_QUEUE_ENTRY_RPC_ID);

	homa_rpc_t homa_rpc = send_rpcs[id];

	ap_uint<32> dbuff_offset = dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED);

	dbuff_notif(SRPT_QUEUE_ENTRY_DBUFF_ID) = homa_rpc.h2c_buff_id;
	dbuff_notif(SRPT_QUEUE_ENTRY_PRIORITY) = SRPT_DBUFF_UPDATE;

	if (dbuff_offset + DBUFF_CHUNK_SIZE < homa_rpc.buff_size) {
	    dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED) = homa_rpc.buff_size - dbuff_offset - DBUFF_CHUNK_SIZE;
	} else {
	    dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
	}

	// TODO these notifs need to also go to the fetch core?

	dbuff_notif_o.write(dbuff_notif);
    }

    dma_w_req_t dma_w_req;
    if (dma_w_req_i.read_nb(dma_w_req)) {
	// TODO should error log here if the entry does not exist

	// Get the physical address of this ports entire buffer
	host_addr_t phys_addr = c2h_port_to_msgbuff[dma_w_req.port];
	msg_addr_t msg_addr   = c2h_rpc_to_offset[dma_w_req.rpc_id];

	msg_addr >>= 1;

	dma_w_req.offset += phys_addr + msg_addr;

	dma_w_req_o.write(dma_w_req);
    }

    srpt_queue_entry_t dma_fetch;
    if (dma_r_req_i.read_nb(dma_fetch)) {
	local_id_t id = dma_fetch(SRPT_QUEUE_ENTRY_RPC_ID);
	homa_rpc_t homa_rpc = send_rpcs[id];

	// Get the physical address of this ports entire buffer

	msg_addr_t msg_addr   = h2c_rpc_to_offset[id];
	host_addr_t phys_addr = h2c_port_to_msgbuff[homa_rpc.sport];

	dma_r_req_t dma_r_req;
	dma_r_req(SRPT_QUEUE_ENTRY_SIZE-1, 0) = dma_fetch;
	dma_r_req(DMA_R_REQ_MSG_LEN)          = homa_rpc.buff_size;

	msg_addr >>= 1;
	dma_r_req(DMA_R_REQ_HOST_ADDR) = (homa_rpc.buff_size - dma_fetch(SRPT_QUEUE_ENTRY_REMAINING)) + (phys_addr + msg_addr);

	dma_r_req_o.write(dma_r_req);
    }

    header_t h2c_header;
    if (h2c_header_i.read_nb(h2c_header)) {
	
	homa_rpc_t homa_rpc;
	switch(h2c_header.type) {
	    case DATA:
		homa_rpc = send_rpcs[h2c_header.local_id];

		h2c_header.daddr = homa_rpc.daddr;
		h2c_header.dport = homa_rpc.dport;
		h2c_header.saddr = homa_rpc.saddr;
		h2c_header.sport = homa_rpc.sport;
		h2c_header.id    = homa_rpc.id;
		break;
	    case GRANT:
		homa_rpc = recv_rpcs[h2c_header.local_id];

		h2c_header.daddr = homa_rpc.saddr;
		h2c_header.dport = homa_rpc.sport;
		h2c_header.saddr = homa_rpc.daddr;
		h2c_header.sport = homa_rpc.dport;
		h2c_header.id    = homa_rpc.id;
		break;
	}

	// Log the packet type we are sending
	h2c_pkt_log_o.write((h2c_header.type == DATA) ? LOG_DATA_OUT : LOG_GRANT_OUT);

	// Get the location of buffered data and prepare for packet construction
	h2c_header.h2c_buff_id    = homa_rpc.h2c_buff_id;
	h2c_header.incoming       = homa_rpc.buff_size - h2c_header.incoming;
	h2c_header.data_offset    = homa_rpc.buff_size - h2c_header.data_offset;
	h2c_header.message_length = homa_rpc.buff_size;

	h2c_header_o.write_nb(h2c_header);
    }

    header_t c2h_header;
    if (c2h_header_i.read_nb(c2h_header)) {
	if ((c2h_header.packetmap & PMAP_INIT) == PMAP_INIT) {
	    // Store data associated with this RPC
	    homa_rpc_t homa_rpc;
	    homa_rpc.saddr   = c2h_header.saddr;
	    homa_rpc.daddr   = c2h_header.daddr;
	    homa_rpc.dport   = c2h_header.dport;
	    homa_rpc.sport   = c2h_header.sport;
	    homa_rpc.id      = c2h_header.id;
	    homa_rpc.peer_id = c2h_header.peer_id;

	    recv_rpcs[c2h_header.local_id] = homa_rpc;

	    // msg_addr_t msg_addr = ((c2h_buff_ids[c2h_header.sport] * HOMA_MAX_MESSAGE_LENGTH) << 1);
	    msg_addr_t msg_addr = (c2h_buff_ids[c2h_header.sport]);
	    c2h_rpc_to_offset[c2h_header.local_id] = msg_addr;
	    c2h_buff_ids[c2h_header.sport]++;
	}
	
	// TODO should log errors here for hash table misses
	switch (c2h_header.type) {
	    case DATA: {
		if (c2h_header.message_length > c2h_header.incoming) {
		    srpt_grant_new_t grant_in;


		    grant_in(SRPT_GRANT_NEW_MSG_LEN) = c2h_header.message_length;
		    grant_in(SRPT_GRANT_NEW_RPC_ID)  = c2h_header.local_id;
		    grant_in(SRPT_GRANT_NEW_PEER_ID) = c2h_header.peer_id;
		    
		    grant_in(SRPT_GRANT_NEW_PMAP)    = c2h_header.packetmap;
		    
		    // Notify the grant queue of this receipt
		    grant_queue_o.write(grant_in); 
		}

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

    // TODO break this up
    // c2h_header_proc(
    // 	send_rpcs,
    // 	recv_rpcs,
    // 	c2h_buff_ids,
    // 	c2h_rpc_to_offset,
    // 	c2h_header_i,
    // 	c2h_header_o,
    // 	pm_c2h_header_i,
    // 	pm_c2h_header_o,
    // 	grant_queue_o,
    // 	data_srpt_o,
    // 	c2h_pkt_log_o
    // );
}
