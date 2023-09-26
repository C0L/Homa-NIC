#include "srptmgmt.hh"
#include "rpcmgmt.hh"

/**
 * WARNING: For C simulation only
 * srpt_data_pkts() - Determines what DATA packet to send next.
 * @sendmsg_i - New request or response messages from the user

 * @dbuff_notif_i - Updates about what data is held on-chip
 * @data_pkt_o - The next outgoing DATA packet that should be sent
 */
void srpt_data_pkts(hls::stream<srpt_data_new_t> & sendmsg_i,
		    hls::stream<srpt_data_dbuff_notif_t> & dbuff_notif_i,
		    hls::stream<srpt_data_send_t> & data_pkt_o,
		    hls::stream<srpt_data_grant_notif_t> & grant_notif_i,
		    hls::stream<srpt_data_fetch_t> & cache_req_o) {

    static srpt_data_t active_rpcs[MAX_RPCS];

    /* Add Elements to */

    if (!dbuff_notif_i.empty()) {
	srpt_data_dbuff_notif_t dbuff_notif = dbuff_notif_i.read();
	active_rpcs[dbuff_notif(SRPT_DATA_DBUFF_NOTIF_RPC_ID)].dbuffered_recv = dbuff_notif(SRPT_DATA_DBUFF_NOTIF_MSG_ADDR);
    }

    if (!grant_notif_i.empty()) {
	srpt_data_grant_notif_t header_in = grant_notif_i.read();
	active_rpcs[header_in(SRPT_DATA_GRANT_NOTIF_RPC_ID)].granted = header_in(SRPT_DATA_GRANT_NOTIF_MSG_ADDR);
    }

    if (!sendmsg_i.empty()) {
	srpt_data_new_t sendmsg = sendmsg_i.read();

	active_rpcs[sendmsg(SRPT_DATA_NEW_RPC_ID)].rpc_id         = sendmsg(SRPT_DATA_NEW_RPC_ID);
	active_rpcs[sendmsg(SRPT_DATA_NEW_RPC_ID)].dbuff_id       = sendmsg(SRPT_DATA_NEW_DBUFF_ID);
	active_rpcs[sendmsg(SRPT_DATA_NEW_RPC_ID)].remaining      = sendmsg(SRPT_DATA_NEW_MSG_LEN);
	active_rpcs[sendmsg(SRPT_DATA_NEW_RPC_ID)].dbuffered_req  = sendmsg(SRPT_DATA_NEW_MSG_LEN);
	active_rpcs[sendmsg(SRPT_DATA_NEW_RPC_ID)].dbuffered_recv = sendmsg(SRPT_DATA_NEW_MSG_LEN);
	active_rpcs[sendmsg(SRPT_DATA_NEW_RPC_ID)].granted        = sendmsg(SRPT_DATA_NEW_GRANTED);
	active_rpcs[sendmsg(SRPT_DATA_NEW_RPC_ID)].msg_len        = sendmsg(SRPT_DATA_NEW_MSG_LEN);
    }


    /* Find Next RPC to Send*/

    srpt_data_t best_send;
    best_send.rpc_id = 0;

    // Brute force checdbuff_notif(SRPT_DATA_DBUFF_NOTIF_MSG_ADDR)dbuff_notif(SRPT_DATA_DBUFF_NOTIF_MSG_ADDR);;k every RPC ID for the best choice
    for (int i = 0; i < MAX_RPCS; ++i) {
	// Only compare against RPC IDs that are active
	if (active_rpcs[i].rpc_id != 0) {
	    // We found a better choice if there are less remaining bytes, or we are setting best_send for the first time
	    if (active_rpcs[i].remaining < best_send.remaining || best_send.rpc_id == 0) {
		bool granted   = (active_rpcs[i].granted + 1 <= active_rpcs[i].remaining) || (active_rpcs[i].granted == 0);
		bool dbuffered = (active_rpcs[i].dbuffered_recv + HOMA_PAYLOAD_SIZE <= active_rpcs[i].remaining) || (active_rpcs[i].dbuffered_recv == 0);

		// TODO why add homa payload size and not subtract???

		if (granted && dbuffered) {
		    best_send = active_rpcs[i];
		}
	    }
	}
    }

    // Did we find a valid RPC we should send
    if (best_send.rpc_id != 0 && !data_pkt_o.full()) {
	ap_uint<32> remaining = (HOMA_PAYLOAD_SIZE > best_send.remaining) 
	    ? ((ap_uint<32>) 0) : ((ap_uint<32>) (best_send.remaining - HOMA_PAYLOAD_SIZE));

	srpt_data_send_t send;

	send(SRPT_DATA_SEND_RPC_ID)    = best_send.rpc_id;
	send(SRPT_DATA_SEND_REMAINING) = best_send.remaining;
	send(SRPT_DATA_SEND_GRANTED)   = best_send.granted;

	data_pkt_o.write(send);

	active_rpcs[best_send.rpc_id].remaining = remaining;

	// The entry is complete so zero it
	if (remaining == 0) {
	    active_rpcs[best_send.rpc_id].rpc_id = 0;
	}
    }

    /* Find Next RPC to Fetch Data */

    srpt_data_t best_fetch;
    best_fetch.dbuffered_req = 0;

    // Brute force finding next best RPC to get data for
    for (int i = 0; i < MAX_RPCS; ++i) {

	// Check if there is data remaining to buffer
	if (active_rpcs[i].dbuffered_req != 0) {
	    if (active_rpcs[i].dbuffered_req < best_fetch.dbuffered_req || best_fetch.dbuffered_req == 0) {
		best_fetch = active_rpcs[i];
	    }
	}
    }

    if (best_fetch.dbuffered_req != 0) {
	ap_uint<32> dbuffered_req = (64 > best_fetch.dbuffered_req) 
	    ? ((ap_uint<32>) 0) : ((ap_uint<32>) (best_fetch.dbuffered_req - 64));

	active_rpcs[best_fetch.rpc_id].dbuffered_req = dbuffered_req;
	// active_rpcs[best_fetch.rpc_id]._addr   = active_rpcs[best_fetch.rpc_id].host_addr + 64;

	srpt_data_fetch_t fetch;
	fetch(SRPT_DATA_FETCH_RPC_ID)    = best_fetch.rpc_id;
	fetch(SRPT_DATA_FETCH_DBUFF_ID)  = best_fetch.dbuff_id;
	fetch(SRPT_DATA_FETCH_HOST_ADDR) = best_fetch.msg_len - best_fetch.dbuffered_req; 
	// fetch(SRPT_DATA_FETCH_HOST_ADDR) = best_fetch.host_addr;
	fetch(SRPT_DATA_FETCH_MSG_LEN)   = best_fetch.msg_len;

	cache_req_o.write(fetch);
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
	} else {
	    entries[grant_in(SRPT_GRANT_NEW_RPC_ID)].recv_bytes += HOMA_PAYLOAD_SIZE;
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

		    curr_best = (!dupe) ? entries[e] : curr_best;
		}
	    }
	    best[i] = curr_best;
	}

	srpt_grant_t next_grant = best[0];

	// Who to grant to next?
	for (int i = 0; i < 8; ++i) {
	    if (best[i].grantable_bytes < next_grant.grantable_bytes && best[i].recv_bytes != 0) {
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

	    } else {
		entries[next_grant.rpc_id].recv_bytes = 0;
		entries[next_grant.rpc_id].grantable_bytes -= MIN(next_grant.recv_bytes, entries[next_grant.rpc_id].grantable_bytes);

		srpt_grant_send_t grant_out;

		grant_out(SRPT_GRANT_SEND_MSG_ADDR)   = entries[next_grant.rpc_id].grantable_bytes;
		grant_out(SRPT_GRANT_SEND_RPC_ID)  = next_grant.rpc_id;
		// grant_out(SRPT_GRANT_SEND_PEER_ID) = next_grant.peer_id;

		grant_out_o.write(grant_out);

		if (next_grant.grantable_bytes == 0) {
		    entries[next_grant.rpc_id] = {0, 0, 0xFFFFFFFF, 0xFFFFFFFF};
		}
	    } 
	}
    }
}
