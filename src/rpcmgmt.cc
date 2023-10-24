#include "rpcmgmt.hh"
#include "hashmap.hh"

using namespace hls;

void h2c_header_proc(
    homa_rpc_t * send_rpcs,
    homa_rpc_t * recv_rpcs,
    hls::stream<header_t> & pkt_in,
    hls::stream<header_t> & pkt_out,
    hls::stream<ap_uint<8>> & log_out
    ) {
    
    header_t h2c_header;
    if (pkt_in.read_nb(h2c_header)) {
	
	homa_rpc_t homa_rpc;
	switch(h2c_header.type) {
	    case DATA:
		homa_rpc = send_rpcs[h2c_header.local_id];

		h2c_header.daddr = homa_rpc.daddr;
		h2c_header.dport = homa_rpc.dport;
		h2c_header.saddr = homa_rpc.saddr;
		h2c_header.sport = homa_rpc.sport;
		h2c_header.id    = homa_rpc.id;

		std::cerr << "data packet out local " << h2c_header.local_id << " net " << homa_rpc.id << std::endl;
		// std::cerr << "data packet out local " << h2c_header.saddr << " net " << homa_rpc.id << std::endl;
		break;
	    case GRANT:
		homa_rpc = recv_rpcs[h2c_header.local_id];

		h2c_header.daddr = homa_rpc.saddr;
		h2c_header.dport = homa_rpc.sport;
		h2c_header.saddr = homa_rpc.daddr;
		h2c_header.sport = homa_rpc.dport;
		h2c_header.id    = homa_rpc.id;

		std::cerr << "grant packet out local " << h2c_header.local_id << " net " << homa_rpc.id << std::endl;
		break;
	}


	// Log the packet type we are sending
	log_out.write((h2c_header.type == DATA) ? LOG_DATA_OUT : LOG_GRANT_OUT);

	// Get the location of buffered data and prepare for packet construction
	h2c_header.h2c_buff_id    = homa_rpc.h2c_buff_id;
	h2c_header.incoming       = homa_rpc.buff_size - h2c_header.incoming;
	h2c_header.data_offset    = homa_rpc.buff_size - h2c_header.data_offset;
	h2c_header.message_length = homa_rpc.buff_size;

	pkt_out.write_nb(h2c_header);
    }
}

/* Cases:
 * Is Server:
 *   - DATA Packet    : Map Network ID -> Local ID. Create entry/id in recv_rpcs on init
 *   - Control Packet : Map Network ID -> Local ID. Entry already in recv_rpcs
 * Is Client:
 *   - DATA Packet:   : Create entry/id in recv_rpcs on init
 *   - Control Packet : None.
 */
void c2h_header_proc(
    homa_rpc_t * send_rpcs,
    homa_rpc_t * recv_rpcs,
    ap_uint<7> * c2h_buff_ids,
    msg_addr_t  * c2h_rpc_to_offset,
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<header_t> & pm_c2h_header_i,
    hls::stream<header_t> & pm_c2h_header_o,
    hls::stream<srpt_grant_new_t> & grant_queue_o,
    hls::stream<srpt_queue_entry_t> & data_srpt_o,
    hls::stream<ap_uint<8>> & c2h_pkt_log_o
    ) {

    /* Unique local RPC ID assigned when this core is the server */
    static stack_t<local_id_t, MAX_RPCS> server_ids(STACK_ODD);

    /* Unique local Peer IDs */
    static stack_t<peer_id_t, MAX_PEERS> peer_ids(STACK_ALL);

    /* hash(dest addr, sender ID, dest port) -> rpc ID */
    static cam_t<rpc_hashpack_t, local_id_t, 16, 4> rpc_cam;

    /* hash(dest addr) -> peer ID */
    static cam_t<peer_hashpack_t, peer_id_t, 16, 4> peer_cam;

    /* post packet map */
    header_t pm_c2h_header;
    if (pm_c2h_header_i.read_nb(pm_c2h_header)) {
	if ((pm_c2h_header.packetmap & PMAP_INIT) == PMAP_INIT) {
	    std::cerr << "init local " << pm_c2h_header.local_id << " network " << pm_c2h_header.id << std::endl;
	    // Store data associated with this RPC
	    homa_rpc_t homa_rpc;
	    homa_rpc.saddr   = pm_c2h_header.saddr;
	    homa_rpc.daddr   = pm_c2h_header.daddr;
	    homa_rpc.dport   = pm_c2h_header.dport;
	    homa_rpc.sport   = pm_c2h_header.sport;
	    homa_rpc.id      = pm_c2h_header.id;
	    homa_rpc.peer_id = pm_c2h_header.peer_id;

	    recv_rpcs[pm_c2h_header.local_id] = homa_rpc;

	    msg_addr_t msg_addr = ((c2h_buff_ids[pm_c2h_header.sport] * HOMA_MAX_MESSAGE_LENGTH) << 1);
	    c2h_rpc_to_offset[pm_c2h_header.local_id] = msg_addr;
	    c2h_buff_ids[pm_c2h_header.sport]++;
	}
	
	// TODO should log errors here for hash table misses
	switch (pm_c2h_header.type) {
	    case DATA: {
		if (pm_c2h_header.message_length > pm_c2h_header.incoming) {
		    srpt_grant_new_t grant_in;


		    grant_in(SRPT_GRANT_NEW_MSG_LEN) = pm_c2h_header.message_length;
		    grant_in(SRPT_GRANT_NEW_RPC_ID)  = pm_c2h_header.local_id;
		    grant_in(SRPT_GRANT_NEW_PEER_ID) = pm_c2h_header.peer_id;
		    
		    grant_in(SRPT_GRANT_NEW_PMAP)    = pm_c2h_header.packetmap;
		    
		    // Notify the grant queue of this receipt
		    grant_queue_o.write(grant_in); 
		}

		pm_c2h_header_o.write(pm_c2h_header);

		// Log the receipt of a data packet
		c2h_pkt_log_o.write(LOG_DATA_OUT);
		
		break;
	    }
		
	    case GRANT: {

		srpt_queue_entry_t srpt_grant_notif;
		srpt_grant_notif(SRPT_QUEUE_ENTRY_RPC_ID)   = pm_c2h_header.local_id;
		srpt_grant_notif(SRPT_QUEUE_ENTRY_GRANTED)  = pm_c2h_header.grant_offset;
		srpt_grant_notif(SRPT_QUEUE_ENTRY_PRIORITY) = SRPT_GRANT_UPDATE;

		// Instruct the SRPT data of the new grant
		data_srpt_o.write(srpt_grant_notif);

		// Log the receipt of the grant packet
		c2h_pkt_log_o.write(LOG_GRANT_OUT);
		
		break;
	    }
	}
    }

    /* pre packet map */
    header_t c2h_header;
    if (c2h_header_i.read_nb(c2h_header)) {

	/* Perform the peer ID lookup regardless */
	peer_hashpack_t peer_query = {c2h_header.saddr};
	c2h_header.peer_id          = peer_cam.search(peer_query);
 	
	// If the peer is not registered, generate new ID and register it
	if (c2h_header.peer_id == 0) {
	    c2h_header.peer_id = peer_ids.pop();

	    peer_cam.insert(peer_query, c2h_header.peer_id);
	}

	if (!IS_CLIENT(c2h_header.id)) {
	    /* If we are the client then the entry already exists inside
	     * rpc_client. If we are the server then the entry needs to be
	     * created inside rpc_server if this is the first packet received
	     * for a message. If it is not the first message then the entry
	     * already exists in server_rpcs.
	     */
	    rpc_hashpack_t rpc_query = {c2h_header.saddr, c2h_header.id, c2h_header.sport, 0};

	    std::cerr << "search saddr " << c2h_header.saddr << std::endl;

	    c2h_header.local_id = rpc_cam.search(rpc_query);

	    if (c2h_header.local_id == 0) {
		c2h_header.local_id = server_ids.pop();

		// If the rpc is not registered, generate a new RPC ID and register it 
		rpc_cam.insert(rpc_query, c2h_header.local_id);
		std::cerr << "map create " << c2h_header.local_id << " net " << c2h_header.id << std::endl;
	    } else {
		std::cerr << "map hit local " << c2h_header.local_id << " net " << c2h_header.id << std::endl;
	    }
	} else {
	    c2h_header.local_id = c2h_header.id;
	}

	c2h_header_o.write(c2h_header);
    }
}

void dbuff_notif_proc(
    homa_rpc_t * send_rpcs,
    hls::stream<srpt_queue_entry_t> & dbuff_notif_i,
    hls::stream<srpt_queue_entry_t> & dbuff_notif_o
    ) {

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
}

void dma_fetch_proc(
    homa_rpc_t * send_rpcs,
    host_addr_t * h2c_port_to_msgbuff,
    msg_addr_t * h2c_rpc_to_offset,
    hls::stream<srpt_queue_entry_t> & dma_r_req_i,
    hls::stream<dma_r_req_t> & dma_r_req_o
    ) {

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
}

void dma_write_proc(
    host_addr_t * c2h_port_to_msgbuff,
    msg_addr_t  * c2h_rpc_to_offset,
    hls::stream<dma_w_req_t> & dma_w_req_i,
    hls::stream<dma_w_req_t> & dma_w_req_o
    ) {
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
}

void update_host_map_proc(
    host_addr_t * h2c_port_to_msgbuff,
    host_addr_t * c2h_port_to_msgbuff,
    host_addr_t * c2h_port_to_metarecv,
    host_addr_t * c2h_port_to_metasend,
    hls::stream<port_to_phys_t> & c2h_port_to_msgbuff_i,
    hls::stream<port_to_phys_t> & h2c_port_to_msgbuff_i,
    hls::stream<port_to_phys_t> & c2h_port_to_metarecv_i,
    hls::stream<port_to_phys_t> & c2h_port_to_metasend_i
    ) {

    port_to_phys_t new_h2c_port_to_msgbuff;
    if (h2c_port_to_msgbuff_i.read_nb(new_h2c_port_to_msgbuff)) {
	h2c_port_to_msgbuff[new_h2c_port_to_msgbuff(PORT_TO_PHYS_PORT)] = new_h2c_port_to_msgbuff(PORT_TO_PHYS_ADDR);
    }

    port_to_phys_t new_c2h_port_to_msgbuff;
    if (c2h_port_to_msgbuff_i.read_nb(new_c2h_port_to_msgbuff)) {
	c2h_port_to_msgbuff[new_c2h_port_to_msgbuff(PORT_TO_PHYS_PORT)] = new_c2h_port_to_msgbuff(PORT_TO_PHYS_ADDR);
    }

    port_to_phys_t new_c2h_port_to_metarecv;
    if (c2h_port_to_metarecv_i.read_nb(new_c2h_port_to_metarecv)) {
	c2h_port_to_metarecv[new_c2h_port_to_metarecv(PORT_TO_PHYS_PORT)] = new_c2h_port_to_metarecv(PORT_TO_PHYS_ADDR);
    }

    port_to_phys_t new_c2h_port_to_metasend;
    if (c2h_port_to_metasend_i.read_nb(new_c2h_port_to_metasend)) {
	c2h_port_to_metasend[new_c2h_port_to_metasend(PORT_TO_PHYS_PORT)] = new_c2h_port_to_metasend(PORT_TO_PHYS_ADDR);
    }
}

void send_proc(
    homa_rpc_t * send_rpcs,
    homa_rpc_t * recv_rpcs,
    host_addr_t * c2h_port_to_metasend,
    host_addr_t * c2h_port_to_headsend,
    msg_addr_t * h2c_rpc_to_offset,
    hls::stream<msghdr_send_t> & msghdr_send_i,
    hls::stream<dma_w_req_t> & msghdr_send_o,
    hls::stream<srpt_queue_entry_t> & data_queue_o,
    hls::stream<srpt_queue_entry_t> & fetch_queue_o,
    hls::stream<dbuff_id_t> & free_dbuff_id_i
    ) {

    static stack_t<local_id_t, MAX_RPCS> client_ids(STACK_EVEN);
    static stack_t<local_id_t, NUM_EGRESS_BUFFS> msg_cache_ids(STACK_ALL);

    static msghdr_send_t msghdr_send;
    static bool headsend_update = false;

    if (!headsend_update && msghdr_send_i.read_nb(msghdr_send)) {

	homa_rpc_t rpc;

	if (msghdr_send.data(MSGHDR_SEND_ID) == 0) {
	    msghdr_send.data(MSGHDR_SEND_ID) = client_ids.pop();
	    rpc.id = msghdr_send.data(MSGHDR_SEND_ID);
	} else {
	    rpc.id = recv_rpcs[msghdr_send.data(MSGHDR_SEND_ID)].id;
	}

	local_id_t id = msghdr_send.data(MSGHDR_SEND_ID);

	rpc.saddr        = msghdr_send.data(MSGHDR_SADDR);
	rpc.daddr        = msghdr_send.data(MSGHDR_DADDR);
	rpc.sport        = msghdr_send.data(MSGHDR_SPORT);
	rpc.dport        = msghdr_send.data(MSGHDR_DPORT);
	rpc.buff_addr    = msghdr_send.data(MSGHDR_BUFF_ADDR);
	rpc.buff_size    = msghdr_send.data(MSGHDR_BUFF_SIZE);
	rpc.cc           = msghdr_send.data(MSGHDR_SEND_CC);
	rpc.h2c_buff_id  = msg_cache_ids.pop();

	std::cerr << "new send entry local " << msghdr_send.data(MSGHDR_SEND_ID) << " network " << rpc.id << std::endl;

	send_rpcs[id]         = rpc;
	h2c_rpc_to_offset[id] = (msghdr_send.data(MSGHDR_BUFF_ADDR) << 1) | 1;

	dma_w_req_t msghdr_resp;
	msghdr_resp.data   = msghdr_send.data;

	ap_uint<64> offset = ((c2h_port_to_headsend[msghdr_send.data(MSGHDR_SPORT)]) % (16384 - 64)) + 64;
	msghdr_resp.offset = c2h_port_to_metasend[msghdr_send.data(MSGHDR_SPORT)] + offset;
	msghdr_resp.strobe = 64;

	msghdr_send_o.write(msghdr_resp);
	
	// TODO so much type shuffling here.... 
    	srpt_queue_entry_t srpt_data_in;
	srpt_data_in(SRPT_QUEUE_ENTRY_RPC_ID)    = id;
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
	srpt_fetch_in(SRPT_QUEUE_ENTRY_RPC_ID)    = id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = rpc.h2c_buff_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_REMAINING) = rpc.buff_size;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_GRANTED)   = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;
	
	// Insert this entry into the SRPT fetch queue
	fetch_queue_o.write(srpt_fetch_in);

	headsend_update = true;
    } else if (headsend_update) {
	ap_uint<64> offset = ((c2h_port_to_headsend[msghdr_send.data(MSGHDR_SPORT)]) % (16384 - 64)) + 64;
	c2h_port_to_headsend[msghdr_send.data(MSGHDR_SPORT)] = offset;

	dma_w_req_t msghdr_resp;
	msghdr_resp.data   = offset;
	msghdr_resp.offset = c2h_port_to_metasend[msghdr_send.data(MSGHDR_SPORT)];
	msghdr_resp.strobe = 8;

	msghdr_send_o.write(msghdr_resp);
	
	headsend_update = false;
    }

    dbuff_id_t dbuff_id;
    if (free_dbuff_id_i.read_nb(dbuff_id)) {
	msg_cache_ids.push(dbuff_id);
    }
}

void recv_proc(
    host_addr_t * c2h_port_to_metarecv,
    host_addr_t * c2h_port_to_headrecv,
    hls::stream<header_t> & complete_msgs_i,
    hls::stream<dma_w_req_t> & msghdr_recv_o
    ) {
    
    static header_t complete_msg;
    static bool headrecv_update = false;

    if (!headrecv_update && complete_msgs_i.read_nb(complete_msg)) {
	msghdr_recv_t new_recv;
	new_recv.data(MSGHDR_SADDR)      = complete_msg.saddr;
	new_recv.data(MSGHDR_DADDR)      = complete_msg.daddr;
	new_recv.data(MSGHDR_SPORT)      = complete_msg.sport;
	new_recv.data(MSGHDR_DPORT)      = complete_msg.dport;
	new_recv.data(MSGHDR_BUFF_ADDR)  = complete_msg.host_addr;
	new_recv.data(MSGHDR_BUFF_SIZE)  = complete_msg.message_length;
	new_recv.data(MSGHDR_RECV_ID)    = complete_msg.local_id; 
	// new_recv.data(MSGHDR_RECV_ID)    = complete_msg.id; 
	new_recv.data(MSGHDR_RECV_CC)    = complete_msg.completion_cookie;

	dma_w_req_t msghdr_resp;
	msghdr_resp.data   = new_recv.data;

	ap_uint<64> offset = ((c2h_port_to_headrecv[complete_msg.sport]) % (16384 - 64)) + 64;
	msghdr_resp.offset = c2h_port_to_metarecv[new_recv.data(MSGHDR_SPORT)] + offset;
	msghdr_resp.strobe = 64;

	msghdr_recv_o.write(msghdr_resp);

	headrecv_update = true;
    } else if (headrecv_update) {
	ap_uint<64> offset = ((c2h_port_to_headrecv[complete_msg.sport]) % (16384 - 64)) + 64;
	c2h_port_to_headrecv[complete_msg.sport] = offset;

	dma_w_req_t msghdr_resp;
	msghdr_resp.data   = offset;
	msghdr_resp.offset = c2h_port_to_metarecv[complete_msg.sport];
	msghdr_resp.strobe = 8;

	msghdr_recv_o.write(msghdr_resp);
	
	headrecv_update = false;
    }
}

/**
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
    hls::stream<dma_w_req_t> & msghdr_send_o,
    hls::stream<dma_w_req_t> & msghdr_recv_o,
    hls::stream<srpt_queue_entry_t> & data_queue_o,
    hls::stream<srpt_queue_entry_t> & fetch_queue_o,
    hls::stream<srpt_grant_new_t> & grant_queue_o,
    hls::stream<header_t> & h2c_header_i,
    hls::stream<header_t> & h2c_header_o,
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<header_t> & pm_c2h_header_i,
    hls::stream<header_t> & pm_c2h_header_o,
    hls::stream<header_t> & complete_msgs_i,
    hls::stream<srpt_queue_entry_t> & data_srpt_o,
    hls::stream<srpt_queue_entry_t> & dbuff_notif_i,
    hls::stream<srpt_queue_entry_t> & dbuff_notif_o,
    hls::stream<port_to_phys_t> & c2h_port_to_msgbuff_i,
    hls::stream<dma_w_req_t> & dma_w_req_i,
    hls::stream<dma_w_req_t> & dma_w_req_o,
    hls::stream<port_to_phys_t> & h2c_port_to_msgbuff_i,
    hls::stream<srpt_queue_entry_t> & dma_r_req_i,
    hls::stream<dma_r_req_t> & dma_r_req_o,
    hls::stream<port_to_phys_t> & c2h_port_to_metasend_i,
    hls::stream<port_to_phys_t> & c2h_port_to_metarecv_i,
    hls::stream<dbuff_id_t> & free_dbuff_id_i,
    hls::stream<ap_uint<8>> & h2c_pkt_log_o,
    hls::stream<ap_uint<8>> & c2h_pkt_log_o
    ) {
    /* TODO: this is potentially not densely utilized because of paritiy of
     * rpc IDs for client vs server.
     */
    /* Long-lived RPC data */
    static homa_rpc_t send_rpcs[MAX_RPCS];
    static homa_rpc_t recv_rpcs[MAX_RPCS];

#pragma HLS bind_storage variable=send_rpcs type=RAM_1WNR
#pragma HLS bind_storage variable=recv_rpcs type=RAM_1WNR

    /* Port to metadata mapping DMA */
    static host_addr_t c2h_port_to_metasend[MAX_PORTS]; // Port -> metadata RB
    static host_addr_t c2h_port_to_headsend[MAX_PORTS]; // Port -> current offset in RB

    static host_addr_t c2h_port_to_metarecv[MAX_PORTS]; // Port -> metadata RB
    static host_addr_t c2h_port_to_headrecv[MAX_PORTS]; // Port -> current offset in RB

    /* Port/RPC to user databuffer mapping DMA */
    static host_addr_t c2h_port_to_msgbuff[MAX_PORTS];  // Port -> large c2h buffer space 
    static msg_addr_t  c2h_rpc_to_offset[MAX_RPCS];     // RPC -> offset in that buffer space
    static ap_uint<7>  c2h_buff_ids[MAX_PORTS];         // Next availible offset in buffer space

    static host_addr_t h2c_port_to_msgbuff[MAX_PORTS];  // Port -> large h2c buffer space
    static msg_addr_t  h2c_rpc_to_offset[MAX_RPCS];     // RPC -> offset in that buffer space

    // TODO is this potentially risky with OOO small packets
    // Can maybe do a small sort of rebuffering if needed?
#pragma HLS dependence variable=send_rpcs inter WAR false
#pragma HLS dependence variable=send_rpcs inter RAW false
#pragma HLS dependence variable=recv_rpcs inter WAR false
#pragma HLS dependence variable=recv_rpcs inter RAW false

    // TODO is this a guarantee??
#pragma HLS dependence variable=c2h_buff_ids inter RAW false
#pragma HLS dependence variable=c2h_buff_ids inter WAR false

#pragma HLS dependence variable=c2h_port_to_headsend intra WAR false
#pragma HLS dependence variable=c2h_port_to_headsend intra RAW false

#pragma HLS dependence variable=c2h_port_to_headrecv intra WAR false
#pragma HLS dependence variable=c2h_port_to_headrecv intra RAW false

#pragma HLS dependence variable=c2h_port_to_headsend inter WAR false
#pragma HLS dependence variable=c2h_port_to_headsend inter RAW false

#pragma HLS dependence variable=c2h_port_to_headrecv inter WAR false
#pragma HLS dependence variable=c2h_port_to_headrecv inter RAW false

#pragma HLS pipeline II=1

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

    /* augment incoming headers with rpc state data*/
    h2c_header_proc(
	send_rpcs,
	recv_rpcs,
	h2c_header_i,
	h2c_header_o,
	h2c_pkt_log_o
	);

    /* notify srpt data queue of dbuff updates */
    dbuff_notif_proc(
	send_rpcs,
	dbuff_notif_i,
	dbuff_notif_o
	);

    /* corrdinate DMA message data fetch */
    dma_fetch_proc(
	send_rpcs,
	h2c_port_to_msgbuff,
	h2c_rpc_to_offset,
	dma_r_req_i,
	dma_r_req_o
	);

    /* forward data to DMA write core */ 
    dma_write_proc(
	c2h_port_to_msgbuff,
	c2h_rpc_to_offset,
	dma_w_req_i,
	dma_w_req_o
	);

    /* incorporate new port to message buffer mappings */
    update_host_map_proc(
	h2c_port_to_msgbuff,
	c2h_port_to_msgbuff,
	c2h_port_to_metarecv,
	c2h_port_to_metasend,
	c2h_port_to_msgbuff_i,
	h2c_port_to_msgbuff_i,
	c2h_port_to_metarecv_i,
	c2h_port_to_metasend_i
	);

    /* forward completed messages to user */
    recv_proc(
	c2h_port_to_metarecv,
	c2h_port_to_headrecv,
	complete_msgs_i,
	msghdr_recv_o
	);

    send_proc(
	send_rpcs,
	recv_rpcs,
	c2h_port_to_metasend,
	c2h_port_to_headsend,
	h2c_rpc_to_offset,
	msghdr_send_i,
	msghdr_send_o,
	data_queue_o,
	fetch_queue_o,
	free_dbuff_id_i
	);

    c2h_header_proc(
	send_rpcs,
	recv_rpcs,
	c2h_buff_ids,
	c2h_rpc_to_offset,
	c2h_header_i,
	c2h_header_o,
	pm_c2h_header_i,
	pm_c2h_header_o,
	grant_queue_o,
	data_srpt_o,
	c2h_pkt_log_o
    );
}
