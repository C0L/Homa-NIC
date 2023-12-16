#include "homa.hh"
#include "stack.hh"

using namespace hls;

void user(
    hls::stream<msghdr_send_t> & sendmsg_i,
    hls::stream<msghdr_send_t> & sendmsg_o,
    hls::stream<msghdr_send_t> & recvmsg_i,
    ap_uint<512> cbs[MAX_RPCS],
    hls::stream<srpt_queue_entry_t> & new_fetch_o,
    hls::stream<srpt_queue_entry_t> & new_sendmsg_o
    ) {

#pragma HLS interface mode=ap_ctrl_none port=return

#pragma HLS pipeline II=1

#pragma HLS interface axis port=sendmsg_i
#pragma HLS interface axis port=sendmsg_o
#pragma HLS interface axis port=recvmsg_i
#pragma HLS interface bram port=cbs
#pragma HLS interface axis port=new_fetch_o
#pragma HLS interface axis port=new_sendmsg_o

    /* Unique local RPC ID assigned when this core is the client */
    static stack_t<local_id_t, MAX_RPCS> client_ids;

    /* Unique local databuffer IDs assigned for outgoing data */
    static stack_t<local_id_t, NUM_EGRESS_BUFFS> msg_cache_ids;
 
    msghdr_send_t msghdr_send;
    if (sendmsg_i.read_nb(msghdr_send)) {

	msghdr_send(MSGHDR_SEND_ID) = msghdr_send(MSGHDR_SEND_ID) == 0 ? client_ids.pop() : msghdr_send(MSGHDR_SEND_ID);

	cbs[msghdr_send(MSGHDR_SEND_ID)] = msghdr_send;

	ap_uint<32> dbuff_id = msg_cache_ids.pop();

	srpt_queue_entry_t srpt_data_in = 0;
	srpt_data_in(SRPT_QUEUE_ENTRY_RPC_ID)    = msghdr_send(MSGHDR_SEND_ID);
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = dbuff_id;
	srpt_data_in(SRPT_QUEUE_ENTRY_REMAINING) = msghdr_send(MSGHDR_BUFF_SIZE);
	srpt_data_in(SRPT_QUEUE_ENTRY_DBUFFERED) = 0; // Fully dbuffered
	srpt_data_in(SRPT_QUEUE_ENTRY_GRANTED)   = 0; // Fully granted
	//msghdr_send(MSGHDR_BUFF_SIZE) - ((((ap_uint<32>) RTT_BYTES) > rpc.buff_size) ? rpc.buff_size : ((ap_uint<32>) RTT_BYTES));
	srpt_data_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;

	// Insert this entry into the SRPT data queue
	new_sendmsg_o.write(srpt_data_in);
	
	srpt_queue_entry_t srpt_fetch_in = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_RPC_ID)    = msghdr_send(MSGHDR_SPORT);
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = dbuff_id;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_REMAINING) = msghdr_send(MSGHDR_BUFF_SIZE);
	srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_GRANTED)   = 0;
	srpt_fetch_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;
	
	// Insert this entry into the SRPT fetch queue
	new_fetch_o.write(srpt_fetch_in);

	sendmsg_o.write(msghdr_send);
    }
}

/* Maintain long-lived state associated with RPCs/NIC, including:
 *   - Memory management: client/server IDs, peer IDs, dbuff IDs
 *   - Userspace mappings
 *   - RPC setup info: ip addrs, ports, msg size, location, ....
 */

// void rpc_state(
//     hls::stream<dma_w_req_t> & sendmsg_dma_o,
//     hls::stream<srpt_queue_entry_t> & data_queue_o,
//     hls::stream<srpt_queue_entry_t> & fetch_queue_o,
//     hls::stream<srpt_grant_new_t> & grant_queue_o,
//     hls::stream<ap_uint<MSGHDR_RECV_SIZE>> & recvmsg_i,
//     hls::stream<dma_w_req_t> & recvmsg_dma_o,
//     hls::stream<header_t> & h2c_header_i,
//     hls::stream<header_t> & h2c_header_o,
//     hls::stream<header_t> & c2h_header_i,
//     hls::stream<header_t> & c2h_header_o,
//     hls::stream<srpt_queue_entry_t> & data_srpt_o,
//     hls::stream<srpt_queue_entry_t> & dbuff_notif_i,
//     hls::stream<srpt_queue_entry_t> & dbuff_notif_o,
//     hls::stream<port_to_phys_t> & c2h_port_to_msgbuff_i,
//     hls::stream<dma_w_req_t> & dma_w_req_i,
//     hls::stream<dma_w_req_t> & dma_w_req_o,
//     hls::stream<port_to_phys_t> & h2c_port_to_msgbuff_i,
//     hls::stream<port_to_phys_t> & c2h_port_to_metadata_i,
//     hls::stream<srpt_queue_entry_t> & dma_r_req_i,
//     hls::stream<dma_r_req_t> & dma_r_req_o,
//     hls::stream<dbuff_id_t> & free_dbuff_i,
//     hls::stream<ap_uint<8>> & h2c_pkt_log_o,
//     hls::stream<ap_uint<8>> & c2h_pkt_log_o
//     ) {
// 
//     /* Long-lived RPC data */
// 
//     /* Port to metadata mapping DMA */
// //    static host_addr_t c2h_port_to_metadata[MAX_PORTS]; // Port -> metadata buffer
// //#pragma HLS bind_storage variable=c2h_port_to_metadata type=RAM_1WNR
// //
// //    static host_addr_t h2c_port_to_msgbuff[MAX_PORTS];  // Port -> large h2c buffer space
// //    static msg_addr_t  h2c_rpc_to_offset[MAX_RPCS];     // RPC -> offset in that buffer space
// //
// //    // TODO will need a small packet OOO buffer to mitigate pipeline hazards
// //#pragma HLS dependence variable=send_rpcs inter WAR false
// //#pragma HLS dependence variable=send_rpcs inter RAW false
// //#pragma HLS dependence variable=recv_rpcs inter WAR false
// //#pragma HLS dependence variable=recv_rpcs inter RAW false
// //
// //    // TODO is this a guarantee??
// //#pragma HLS dependence variable=c2h_buff_ids inter RAW false
// //#pragma HLS dependence variable=c2h_buff_ids inter WAR false
// //
// //#pragma HLS pipeline II=1
// 
//     //port_to_phys_t new_h2c_port_to_msgbuff;
//     //if (h2c_port_to_msgbuff_i.read_nb(new_h2c_port_to_msgbuff)) {
//     //	h2c_port_to_msgbuff[new_h2c_port_to_msgbuff(PORT_TO_PHYS_PORT)] = new_h2c_port_to_msgbuff(PORT_TO_PHYS_ADDR);
//     //}
// 
//     //port_to_phys_t new_c2h_port_to_msgbuff;
//     //if (c2h_port_to_msgbuff_i.read_nb(new_c2h_port_to_msgbuff)) {
//     //	c2h_port_to_msgbuff[new_c2h_port_to_msgbuff(PORT_TO_PHYS_PORT)] = new_c2h_port_to_msgbuff(PORT_TO_PHYS_ADDR);
//     //}
// 
//     //port_to_phys_t new_c2h_port_to_metadata;
//     //if (c2h_port_to_metadata_i.read_nb(new_c2h_port_to_metadata)) {
//     //	c2h_port_to_metadata[new_c2h_port_to_metadata(PORT_TO_PHYS_PORT)] = new_c2h_port_to_metadata(PORT_TO_PHYS_ADDR);
//     //}
// 
//     msghdr_send_t msghdr_send;
//     dbuff_id_t dbuff_id;
//     if (sendmsg_i.read_nb(msghdr_send)) {
// 	// Copy sendmsg data to a homa rpc structure
// 	homa_rpc_t rpc(msghdr_send.data);
// 
// 	// Allocate space in the data buffer for this RPC h2c data
// 	rpc.h2c_buff_id = msg_cache_ids.pop();
// 	std::cerr << "\n\n\n POP \n\n\n";
// 
// 	// Is this a request or a response
// 	if (rpc.local_id == 0) { 
// 	    local_id_t new_client = (2 * client_ids.pop());
// 
// 	    // The network ID and local ID will be the same
// 	    rpc.local_id = new_client;
// 	    rpc.id       = new_client;
// 	}
// 
// 	msghdr_send.data(MSGHDR_SEND_ID) = rpc.id;
// 
// 	// Store the state associated with this data
// 	send_rpcs[rpc.local_id] = rpc;
// 
// 	h2c_rpc_to_offset[rpc.local_id] = (rpc.buff_addr << 1) | 1; // TODO can this get removed?
// 
// 	// TODO use get function from homa_rpc
// 
// 	if (rpc.buff_size != 0) {
// 	    srpt_queue_entry_t srpt_data_in = 0;
// 	    srpt_data_in(SRPT_QUEUE_ENTRY_RPC_ID)    = rpc.local_id;
// 	    srpt_data_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = rpc.h2c_buff_id;
// 	    srpt_data_in(SRPT_QUEUE_ENTRY_REMAINING) = rpc.buff_size;
// 	    srpt_data_in(SRPT_QUEUE_ENTRY_DBUFFERED) = rpc.buff_size;
// 	    // TODO would be cleaner if we could just set this to RTT_BYTES
// 	    srpt_data_in(SRPT_QUEUE_ENTRY_GRANTED)   = rpc.buff_size - ((((ap_uint<32>) RTT_BYTES) > rpc.buff_size)
// 									? rpc.buff_size : ((ap_uint<32>) RTT_BYTES));
// 	    srpt_data_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;
// 
// 	    // Insert this entry into the SRPT data queue
// 	    data_queue_o.write(srpt_data_in);
// 	    
// 	    srpt_queue_entry_t srpt_fetch_in = 0;
// 	    srpt_fetch_in(SRPT_QUEUE_ENTRY_RPC_ID)    = rpc.local_id;
// 	    srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFF_ID)  = rpc.h2c_buff_id;
// 	    srpt_fetch_in(SRPT_QUEUE_ENTRY_REMAINING) = rpc.buff_size;
// 	    srpt_fetch_in(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
// 	    srpt_fetch_in(SRPT_QUEUE_ENTRY_GRANTED)   = 0;
// 	    srpt_fetch_in(SRPT_QUEUE_ENTRY_PRIORITY)  = SRPT_ACTIVE;
// 	    
// 	    // Insert this entry into the SRPT fetch queue
// 	    fetch_queue_o.write(srpt_fetch_in);
// 	}
//     	
// 	/* Instruct the user that the sendmsg request is active */
// 	dma_w_req_t msghdr_resp;
// 	msghdr_resp.data   = msghdr_send.data;
// 	msghdr_resp.offset = c2h_port_to_metadata[msghdr_send.data(MSGHDR_SPORT)] + (msghdr_send.data(MSGHDR_RETURN) * 64);
// 	msghdr_resp.strobe = 64;
// 	sendmsg_dma_o.write(msghdr_resp);
// 
//     } else if (free_dbuff_i.read_nb(dbuff_id)) {
// 	msg_cache_ids.push(dbuff_id);
//     }
// 
//     ap_uint<MSGHDR_RECV_SIZE> msghdr_recv;
//     if (recvmsg_i.read_nb(msghdr_recv)) {
// 	dma_w_req_t msghdr_resp;
// 
// 	msghdr_resp.data = msghdr_recv;
// 	msghdr_resp.offset = c2h_port_to_metadata[msghdr_recv(MSGHDR_DPORT)] + (msghdr_recv(MSGHDR_RETURN) * 64);
// 	msghdr_resp.strobe = 64;
// 
// 	recvmsg_dma_o.write(msghdr_resp);
// 
//     }
// 
//     srpt_queue_entry_t dbuff_notif;
//     if (dbuff_notif_i.read_nb(dbuff_notif)) {
// 	local_id_t id = dbuff_notif(SRPT_QUEUE_ENTRY_RPC_ID);
// 
// 	homa_rpc_t homa_rpc = send_rpcs[id];
// 
// 	ap_uint<32> dbuff_offset = dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED);
// 
// 	dbuff_notif(SRPT_QUEUE_ENTRY_DBUFF_ID) = homa_rpc.h2c_buff_id;
// 	dbuff_notif(SRPT_QUEUE_ENTRY_PRIORITY) = SRPT_DBUFF_UPDATE;
// 
// 	if (dbuff_offset + DBUFF_CHUNK_SIZE < homa_rpc.buff_size) {
// 	    dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED) = homa_rpc.buff_size - dbuff_offset - DBUFF_CHUNK_SIZE;
// 	} else {
// 	    dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED) = 0;
// 	}
// 
// 	// TODO these notifs need to also go to the fetch core?
// 
// 	dbuff_notif_o.write(dbuff_notif);
//     }
// 
//     dma_w_req_t dma_w_req;
//     if (dma_w_req_i.read_nb(dma_w_req)) {
// 	// TODO should error log here if the entry does not exist
// 
// 	// Get the physical address of this ports entire buffer
// 	host_addr_t phys_addr = c2h_port_to_msgbuff[dma_w_req.port];
// 	msg_addr_t msg_addr   = c2h_rpc_to_offset[dma_w_req.rpc_id];
// 
// 	msg_addr >>= 1;
// 	dma_w_req.offset += phys_addr + msg_addr;
// 	dma_w_req_o.write(dma_w_req);
//     }
// 
//     srpt_queue_entry_t dma_fetch;
//     if (dma_r_req_i.read_nb(dma_fetch)) {
// 	local_id_t local_id = dma_fetch(SRPT_QUEUE_ENTRY_RPC_ID);
// 	homa_rpc_t homa_rpc = send_rpcs[local_id];
// 
// 	// TODO avoid BRAM read dependent on read
// 
// 	// Get the physical address of this ports entire buffer
// 
// 	msg_addr_t msg_addr   = h2c_rpc_to_offset[local_id];
// 	// host_addr_t phys_addr = h2c_port_to_msgbuff[homa_rpc.sport];
// 
// 	host_addr_t phys_addr = h2c_port_to_msgbuff[homa_rpc.sport];
// 	dma_r_req_t dma_r_req;
// 	dma_r_req(SRPT_QUEUE_ENTRY_SIZE-1, 0) = dma_fetch;
// 	dma_r_req(DMA_R_REQ_MSG_LEN)          = homa_rpc.buff_size;
// 
// 	msg_addr >>= 1;
// 	dma_r_req(DMA_R_REQ_HOST_ADDR) = (homa_rpc.buff_size - dma_fetch(SRPT_QUEUE_ENTRY_REMAINING)) + (phys_addr + msg_addr);
// 
// 	// TODO this should work and be a better choice
// 	// dma_r_req(DMA_R_REQ_HOST_ADDR) = dma_fetch(SRPT_QUEUE_ENTRY_DBUFFERED) + (phys_addr + msg_addr);
// 
// 	dma_r_req_o.write(dma_r_req);
//     }
// 
//     header_t h2c_header;
//     if (h2c_header_i.read_nb(h2c_header)) {
// 	
// 	homa_rpc_t homa_rpc;
// 	switch(h2c_header.type) {
// 	    case DATA:
// 		homa_rpc = send_rpcs[h2c_header.local_id];
// 
// 		h2c_header.daddr = homa_rpc.daddr;
// 		h2c_header.dport = homa_rpc.dport;
// 		h2c_header.saddr = homa_rpc.saddr;
// 		h2c_header.sport = homa_rpc.sport;
// 		h2c_header.id    = homa_rpc.id;
// 		break;
// 	    case GRANT:
// 		homa_rpc = recv_rpcs[h2c_header.local_id];
// 
// 		h2c_header.daddr = homa_rpc.saddr;
// 		h2c_header.dport = homa_rpc.sport;
// 		h2c_header.saddr = homa_rpc.daddr;
// 		h2c_header.sport = homa_rpc.dport;
// 		h2c_header.id    = homa_rpc.id;
// 		break;
// 	}
// 
// 	// Log the packet type we are sending
// 	h2c_pkt_log_o.write((h2c_header.type == DATA) ? LOG_DATA_OUT : LOG_GRANT_OUT);
// 
// 	// Get the location of buffered data and prepare for packet construction
// 	h2c_header.h2c_buff_id    = homa_rpc.h2c_buff_id;
// 	h2c_header.incoming       = homa_rpc.buff_size - h2c_header.incoming;
// 	h2c_header.data_offset    = homa_rpc.buff_size - h2c_header.data_offset;
// 	h2c_header.message_length = homa_rpc.buff_size;
// 
// 	h2c_header_o.write_nb(h2c_header);
//     }
// 
//     header_t c2h_header;
//     if (c2h_header_i.read_nb(c2h_header)) {
// 	if ((c2h_header.packetmap & PMAP_INIT) == PMAP_INIT) {
// 	    // Store data associated with this RPC
// 	    homa_rpc_t homa_rpc;
// 	    homa_rpc.saddr   = c2h_header.saddr;
// 	    homa_rpc.daddr   = c2h_header.daddr;
// 	    homa_rpc.dport   = c2h_header.dport;
// 	    homa_rpc.sport   = c2h_header.sport;
// 	    homa_rpc.id      = c2h_header.id;
// 	    homa_rpc.peer_id = c2h_header.peer_id;
// 
// 	    recv_rpcs[c2h_header.local_id] = homa_rpc;
// 
// 	    // msg_addr_t msg_addr = ((c2h_buff_ids[c2h_header.sport] * HOMA_MAX_MESSAGE_LENGTH) << 1);
// 	    msg_addr_t msg_addr = (c2h_buff_ids[c2h_header.sport]);
// 	    // msg_addr_t msg_addr; // TODO
// 	    c2h_rpc_to_offset[c2h_header.local_id] = msg_addr;
// 	    c2h_buff_ids[c2h_header.sport]++;
// 	}
// 	
// 	switch (c2h_header.type) {
// 	    case DATA: {
// 		if (c2h_header.message_length > c2h_header.incoming) {
// 		    srpt_grant_new_t grant_in;
// 
// 		    grant_in(SRPT_GRANT_NEW_MSG_LEN) = c2h_header.message_length;
// 		    grant_in(SRPT_GRANT_NEW_RPC_ID)  = c2h_header.local_id;
// 		    grant_in(SRPT_GRANT_NEW_PEER_ID) = c2h_header.peer_id;
// 		    
// 		    grant_in(SRPT_GRANT_NEW_PMAP)    = c2h_header.packetmap;
// 		    
// 		    // Notify the grant queue of this receipt
// 		    grant_queue_o.write(grant_in); 
// 		}
// 
// 		c2h_header_o.write(c2h_header);
// 
// 		// Log the receipt of a data packet
// 		c2h_pkt_log_o.write(LOG_DATA_OUT);
// 		
// 		break;
// 	    }
// 		
// 	    case GRANT: {
// 
// 		srpt_queue_entry_t srpt_grant_notif;
// 		srpt_grant_notif(SRPT_QUEUE_ENTRY_RPC_ID)   = c2h_header.local_id;
// 		srpt_grant_notif(SRPT_QUEUE_ENTRY_GRANTED)  = c2h_header.grant_offset;
// 		srpt_grant_notif(SRPT_QUEUE_ENTRY_PRIORITY) = SRPT_GRANT_UPDATE;
// 
// 		// Instruct the SRPT data of the new grant
// 		data_srpt_o.write(srpt_grant_notif);
// 
// 		// Log the receipt of the grant packet
// 		c2h_pkt_log_o.write(LOG_GRANT_OUT);
// 		
// 		break;
// 	    }
// 	}
//     }
// }
