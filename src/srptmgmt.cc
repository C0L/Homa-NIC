#include "srptmgmt.hh"
#include "rpcmgmt.hh"

/**
 * WARNING: For C simulation only
 * srpt_data_pkts() - Determines what DATA packet to send next.
 * @sendmsg_i - New request or response messages from the user

 * @dbuff_notif_i - Updates about what data is held on-chip
 * @data_pkt_o - The next outgoing DATA packet that should be sent
 */
void srpt_data_pkts(hls::stream<srpt_sendmsg_t> & sendmsg_i,
		    hls::stream<srpt_dbuff_notif_t> & dbuff_notif_i,
		    hls::stream<srpt_pktq_t> & data_pkt_o,
		    hls::stream<srpt_grant_notif_t> & grant_notif_i,
		    hls::stream<srpt_sendq_t> & cache_req_o) {

  static srpt_pktq_t pktq[MAX_RPCS];
  static srpt_sendq_t sendq[MAX_RPCS];

  if (!dbuff_notif_i.empty()) {
    srpt_dbuff_notif_t dbuff_notif = dbuff_notif_i.read();
    pktq[SEND_INDEX_FROM_RPC_ID(dbuff_notif(SRPT_DBUFF_NOTIF_RPC_ID))](PKTQ_DBUFFERED) = dbuff_notif(SRPT_DBUFF_NOTIF_OFFSET);
  }

  if (!grant_notif_i.empty()) {
    srpt_grant_notif_t header_in = grant_notif_i.read();
    pktq[SEND_INDEX_FROM_RPC_ID(header_in(SRPT_GRANT_NOTIF_RPC_ID))](PKTQ_GRANTED) = header_in(SRPT_GRANT_NOTIF_OFFSET);
  }

  if (!sendmsg_i.empty()) {
    srpt_sendmsg_t sendmsg = sendmsg_i.read();

    pktq[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_SENDMSG_RPC_ID))](PKTQ_RPC_ID)    = sendmsg(SRPT_SENDMSG_RPC_ID);
    pktq[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_SENDMSG_RPC_ID))](PKTQ_DBUFF_ID)  = sendmsg(SRPT_SENDMSG_DBUFF_ID);
    pktq[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_SENDMSG_RPC_ID))](PKTQ_REMAINING) = sendmsg(SRPT_SENDMSG_MSG_LEN);
    pktq[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_SENDMSG_RPC_ID))](PKTQ_DBUFFERED) = sendmsg(SRPT_SENDMSG_MSG_LEN);
    pktq[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_SENDMSG_RPC_ID))](PKTQ_GRANTED)   = sendmsg(SRPT_SENDMSG_GRANTED);

    sendq[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_SENDMSG_RPC_ID))](SENDQ_RPC_ID)    = sendmsg(SRPT_SENDMSG_RPC_ID);
    sendq[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_SENDMSG_RPC_ID))](SENDQ_DBUFF_ID)  = sendmsg(SRPT_SENDMSG_DBUFF_ID);
    sendq[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_SENDMSG_RPC_ID))](SENDQ_OFFSET)    = sendmsg(SRPT_SENDMSG_DMA_OFFSET);
    sendq[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_SENDMSG_RPC_ID))](SENDQ_DBUFFERED) = sendmsg(SRPT_SENDMSG_MSG_LEN);
    sendq[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_SENDMSG_RPC_ID))](SENDQ_MSG_LEN)   = sendmsg(SRPT_SENDMSG_MSG_LEN);

  }

  srpt_pktq_t pkt = 0;
  pkt(PKTQ_RPC_ID)    = 0;
  pkt(PKTQ_REMAINING) = 0xFFFFFFFF;

  for (int i = 0; i < MAX_RPCS; ++i) {
    if (pktq[i](PKTQ_REMAINING) < pkt(PKTQ_REMAINING) && pktq[i](PKTQ_RPC_ID) != 0) {
      bool granted   = (pktq[i](PKTQ_GRANTED) + 1 <= pktq[i](PKTQ_REMAINING)) || (pktq[i](PKTQ_GRANTED) == 0);
      bool dbuffered = (pktq[i](PKTQ_DBUFFERED) + HOMA_PAYLOAD_SIZE <= pktq[i](PKTQ_REMAINING)) || (pktq[i](PKTQ_DBUFFERED) == 0);

      if (granted && dbuffered) {
	pkt = pktq[i];
      }
    }
  }

  if (pkt(PKTQ_RPC_ID) != 0 && !data_pkt_o.full()) {
    ap_uint<32> remaining = (HOMA_PAYLOAD_SIZE > pkt(PKTQ_REMAINING)) 
      ? ((ap_uint<32>) 0) : ((ap_uint<32>) (pkt(PKTQ_REMAINING) - HOMA_PAYLOAD_SIZE));

    data_pkt_o.write(pkt);
    pktq[SEND_INDEX_FROM_RPC_ID(pkt(PKTQ_RPC_ID))](PKTQ_REMAINING) = remaining;

    if (remaining == 0) {
      pktq[SEND_INDEX_FROM_RPC_ID(pkt(PKTQ_RPC_ID))] = 0;
    }
  }

  // TODO can clean this up
  uint32_t match_index = 0;
  srpt_sendq_t req;
  req(SENDQ_DBUFFERED) = 0;

  for (int i = 0; i < MAX_RPCS; ++i) {
    if (sendq[i](SENDQ_DBUFFERED) == 0) continue;
    if (sendq[i](SENDQ_DBUFFERED) < req(SENDQ_DBUFFERED) || req(SENDQ_DBUFFERED) == 0) {
      match_index = i;
      req = sendq[i];
    }
  }

  if (req(SENDQ_DBUFFERED) != 0 && !cache_req_o.full()) {
    ap_uint<32> dbuffered = (64 > req(SENDQ_DBUFFERED)) 
      ? ((ap_uint<32>) 0) : ((ap_uint<32>) (req(SENDQ_DBUFFERED) - 64));

    sendq[match_index](SENDQ_DBUFFERED) = dbuffered;
    sendq[match_index](SENDQ_OFFSET)    = sendq[match_index](SENDQ_OFFSET) + 64;

    cache_req_o.write(req);
	
    if (dbuffered == 0) {
      sendq[match_index] = 0;
    }
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
void srpt_grant_pkts(hls::stream<srpt_grant_in_t> & grant_in_i,
		     hls::stream<srpt_grant_out_t> & grant_out_o) {

  // Because this is only used for sim we brute force grants
  static srpt_grant_t entries[MAX_RPCS];
  static ap_uint<32> avail_bytes = OVERCOMMIT_BYTES; 

  // Headers from incoming DATA packets
  if (!grant_in_i.empty()) {
    std::cerr << "ADDED TO GRANT QUEUE\n";
    srpt_grant_in_t grant_in = grant_in_i.read();

    // The first unscheduled packet creates the entry. Only need an entry if the RPC needs grants.
    if ((grant_in(SRPT_GRANT_IN_PMAP) & PMAP_INIT) == PMAP_INIT) {
      std::cerr << "CREATE GRANT ENTRY " << RECV_INDEX_FROM_RPC_ID(grant_in(SRPT_GRANT_IN_RPC_ID)) << std::endl;
      entries[RECV_INDEX_FROM_RPC_ID(grant_in(SRPT_GRANT_IN_RPC_ID))] = {grant_in(SRPT_GRANT_IN_PEER_ID), grant_in(SRPT_GRANT_IN_RPC_ID), HOMA_PAYLOAD_SIZE, grant_in(SRPT_GRANT_IN_MSG_LEN) - HOMA_PAYLOAD_SIZE};
    } else {
      std::cerr << "UPDATED GRANT ENTRY " << RECV_INDEX_FROM_RPC_ID(grant_in(SRPT_GRANT_IN_RPC_ID)) << std::endl;
      entries[RECV_INDEX_FROM_RPC_ID(grant_in(SRPT_GRANT_IN_RPC_ID))].recv_bytes += HOMA_PAYLOAD_SIZE;
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
      srpt_grant_out_t grant_out;

      std::cerr << "VALID FOR GRANTS\n";

      if (next_grant.recv_bytes > avail_bytes) {
	// Just send avail_pkts of data
	entries[RECV_INDEX_FROM_RPC_ID(next_grant.rpc_id)].recv_bytes -= avail_bytes;
	entries[RECV_INDEX_FROM_RPC_ID(next_grant.rpc_id)].grantable_bytes -= avail_bytes;

	grant_out(SRPT_GRANT_OUT_GRANT) = avail_bytes;
	grant_out(SRPT_GRANT_OUT_RPC_ID) = next_grant.rpc_id;
	grant_out(SRPT_GRANT_OUT_PEER_ID) = next_grant.peer_id;
	std::cerr << "WROTE GRANT OUT\n";
	grant_out_o.write(grant_out);

      } else {
	entries[RECV_INDEX_FROM_RPC_ID(next_grant.rpc_id)].recv_bytes = 0;
	entries[RECV_INDEX_FROM_RPC_ID(next_grant.rpc_id)].grantable_bytes -= MIN(next_grant.recv_bytes, entries[RECV_INDEX_FROM_RPC_ID(next_grant.rpc_id)].grantable_bytes);

	srpt_grant_out_t grant_out;

	grant_out(SRPT_GRANT_OUT_GRANT)   = entries[RECV_INDEX_FROM_RPC_ID(next_grant.rpc_id)].grantable_bytes;
	grant_out(SRPT_GRANT_OUT_RPC_ID)  = next_grant.rpc_id;
	grant_out(SRPT_GRANT_OUT_PEER_ID) = next_grant.peer_id;

	grant_out_o.write(grant_out);

	if (next_grant.grantable_bytes == 0) {
	  entries[RECV_INDEX_FROM_RPC_ID(next_grant.rpc_id)] = {0, 0, 0xFFFFFFFF, 0xFFFFFFFF};
	}
      } 
    }
  }
}
