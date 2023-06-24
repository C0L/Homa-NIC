#include "srptmgmt.hh"
#include "rpcmgmt.hh"

/**
 * srpt_data_pkts() - Determines what DATA packet to send next.
 * @sendmsg_i - New request or response messages from the user
 * @dbuff_notif_i - Updates about what data is held on-chip
 * @data_pkt_o - The next outgoing DATA packet that should be sent
 *
 * TODO this should be II=2 probably with the first cycle for write
 * second cycle for read
 */


//void srpt_data_pkts(hls::stream<sendmsg_t> & sendmsg_i,
//		    hls::stream<dbuff_notif_t> & dbuff_notif_i,
//		    hls::stream<ready_data_pkt_t> & data_pkt_o,
//		    hls::stream<header_t> & header_in_i) {
//
//  static srpt_queue_t<srpt_data_t, MAX_SRPT> srpt_queue;
//  static ap_uint<32> grants[NUM_DBUFF];
//  static srpt_data_t exhausted[NUM_DBUFF];
//  static dbuff_notif_t dbuff_notifs[NUM_DBUFF];
//
//#pragma HLS pipeline II=2
//
//  dbuff_notif_t new_dbuff_notif;
//  header_t new_header_in;
//  sendmsg_t new_sendmsg;
//  if (dbuff_notif_i.read_nb(new_dbuff_notif)) {
//    std::cerr << "RECV DATABUFF\n";
//    dbuff_notifs[new_dbuff_notif.dbuff_id] = new_dbuff_notif;
//
//    srpt_data_t exhausted_entry = exhausted[new_dbuff_notif.dbuff_id];
//    ap_uint<32> grant = grants[new_dbuff_notif.dbuff_id];
//
//    // Is this RPC in a blocked state
//    if (exhausted_entry.rpc_id != 0) {
//      ap_uint<32> remaining  = (HOMA_PAYLOAD_SIZE > exhausted_entry.remaining) ? ((ap_uint<32>) 0) : (ap_uint<32>) (exhausted_entry.remaining - HOMA_PAYLOAD_SIZE);
//      // Even a single byte more of grant will enable one more packet of data
//      bool grant_blocked = (exhausted_entry.total - grant < exhausted_entry.remaining);
//      bool dbuff_blocked = ((new_dbuff_notif.dbuff_chunk + 1) * DBUFF_CHUNK_SIZE < (exhausted_entry.total - remaining));
//
//      if (!dbuff_blocked && !grant_blocked) {
//	srpt_queue.push(exhausted_entry);
//	exhausted[new_dbuff_notif.dbuff_id].rpc_id = 0;
//      } 
//    }
//  } else if (header_in_i.read_nb(new_header_in)) {
//    std::cerr << "HEADER IN\n";
//    // Incoming grants
//    grants[new_header_in.dbuff_id] = new_header_in.grant_offset;
//
//    srpt_data_t exhausted_entry = exhausted[new_header_in.dbuff_id];
//    ap_uint<32> grant = new_header_in.grant_offset;
//    dbuff_notif_t dbuff_notif = dbuff_notifs[new_header_in.dbuff_id];
//
//    // Is this RPC in a blocked state
//    if (exhausted_entry.rpc_id != 0) {
//      ap_uint<32> remaining  = (HOMA_PAYLOAD_SIZE > exhausted_entry.remaining) ? (ap_uint<32>) 0 : (ap_uint<32>) (exhausted_entry.remaining - HOMA_PAYLOAD_SIZE);
//      // Even a single byte more of grant will enable one more packet of data
//      bool granted = (grant >= (exhausted_entry.total - exhausted_entry.remaining));
//
//      // Regardless, we need to have enough packet data on hand to send this message
//      bool unblocked = (((dbuff_notif.dbuff_chunk+1) * DBUFF_CHUNK_SIZE) >= (exhausted_entry.total - remaining));
//
//      if (unblocked && granted) {
//	srpt_queue.push(exhausted_entry);
//	exhausted[dbuff_notif.dbuff_id].rpc_id = -1;
//      } 
//    }
//  } else if (sendmsg_i.read_nb(new_sendmsg)) {
//    std::cerr << "SEND MSG INTO SRPT QUEUE\n";
//    srpt_data_t new_entry = {new_sendmsg.local_id, new_sendmsg.dbuff_id, new_sendmsg.length, new_sendmsg.length};
//    grants[new_entry.rpc_id] = new_sendmsg.granted;
//
//    dbuff_notif_t dbuff_notif = dbuff_notifs[new_sendmsg.dbuff_id];
//
//    bool dbuff_blocked = ((dbuff_notif.dbuff_chunk + 1) * DBUFF_CHUNK_SIZE) < HOMA_PAYLOAD_SIZE;
//
//    if (dbuff_blocked) {
//      std::cerr << "DBUFF BLOCKED\n";
//      exhausted[new_sendmsg.local_id] = new_entry;
//    } else {
//      std::cerr << "SRPT PUSH\n";
//      srpt_queue.push(new_entry);
//    }
//  } else if (!data_pkt_o.full() && !srpt_queue.empty()) {
//    std::cerr << "SELECTED DATA PACKET TO SEND\n";
//    srpt_data_t & head = srpt_queue.head();
//
//    ap_uint<32> remaining  = (HOMA_PAYLOAD_SIZE > head.remaining) ? (ap_uint<32>) 0 : (ap_uint<32>) (head.remaining - HOMA_PAYLOAD_SIZE);
//
//    // Check if the offset of the highest byte needed has been received
//    dbuff_notif_t dbuff_notif = dbuff_notifs[head.dbuff_id];
//    ap_uint<32> grant = grants[head.rpc_id];
//
//    data_pkt_o.write({head.rpc_id, head.dbuff_id, head.remaining, head.total, grant});
//
//    head.remaining = remaining;
//
//    /*
//      grant is the byte up to which we are eligable to send
//      head.total - grant determines the maximum number of
//      remaining bytes we can send up to
//    */
//    bool grant_blocked = (head.total - grant < head.remaining);
//    bool dbuff_blocked = ((dbuff_notif.dbuff_chunk + 1) * DBUFF_CHUNK_SIZE < (head.total - remaining));
//
//    std::cerr << "GRANT BLOCKED " << grant_blocked << " dbuff blocked " << dbuff_blocked << std::endl;
//
//    bool complete  = (head.remaining == 0);
//
//    if (!complete && (grant_blocked || dbuff_blocked)) {
//      exhausted[head.rpc_id] = head;
//    }
//
//    // Are we done sending this message/out of granted bytes/out of data on-chip
//    if (complete || grant_blocked || dbuff_blocked) {
//      // TODO also wipe the dbuff notifs (so that a future message does not pickup garbage data)
//      srpt_queue.pop();
//    }
//  } 
//}


void srpt_data_pkts(hls::stream<sendmsg_t> & sendmsg_i,
		    hls::stream<dbuff_notif_t> & dbuff_notif_i,
		    hls::stream<ready_data_pkt_t> & data_pkt_o,
		    hls::stream<header_t> & header_in_i) {

  static srpt_queue_t<srpt_data_t, MAX_SRPT> srpt_queue;
  static uint32_t grants[MAX_RPCS];
  static dbuff_notif_t dbuff_notifs[NUM_DBUFF];
  // static blocked rpcs...
  static fifo_t<srpt_data_t, MAX_SRPT> exhausted;

#pragma HLS pipeline II=1

  if (!dbuff_notif_i.empty()) {
    dbuff_notif_t dbuff_notif = dbuff_notif_i.read();
    dbuff_notifs[dbuff_notif.dbuff_id] = dbuff_notif;
  }

  if (!data_pkt_o.full() && !srpt_queue.empty()) {
    //std::cerr << "HEADER OUT CHECK\n";
    srpt_data_t & head = srpt_queue.head();

    uint32_t remaining  = (HOMA_PAYLOAD_SIZE > head.remaining) ? ((ap_uint<32>) 0) : ((ap_uint<32>) (head.remaining - HOMA_PAYLOAD_SIZE));

    // Check if the offset of the highest byte needed has been received
    dbuff_notif_t dbuff_notif = dbuff_notifs[head.dbuff_id];
    bool blocked = ((dbuff_notif.dbuff_chunk + 1) * DBUFF_CHUNK_SIZE < (head.total - remaining));

    if (blocked) {
      exhausted.insert(head);
      srpt_queue.pop();
      //std::cerr << "HEADER OUT BLOCKED\n";
    } else {

      //std::cerr << "HEADER OUT SEND\n";
      uint32_t grant = grants[head.rpc_id];
      data_pkt_o.write({head.rpc_id, head.dbuff_id, head.remaining, head.total, grant});

      head.remaining = remaining;

      /*
	grant is the byte up to which we are eligable to send
	head.total - grant determines the maximum number of
	remaining bytes we can send up to
      */
      bool ungranted = (head.total - grant < head.remaining);
      bool complete  = (head.remaining == 0);

      if (!complete && ungranted) {
	exhausted.insert(head);
      }

      // Are we done sending this message, or are we out of granted bytes?
      if (complete || ungranted) {
	// TODO also wipe the dbuff notifs (so that a future message does not pickup garbage data)
	srpt_queue.pop();
      }
    }
  } else if (!header_in_i.empty()) {
    // Incoming grants
    header_t header_in = header_in_i.read();
    grants[header_in.local_id] = header_in.grant_offset;

    // TODO maybe combine this process with the sendmsg somehow?? 
  } else if (!sendmsg_i.empty()) {
    sendmsg_t sendmsg = sendmsg_i.read();
    srpt_data_t new_entry = {sendmsg.local_id, sendmsg.dbuff_id, sendmsg.length, sendmsg.length};
    grants[new_entry.rpc_id] = sendmsg.granted;
    srpt_queue.push(new_entry);
  } else if (!exhausted.empty()) {

    srpt_data_t exhausted_entry = exhausted.remove();
    uint32_t grant = grants[exhausted_entry.rpc_id];
    dbuff_notif_t dbuff_notif = dbuff_notifs[exhausted_entry.dbuff_id];
    uint32_t remaining  = (HOMA_PAYLOAD_SIZE > exhausted_entry.remaining) ? ((ap_uint<32>) 0) : ((ap_uint<32>) (exhausted_entry.remaining - HOMA_PAYLOAD_SIZE));

    // Even a single byte more of grant will enable one more packet of data
    bool granted = (grant >= (exhausted_entry.total - exhausted_entry.remaining));

    // Regardless, we need to have enough packet data on hand to send this message
    bool unblocked = (((dbuff_notif.dbuff_chunk+1) * DBUFF_CHUNK_SIZE) >= (exhausted_entry.total - remaining));

    if (unblocked && granted) {
      srpt_queue.push(exhausted_entry);
    } else {
      exhausted.insert(exhausted_entry);
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
void srpt_grant_pkts(hls::stream<ap_uint<124>> & header_in_i,
		     hls::stream<ap_uint<95>> & grant_pkt_o) {

  static srpt_queue_t<srpt_grant_t, MAX_SRPT> srpt_queue[MAX_OVERCOMMIT];
  static srpt_grant_t active_set[MAX_OVERCOMMIT];
  static fifo_id_t<ap_uint<3>, MAX_OVERCOMMIT> active_set_fifo(true);
  static ap_uint<32> recv_bytes[MAX_RPCS];
  static ap_uint<32> message_lengths[MAX_RPCS];

  // Headers from incoming DATA packets
  ap_uint<124> header_in_raw;

  if (!header_in_i.read_nb(header_in_raw)) {
    header_t header_in;

    header_in.data_offset = header_in_raw(31, 0);
    header_in.incoming = header_in_raw(63, 2);
    header_in.message_length = header_in_raw(95, 64);
    header_in.local_id = header_in_raw(109, 96);
    header_in.peer_id = header_in_raw(123, 110);

    // Is this peer actively granted to?
    int peer_match = -1;
    int rpc_match = -1;
    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
      if (active_set[i].peer_id == header_in.peer_id) {
	peer_match = i;
      }

      if (active_set[i].rpc_id == header_in.local_id) {
	rpc_match = i;
      }
    }

    if (header_in.message_length > header_in.rtt_bytes) {

      // The first unscheduled packet creates the entry. Only need an entry if the RPC needs grants.
      if (header_in.data_offset == 0) {
	
	message_lengths[header_in.local_id] = header_in.message_length;
	
	// If there is a peer map match, insert BLOCKED entries except for match ID
	if (peer_match != -1) {
	  for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
	    // The initial number of grantable bytes is what is remaining after what has already been granted has been sent 
	    srpt_grant_t new_grantable = {header_in.peer_id, header_in.local_id, header_in.message_length - header_in.incoming, (peer_match == i) ? SRPT_ACTIVE : SRPT_BLOCKED};
	    //srpt_queue[i].push(new_grantable);
	  }
		  
	  // If there is no peer map match, insert all as active
	} else {
	  for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
	    srpt_grant_t new_grantable = {header_in.peer_id, header_in.local_id, header_in.message_length - header_in.incoming, SRPT_ACTIVE};
	    //srpt_queue[i].push(new_grantable);
	  }
	}
	
	recv_bytes[header_in.local_id] = header_in.segment_length; 
	
      // Other received data bytes
      } else {
	// Increase the number of bytes we will grant when the GRANT is sent
	recv_bytes[header_in.local_id] += header_in.segment_length;
	
	if (peer_match != -1 && rpc_match != -1)  {
	  /*
	   * Have we received all the bytes from an active grant?
	   * Have we accumulated enough data bytes to justify sending another grant packet?
	   * This avoids sending 1 GRANT for every DATA packet
	   */
	  if (header_in.message_length == header_in.data_offset + header_in.segment_length) {
	    
	    srpt_grant_t invalidate = {header_in.peer_id, header_in.local_id, 0, SRPT_INVALIDATE}; 
	    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
	      srpt_queue[i].push(invalidate);
	    }

	    // Free the current peer ID
	    active_set_fifo.insert(peer_match);
	  } else if (recv_bytes[header_in.local_id] > GRANT_REFRESH) {
	    
	    srpt_grant_t unblock = {header_in.peer_id, header_in.local_id, 0, SRPT_UNBLOCK};
	    
	    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
	      if (peer_match != i) srpt_queue[i].push(unblock);
	    }
	    
	    // Free the current peer ID
	    active_set_fifo.insert(peer_match);
	  } 
	}
      }
    }
  // Is there room for more grants in the output FIFO, and are there open slots in the active set
  } else if (!grant_pkt_o.full() && !active_set_fifo.empty()) {
    
    ap_uint<3> idx = active_set_fifo.remove();

    //ap_uint<95> & head = srpt_queue[idx].head();
    srpt_grant_t & head = srpt_queue[idx].head();

    ap_uint<32> new_grant = head.grantable + recv_bytes[head.rpc_id];

    srpt_grant_t update = {head.peer_id, head.rpc_id, new_grant, SRPT_UPDATE};
    srpt_queue[idx].push(update);
	
    srpt_grant_t update_block = {head.peer_id, head.rpc_id, new_grant, SRPT_UPDATE_BLOCK}; 
    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
      //if (i != idx) srpt_queue[i].push(update_block);
    }

    active_set[idx] = {head.peer_id, head.rpc_id, new_grant, SRPT_ACTIVE};

    // Create a new grant packet for the last offset + how many bytes we have recieved

    ap_uint<95> ready_grant_pkt;
    ready_grant_pkt(2, 0) = 0;
    ready_grant_pkt(34, 3) = head.grantable;
    ready_grant_pkt(48, 35) = head.rpc_id;
    ready_grant_pkt(62, 49) = head.peer_id;
    ready_grant_pkt(94, 63) = 0;
    grant_pkt_o.write(ready_grant_pkt);
    //grant_pkt_o.write({head.peer_id, head.rpc_id, new_grant});
    active_set_fifo.insert(idx);
  } //else {
    //    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
    //#pragma HLS unroll
    //      //srpt_queue[i].order();
    //    }
    //  }
}
