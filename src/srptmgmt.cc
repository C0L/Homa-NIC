#include "srptmgmt.hh"
#include "rpcmgmt.hh"

/**
 * WARNING: For C simulation only
 * srpt_data_pkts() - Determines what DATA packet to send next.
 * @sendmsg_i - New request or response messages from the user
 * @dbuff_notif_i - Updates about what data is held on-chip
 * @data_pkt_o - The next outgoing DATA packet that should be sent
 * @grant_notif_i - Authorizations to send more data
 */
void srpt_data_queue(hls::stream<srpt_queue_entry_t> & sendmsg_i,
		     hls::stream<srpt_queue_entry_t> & dbuff_notif_i,
		     hls::stream<srpt_queue_entry_t> & data_pkt_o,
		     hls::stream<srpt_queue_entry_t> & grant_notif_i) {

    static srpt_queue_entry_t active_rpcs[MAX_RPCS];

    if (!sendmsg_i.empty()) {
	srpt_queue_entry_t sendmsg = sendmsg_i.read();
	active_rpcs[sendmsg(SRPT_QUEUE_ENTRY_RPC_ID)] = sendmsg;
    } else if (!dbuff_notif_i.empty()) {
	srpt_queue_entry_t dbuff_notif = dbuff_notif_i.read();
	active_rpcs[dbuff_notif(SRPT_QUEUE_ENTRY_RPC_ID)](SRPT_QUEUE_ENTRY_DBUFFERED) = dbuff_notif(SRPT_QUEUE_ENTRY_DBUFFERED);
    } else if (!grant_notif_i.empty()) {
	srpt_queue_entry_t header_in = grant_notif_i.read();
	active_rpcs[header_in(SRPT_QUEUE_ENTRY_RPC_ID)](SRPT_QUEUE_ENTRY_GRANTED) = header_in(SRPT_QUEUE_ENTRY_GRANTED);
    }

    /* Find Next RPC to Send*/

    srpt_queue_entry_t best_send;
    best_send(SRPT_QUEUE_ENTRY_RPC_ID) = 0;

    for (int i = 0; i < MAX_RPCS; ++i) {
	// Only compare against RPC IDs that are active
	if (active_rpcs[i](SRPT_QUEUE_ENTRY_RPC_ID) != 0) {
	    // We found a better choice if there are less remaining bytes, or we are setting best_send for the first time
	    if (active_rpcs[i](SRPT_QUEUE_ENTRY_REMAINING) < best_send(SRPT_QUEUE_ENTRY_REMAINING) || best_send(SRPT_QUEUE_ENTRY_RPC_ID) == 0) {
		bool granted   = (active_rpcs[i](SRPT_QUEUE_ENTRY_GRANTED) + 1 <= active_rpcs[i](SRPT_QUEUE_ENTRY_REMAINING)) || (active_rpcs[i](SRPT_QUEUE_ENTRY_GRANTED) == 0);
		bool dbuffered = (active_rpcs[i](SRPT_QUEUE_ENTRY_DBUFFERED) + HOMA_PAYLOAD_SIZE <= active_rpcs[i](SRPT_QUEUE_ENTRY_REMAINING)) || (active_rpcs[i](SRPT_QUEUE_ENTRY_DBUFFERED) == 0);

		if (granted && dbuffered) {
		    best_send = active_rpcs[i];
		}
	    }
	}
    }

    // Did we find a valid RPC we should send
    if (best_send(SRPT_QUEUE_ENTRY_RPC_ID) != 0) {
	ap_uint<32> remaining = (HOMA_PAYLOAD_SIZE > best_send(SRPT_QUEUE_ENTRY_REMAINING)) 
	    ? ((ap_uint<32>) 0) : ((ap_uint<32>) (best_send(SRPT_QUEUE_ENTRY_REMAINING) - HOMA_PAYLOAD_SIZE));

	std::cerr << "data packet out " << std::endl;
	data_pkt_o.write(best_send);

	active_rpcs[best_send(SRPT_QUEUE_ENTRY_RPC_ID)](SRPT_QUEUE_ENTRY_REMAINING) = remaining;

	// The entry is complete so zero it
	if (remaining == 0) {
	    std::cerr << "zeroed entry " << std::endl;
	    active_rpcs[best_send(SRPT_QUEUE_ENTRY_RPC_ID)](SRPT_QUEUE_ENTRY_RPC_ID) = 0;
	}
    }
}

void srpt_fetch_queue(hls::stream<srpt_queue_entry_t> & sendmsg_i,
		      hls::stream<srpt_queue_entry_t> & cache_req_o) {

    static srpt_queue_entry_t active_rpcs[MAX_RPCS];

    if (!sendmsg_i.empty()) {
	srpt_queue_entry_t sendmsg = sendmsg_i.read();

	active_rpcs[sendmsg(SRPT_QUEUE_ENTRY_RPC_ID)] = sendmsg;
    }

    /* Find Next RPC to Fetch Data */

    srpt_queue_entry_t best_fetch;
    best_fetch(SRPT_QUEUE_ENTRY_REMAINING) = 0;

    // Brute force finding next best RPC to get data for
    for (int i = 0; i < MAX_RPCS; ++i) {
	// Check if there is data remaining to buffer
	if (active_rpcs[i](SRPT_QUEUE_ENTRY_REMAINING) != 0) {
	    if (active_rpcs[i](SRPT_QUEUE_ENTRY_REMAINING) < best_fetch(SRPT_QUEUE_ENTRY_REMAINING) || best_fetch(SRPT_QUEUE_ENTRY_REMAINING) == 0) {
		best_fetch = active_rpcs[i];
	    }
	}
    }

    if (best_fetch(SRPT_QUEUE_ENTRY_REMAINING) != 0) {
	ap_uint<32> dbuffered = (64 > best_fetch(SRPT_QUEUE_ENTRY_REMAINING)) 
	    ? ((ap_uint<32>) 0) : ((ap_uint<32>) (best_fetch(SRPT_QUEUE_ENTRY_REMAINING) - 64));

	active_rpcs[best_fetch(SRPT_QUEUE_ENTRY_RPC_ID)](SRPT_QUEUE_ENTRY_REMAINING) = dbuffered;
	active_rpcs[best_fetch(SRPT_QUEUE_ENTRY_RPC_ID)](SRPT_QUEUE_ENTRY_DBUFFERED) = active_rpcs[best_fetch(SRPT_QUEUE_ENTRY_RPC_ID)](SRPT_QUEUE_ENTRY_DBUFFERED) + 64;

	cache_req_o.write(best_fetch);
    }
}


/**
 * WARNING: For C simulation only
 * srpt_grant_pkts() - Determines what message to grant to next. Messages are ordered by
 * increasing number of bytes not yet granted. When grants are made, the peer is
 * stored in the active set. Grants will not be issued to peers that are in the active set.
 * Maintains RTTBytes per mesage in the active set at a time if possible.
 * @header_in_i: Injest path for new messages that will need to be granted to.
 * @grant_pkt_o: The next rpc that should be granted to. Goes to packet egress process.
 *
 */
void srpt_grant_pkts(hls::stream<srpt_grant_new_t> & grant_in_i,
		     hls::stream<srpt_grant_send_t> & grant_out_o) {
    
    // Because this is only used for sim we brute force grants
    static srpt_grant_t entries[MAX_RPCS];
    static ap_uint<32> avail_bytes = OVERCOMMIT_BYTES; 

    // Headers from incoming DATA packets
    if (!grant_in_i.empty()) {
	srpt_grant_new_t grant_in = grant_in_i.read();

	// The first unscheduled packet creates the entry. Only need an entry if the RPC needs grants.
	if ((grant_in(SRPT_GRANT_NEW_PMAP) & PMAP_INIT) == PMAP_INIT) {
	    entries[grant_in(SRPT_GRANT_NEW_RPC_ID)] = {grant_in(SRPT_GRANT_NEW_PEER_ID), grant_in(SRPT_GRANT_NEW_RPC_ID), HOMA_PAYLOAD_SIZE, grant_in(SRPT_GRANT_NEW_MSG_LEN) - HOMA_PAYLOAD_SIZE};
	    std::cerr << "GRANT INIT " << grant_in(SRPT_GRANT_NEW_RPC_ID) << " " << grant_in(SRPT_GRANT_NEW_MSG_LEN) - HOMA_PAYLOAD_SIZE << " " << grant_in(SRPT_GRANT_NEW_PEER_ID) << std::endl;
	} else {
	    entries[grant_in(SRPT_GRANT_NEW_RPC_ID)].recv_bytes += HOMA_PAYLOAD_SIZE;
	    std::cerr << "UDPATED DATA IN GRANT QUEUE " << entries[grant_in(SRPT_GRANT_NEW_RPC_ID)].recv_bytes << std::endl;
	} 
    } else {
	srpt_grant_t best[8];
	
	for (int i = 0; i < 8; ++i) {
	    best[i] = {0, 0, 0xFFFFFFFF, 0xFFFFFFFF}; 
	}

	// Fill each entry in the best queue
	for (int i = 0; i < 8; ++i) {
	    srpt_grant_t curr_best = entries[0];
	    for (int e = 0; e < MAX_RPCS; ++e) {

		if (entries[e].rpc_id == 0) continue;
		// Is this entry better than our current best
		if (entries[e].grantable_bytes < curr_best.grantable_bytes || curr_best.rpc_id == 0) {

		    bool dupe = false;
		    for (int s = 0; s < 8; s++) {
			if (entries[e].peer_id == best[s].peer_id) {
			    dupe = true;
			}
		    }

		    // std::cerr << "POTENTIAL ENTRY " << dupe << std::endl;
		    curr_best = (!dupe) ? entries[e] : curr_best;
		}
	    }
	    best[i] = curr_best;
	}

	srpt_grant_t next_grant = best[0];

	// Who to grant to next?
	for (int i = 0; i < 8; ++i) {
	    if (best[i].grantable_bytes < next_grant.grantable_bytes && best[i].recv_bytes != 0) {
		std::cerr << "CANDIDATE" << std::endl;
		next_grant = best[i];
	    }
	}

	if (next_grant.recv_bytes > 0 && next_grant.rpc_id != 0) {
	    srpt_grant_send_t grant_out;

	    if (next_grant.recv_bytes > avail_bytes) {
		// Just send avail_pkts of data
		entries[next_grant.rpc_id].recv_bytes -= avail_bytes;
		entries[next_grant.rpc_id].grantable_bytes -= avail_bytes;

		// grant_out(SRPT_GRANT_SEND_GRANT)   = avail_bytes;
		grant_out(SRPT_GRANT_SEND_MSG_ADDR) = avail_bytes;
		grant_out(SRPT_GRANT_SEND_RPC_ID)   = next_grant.rpc_id;
		// grant_out(SRPT_GRANT_SEND_PEER_ID) = next_grant.peer_id;
		grant_out_o.write(grant_out);

		std::cerr << "GRANT OUT FINAL " << std::endl;

	    } else {
		entries[next_grant.rpc_id].recv_bytes = 0;
		entries[next_grant.rpc_id].grantable_bytes -= MIN(next_grant.recv_bytes, entries[next_grant.rpc_id].grantable_bytes);

		srpt_grant_send_t grant_out;

		grant_out(SRPT_GRANT_SEND_MSG_ADDR)   = entries[next_grant.rpc_id].grantable_bytes;
		grant_out(SRPT_GRANT_SEND_RPC_ID)  = next_grant.rpc_id;
		// grant_out(SRPT_GRANT_SEND_PEER_ID) = next_grant.peer_id;

		grant_out_o.write(grant_out);

		std::cerr << "GRANT OUT " << std::endl;

		if (next_grant.grantable_bytes == 0) {
		    entries[next_grant.rpc_id] = {0, 0, 0xFFFFFFFF, 0xFFFFFFFF};
		}
	    } 
	}
    }
}
