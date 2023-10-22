#include "rpcmgmt.hh"
#include "hashmap.hh"

using namespace hls;

void h2c_header_proc(homa_rpc_t * client_rpcs,
		homa_rpc_t * server_rpcs,
		hls::stream<header_t> & pkt_in,
		hls::stream<header_t> & pkt_out,
		hls::stream<ap_uint<8>> & log_out) {

#pragma HLS pipeline
    
    header_t h2c_header;
    if (pkt_in.read_nb(h2c_header)) {
	homa_rpc_t homa_rpc = (IS_CLIENT(h2c_header.local_id)) ? client_rpcs[h2c_header.local_id] : server_rpcs[h2c_header.local_id];

	h2c_header.daddr = homa_rpc.daddr;
	h2c_header.dport = homa_rpc.dport;
	h2c_header.saddr = homa_rpc.saddr;
	h2c_header.sport = homa_rpc.sport;
	h2c_header.id    = homa_rpc.id;

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
    hls::stream<header_t> & h2c_header_i,
    hls::stream<header_t> & h2c_header_o,
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
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
    static homa_rpc_t client_rpcs[MAX_RPCS];
    static homa_rpc_t server_rpcs[MAX_RPCS];

#pragma HLS bind_storage variable=client_rpcs type=RAM_1WNR
#pragma HLS bind_storage variable=server_rpcs type=RAM_1WNR

    /* Resource management */
    static stack_t<local_id_t, MAX_RPCS> client_ids(STACK_EVEN);
    static stack_t<local_id_t, NUM_EGRESS_BUFFS> msg_cache_ids(STACK_ALL);

    /* Unique local RPC ID assigned when this core is the server */
    static stack_t<local_id_t, MAX_RPCS> server_ids(STACK_ODD);

    /* Unique local Peer IDs */
    static stack_t<peer_id_t, MAX_PEERS> peer_ids(STACK_ALL);

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

    /* hash(dest addr, sender ID, dest port) -> rpc ID */
    static cam_t<rpc_hashpack_t, local_id_t, 16, 4> rpc_cam;

    /* hash(dest addr) -> peer ID */
    static cam_t<peer_hashpack_t, peer_id_t, 16, 4> peer_cam;

#pragma HLS dataflow

    // TODO is this potentially risky with OOO small packets
    // Can maybe do a small sort of rebuffering if needed?
#pragma HLS dependence variable=client_rpcs inter WAR false
#pragma HLS dependence variable=client_rpcs inter RAW false
#pragma HLS dependence variable=server_rpcs inter WAR false
#pragma HLS dependence variable=server_rpcs inter RAW false

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
    h2c_header_proc(client_rpcs, server_rpcs, h2c_header_i, h2c_header_o, h2c_pkt_log_o);
	
    srpt_queue_entry_t dbuff_notif;
    if (dbuff_notif_i.read_nb(dbuff_notif)) {
	local_id_t id = dbuff_notif(SRPT_QUEUE_ENTRY_RPC_ID);

	homa_rpc_t homa_rpc = (IS_CLIENT(id)) ? client_rpcs[id] : server_rpcs[id];

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

    srpt_queue_entry_t dma_fetch;
    if (dma_r_req_i.read_nb(dma_fetch)) {
	local_id_t id = dma_fetch(SRPT_QUEUE_ENTRY_RPC_ID);
	homa_rpc_t homa_rpc = (IS_CLIENT(id)) ? client_rpcs[id] : server_rpcs[id];

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

    port_to_phys_t new_h2c_port_to_msgbuff;
    if (h2c_port_to_msgbuff_i.read_nb(new_h2c_port_to_msgbuff)) {
	h2c_port_to_msgbuff[new_h2c_port_to_msgbuff(PORT_TO_PHYS_PORT)] = new_h2c_port_to_msgbuff(PORT_TO_PHYS_ADDR);
    }

    port_to_phys_t new_c2h_port_to_msgbuff;
    if (c2h_port_to_msgbuff_i.read_nb(new_c2h_port_to_msgbuff)) {
	c2h_port_to_msgbuff[new_c2h_port_to_msgbuff(PORT_TO_PHYS_PORT)] = new_c2h_port_to_msgbuff(PORT_TO_PHYS_ADDR);
    }

    /* TODO Because the number of active messages is relatively few we can just CAM everything.
       We can have some denser storage back this up that will fill the CAM when needed */


    static header_t complete_msg;
    static bool headrecv_update = false;

    port_to_phys_t new_c2h_port_to_metarecv;
    if (c2h_port_to_metarecv_i.read_nb(new_c2h_port_to_metarecv)) {
	c2h_port_to_metarecv[new_c2h_port_to_metarecv(PORT_TO_PHYS_PORT)] = new_c2h_port_to_metarecv(PORT_TO_PHYS_ADDR);
	c2h_port_to_headrecv[new_c2h_port_to_metarecv(PORT_TO_PHYS_PORT)] = 64;
    } else if (!headrecv_update && complete_msgs_i.read_nb(complete_msg)) {
	msghdr_recv_t new_recv;
	new_recv.data(MSGHDR_SADDR)      = complete_msg.saddr;
	new_recv.data(MSGHDR_DADDR)      = complete_msg.daddr;
	new_recv.data(MSGHDR_SPORT)      = complete_msg.sport;
	new_recv.data(MSGHDR_DPORT)      = complete_msg.dport;
	new_recv.data(MSGHDR_BUFF_ADDR)  = complete_msg.host_addr;
	new_recv.data(MSGHDR_BUFF_SIZE)  = complete_msg.message_length;
	new_recv.data(MSGHDR_RECV_ID)    = complete_msg.local_id; 
	new_recv.data(MSGHDR_RECV_CC)    = complete_msg.completion_cookie; 
	// new_msg.data(MSGHDR_RECV_FLAGS) = IS_CLIENT(header_in.id) ? HOMA_RECVMSG_RESPONSE : HOMA_RECVMSG_REQUEST;

	dma_w_req_t msghdr_resp;
	msghdr_resp.data   = new_recv.data;

	ap_uint<64> offset = c2h_port_to_headrecv[complete_msg.sport];
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

    /* New Request or Response */
    static msghdr_send_t msghdr_send;
    static bool headsend_update = false;

    port_to_phys_t new_c2h_port_to_metasend;

    /* write paths */
    header_t c2h_header;
    if (c2h_header_i.read_nb(c2h_header)) {
	/* If we are the client then the entry already exists inside
	 * rpc_client. If we are the server then the entry needs to be
	 * created inside rpc_server if this is the first packet received
	 * for a message. If it is not the first message then the entry
	 * already exists in server_rpcs.
	 */
	// TODO the response should not trigger this path as it recovers the network id from the homarpc
	if (!IS_CLIENT(c2h_header.id)) {
	    rpc_hashpack_t rpc_query = {c2h_header.saddr, c2h_header.id, c2h_header.sport, 0};

	    c2h_header.local_id = rpc_cam.search(rpc_query);
 
            // If the rpc is not registered, generate a new RPC ID and register it 
 	    if (c2h_header.local_id == 0) {
 		c2h_header.local_id = server_ids.pop();

		// entry_t<rpc_hashpack_t, local_id_t> rpc_entry = {rpc_query, c2h_header.local_id};
		rpc_cam.insert(rpc_query, c2h_header.local_id);

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
	    c2h_header.peer_id          = peer_cam.search(peer_query);
 	
	    // If the peer is not registered, generate new ID and register it
	    if (c2h_header.peer_id == 0) {
	    	c2h_header.peer_id = peer_ids.pop();
	    	peer_cam.insert(peer_query, c2h_header.peer_id);
	    }
	} else {
	    homa_rpc_t homa_rpc = client_rpcs[c2h_header.id];

	    // If we are the client then a network ID is a local ID
	    c2h_header.local_id = c2h_header.id;
	    c2h_header.peer_id  = homa_rpc.peer_id;
	}

	switch (c2h_header.type) {
	    case DATA: {
		// Will this message ever need grants?
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
    } else if (c2h_port_to_metasend_i.read_nb(new_c2h_port_to_metasend)) {
	c2h_port_to_metasend[new_c2h_port_to_metasend(PORT_TO_PHYS_PORT)] = new_c2h_port_to_metasend(PORT_TO_PHYS_ADDR);
	c2h_port_to_headsend[new_c2h_port_to_metasend(PORT_TO_PHYS_PORT)] = 64;
    } else if (!headsend_update && msghdr_send_i.read_nb(msghdr_send)) {
	local_id_t id;

	if (msghdr_send.data(MSGHDR_SEND_ID) == 0) {
	    id = client_ids.pop();
	} else {
	    id = msghdr_send.data(MSGHDR_SEND_ID);
	}

	homa_rpc_t & rpc = (msghdr_send.data(MSGHDR_SEND_ID) == 0) ?
	    client_rpcs[id] : server_rpcs[msghdr_send.data(MSGHDR_SEND_ID)];

	/* If the caller provided an ID of 0 this is a request message and we
	 * need to generate a new local ID. Otherwise, this is a response
	 * message and the ID is already valid in homa_rpc buffer
	 */
	if (msghdr_send.data(MSGHDR_SEND_ID) == 0) {
	    rpc.saddr        = msghdr_send.data(MSGHDR_SADDR);
	    rpc.daddr        = msghdr_send.data(MSGHDR_DADDR);
	    rpc.sport        = msghdr_send.data(MSGHDR_SPORT);
	    rpc.dport        = msghdr_send.data(MSGHDR_DPORT);
	    rpc.buff_addr    = msghdr_send.data(MSGHDR_BUFF_ADDR);
	    rpc.buff_size    = msghdr_send.data(MSGHDR_BUFF_SIZE);
	    rpc.cc           = msghdr_send.data(MSGHDR_SEND_CC);
	    rpc.h2c_buff_id  = msg_cache_ids.pop();

	    rpc.id = id;
	    msghdr_send.data(MSGHDR_SEND_ID) = id;
	} else {
	    rpc.saddr        = msghdr_send.data(MSGHDR_SADDR);
	    rpc.daddr        = msghdr_send.data(MSGHDR_DADDR);
	    rpc.sport        = msghdr_send.data(MSGHDR_SPORT);
	    rpc.dport        = msghdr_send.data(MSGHDR_DPORT);
	    rpc.buff_addr    = msghdr_send.data(MSGHDR_BUFF_ADDR);
	    rpc.buff_size    = msghdr_send.data(MSGHDR_BUFF_SIZE);
	    rpc.cc           = msghdr_send.data(MSGHDR_SEND_CC);
	    rpc.h2c_buff_id  = msg_cache_ids.pop();
	}

	h2c_rpc_to_offset[id] = (msghdr_send.data(MSGHDR_BUFF_ADDR) << 1) | 1;

	dma_w_req_t msghdr_resp;
	msghdr_resp.data   = msghdr_send.data;

	ap_uint<64> offset = c2h_port_to_headsend[msghdr_send.data(MSGHDR_SPORT)];

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
