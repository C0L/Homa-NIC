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
void srpt_data_pkts(hls::stream<sendmsg_t> & sendmsg_i,
		    hls::stream<dbuff_notif_t> & dbuff_notif_i,
		    hls::stream<ready_data_pkt_t> & data_pkt_o,
		    hls::stream<header_t> & header_in_i) {

  static srpt_queue_t<srpt_data_t, MAX_SRPT> srpt_queue;
  static uint32_t grants[NUM_DBUFF];
  static srpt_data_t exhausted[NUM_DBUFF];
  static dbuff_notif_t dbuff_notifs[NUM_DBUFF];

#pragma HLS pipeline II=2

  dbuff_notif_t new_dbuff_notif;
  header_t new_header_in;
  sendmsg_t new_sendmsg;
  if (dbuff_notif_i.read_nb(new_dbuff_notif)) {
    dbuff_notifs[new_dbuff_notif.dbuff_id] = new_dbuff_notif;

    srpt_data_t exhausted_entry = exhausted[new_dbuff_notif.dbuff_id];
    uint32_t grant = grants[new_dbuff_notif.dbuff_id];

    // Is this RPC in a blocked state
    if (exhausted_entry.rpc_id != 0) {
      uint32_t remaining  = (HOMA_PAYLOAD_SIZE > exhausted_entry.remaining) ? 0 : exhausted_entry.remaining - HOMA_PAYLOAD_SIZE;
      // Even a single byte more of grant will enable one more packet of data
      bool grant_blocked = (exhausted_entry.total - grant < exhausted_entry.remaining);
      bool dbuff_blocked = ((new_dbuff_notif.dbuff_chunk + 1) * DBUFF_CHUNK_SIZE < (exhausted_entry.total - remaining));

      if (!dbuff_blocked && !grant_blocked) {
	srpt_queue.push(exhausted_entry);
	exhausted[new_dbuff_notif.dbuff_id].rpc_id = 0;
      } 
    }
  } else if (header_in_i.read_nb(new_header_in)) {
    // Incoming grants
    grants[new_header_in.dbuff_id] = new_header_in.grant_offset;

    srpt_data_t exhausted_entry = exhausted[new_header_in.dbuff_id];
    uint32_t grant = new_header_in.grant_offset;
    dbuff_notif_t dbuff_notif = dbuff_notifs[new_header_in.dbuff_id];

    // Is this RPC in a blocked state
    if (exhausted_entry.rpc_id != 0) {
      uint32_t remaining  = (HOMA_PAYLOAD_SIZE > exhausted_entry.remaining) ? 0 : exhausted_entry.remaining - HOMA_PAYLOAD_SIZE;
      // Even a single byte more of grant will enable one more packet of data
      bool granted = (grant >= (exhausted_entry.total - exhausted_entry.remaining));

      // Regardless, we need to have enough packet data on hand to send this message
      bool unblocked = (((dbuff_notif.dbuff_chunk+1) * DBUFF_CHUNK_SIZE) >= (exhausted_entry.total - remaining));

      if (unblocked && granted) {
	srpt_queue.push(exhausted_entry);
	exhausted[dbuff_notif.dbuff_id].rpc_id = -1;
      } 
    }
  } else if (sendmsg_i.read_nb(new_sendmsg)) {
    srpt_data_t new_entry = {new_sendmsg.local_id, new_sendmsg.dbuff_id, new_sendmsg.length, new_sendmsg.length};
    grants[new_entry.rpc_id] = new_sendmsg.granted;

    dbuff_notif_t dbuff_notif = dbuff_notifs[new_sendmsg.dbuff_id];

    bool dbuff_blocked = ((dbuff_notif.dbuff_chunk + 1) * DBUFF_CHUNK_SIZE) < HOMA_PAYLOAD_SIZE;

    if (dbuff_blocked) {
      exhausted[new_sendmsg.local_id] = new_entry;
    } else {
      srpt_queue.push(new_entry);
    }
  } else if (!data_pkt_o.full() && !srpt_queue.empty()) {
    srpt_data_t & head = srpt_queue.head();

    uint32_t remaining  = (HOMA_PAYLOAD_SIZE > head.remaining) ? 0 : head.remaining - HOMA_PAYLOAD_SIZE;

    // Check if the offset of the highest byte needed has been received
    dbuff_notif_t dbuff_notif = dbuff_notifs[head.dbuff_id];
    uint32_t grant = grants[head.rpc_id];

    data_pkt_o.write({head.rpc_id, head.dbuff_id, head.remaining, head.total, grant});

    head.remaining = remaining;

    /*
      grant is the byte up to which we are eligable to send
      head.total - grant determines the maximum number of
      remaining bytes we can send up to
    */
    bool grant_blocked = (head.total - grant < head.remaining);
    bool dbuff_blocked = ((dbuff_notif.dbuff_chunk + 1) * DBUFF_CHUNK_SIZE < (head.total - remaining));
    bool complete  = (head.remaining == 0);

    if (!complete && (grant_blocked || dbuff_blocked)) {
      exhausted[head.rpc_id] = head;
    }

    // Are we done sending this message/out of granted bytes/out of data on-chip
    if (complete || grant_blocked || dbuff_blocked) {
      // TODO also wipe the dbuff notifs (so that a future message does not pickup garbage data)
      srpt_queue.pop();
    }
  } 
}


//void srpt_data_pkts(hls::stream<sendmsg_t> & sendmsg_i,
//		    hls::stream<dbuff_notif_t> & dbuff_notif_i,
//		    hls::stream<ready_data_pkt_t> & data_pkt_o,
//		    hls::stream<header_t> & header_in_i) {
//
//  static srpt_queue_t<srpt_data_t, MAX_SRPT> srpt_queue;
//  static uint32_t grants[MAX_RPCS];
//  static dbuff_notif_t dbuff_notifs[NUM_DBUFF];
//  // static blocked rpcs...
//  static fifo_t<srpt_data_t, MAX_SRPT> exhausted;
//
//#pragma HLS pipeline II=1
//
//  if (!dbuff_notif_i.empty()) {
//    dbuff_notif_t dbuff_notif = dbuff_notif_i.read();
//    dbuff_notifs[dbuff_notif.dbuff_id] = dbuff_notif;
//  }
//
//  if (!data_pkt_o.full() && !srpt_queue.empty()) {
//    srpt_data_t & head = srpt_queue.head();
//
//    uint32_t remaining  = (HOMA_PAYLOAD_SIZE > head.remaining) ? 0 : head.remaining - HOMA_PAYLOAD_SIZE;
//
//    // Check if the offset of the highest byte needed has been received
//    dbuff_notif_t dbuff_notif = dbuff_notifs[head.dbuff_id];
//    bool blocked = ((dbuff_notif.dbuff_chunk + 1) * DBUFF_CHUNK_SIZE < (head.total - remaining));
//
//    if (blocked) {
//      exhausted.insert(head);
//      srpt_queue.pop();
//    } else {
//      uint32_t grant = grants[head.rpc_id];
//      data_pkt_o.write({head.rpc_id, head.dbuff_id, head.remaining, head.total, grant});
//
//      head.remaining = remaining;
//
//      /*
//	grant is the byte up to which we are eligable to send
//	head.total - grant determines the maximum number of
//	remaining bytes we can send up to
//      */
//      bool ungranted = (head.total - grant < head.remaining);
//      bool complete  = (head.remaining == 0);
//
//      if (!complete && ungranted) {
//	exhausted.insert(head);
//      }
//
//      // Are we done sending this message, or are we out of granted bytes?
//      if (complete || ungranted) {
//	// TODO also wipe the dbuff notifs (so that a future message does not pickup garbage data)
//	srpt_queue.pop();
//      }
//    }
//  } else if (!header_in_i.empty()) {
//    // Incoming grants
//    header_t header_in = header_in_i.read();
//    grants[header_in.local_id] = header_in.grant_offset;
//
//    // TODO maybe combine this process with the sendmsg somehow?? 
//  } else if (!sendmsg_i.empty()) {
//    sendmsg_t sendmsg = sendmsg_i.read();
//    srpt_data_t new_entry = {sendmsg.local_id, sendmsg.dbuff_id, sendmsg.length, sendmsg.length};
//    grants[new_entry.rpc_id] = sendmsg.granted;
//    srpt_queue.push(new_entry);
//  } else if (!exhausted.empty()) {
//    srpt_data_t exhausted_entry = exhausted.remove();
//    uint32_t grant = grants[exhausted_entry.rpc_id];
//    dbuff_notif_t dbuff_notif = dbuff_notifs[exhausted_entry.dbuff_id];
//    uint32_t remaining  = (HOMA_PAYLOAD_SIZE > exhausted_entry.remaining) ? 0 : exhausted_entry.remaining - HOMA_PAYLOAD_SIZE;
//
//    // Even a single byte more of grant will enable one more packet of data
//    bool granted = (grant >= (exhausted_entry.total - exhausted_entry.remaining));
//
//    // Regardless, we need to have enough packet data on hand to send this message
//    bool unblocked = (((dbuff_notif.dbuff_chunk+1) * DBUFF_CHUNK_SIZE) >= (exhausted_entry.total - remaining));
//
//    if (unblocked && granted) {
//      srpt_queue.push(exhausted_entry);
//    } else {
//      exhausted.insert(exhausted_entry);
//    }
//  }
//}

/**
 * srpt_grant_pkts() - Determines what message to grant to next. Messages are ordered by
 * increasing number of bytes not yet granted. When grants are made, the peer is
 * stored in the active set. Grants will not be issued to peers that are in the active set.
 * Maintains RTTBytes per mesage in the active set at a time if possible.
 * @header_in_i: Injest path for new messages that will need to be granted to.
 * @grant_pkt_o: The next rpc that should be granted to. Goes to packet egress process.
 *
 * TODO There is significantly greater complexity in the grant process in linux Homa.
 * Some of that may be needed here?
 */
void srpt_grant_pkts(hls::stream<header_t> & header_in_i,
		     hls::stream<ready_grant_pkt_t> & grant_pkt_o) {

  static srpt_grant_t active_set[MAX_OVERCOMMIT];
  static fifo_id_t<ap_uint<3>, MAX_OVERCOMMIT> active_set_fifo(true);
  static srpt_queue_t<srpt_grant_t, MAX_SRPT> srpt_queue[MAX_OVERCOMMIT];
  static uint32_t recv_bytes[MAX_RPCS];
  static uint32_t message_lengths[MAX_RPCS];

#pragma HLS dependence variable=recv_bytes inter RAW false

#pragma HLS pipeline II=2
  
  /* SRPT Grant Algorithm:
   * There are MAX_OVERCOMMIT active peers, and MAX_OVERCOMMIT SRPT queues (one for each active peer)
   *
   * New Grant:
   *   Only need to add new entries for first packet of unscheduled bytes (they will remain until fully granted)
   *   Construct a new SRPT entry with the RPC ID, peer ID, total bytes, granted bytes, and empty flag
   *   Check whether the peer ID of the new header matches any of the peers in the active set
   *   If there is a match
   *     Insert the new entry into every SRPT queue with a flag of BLOCKED except the SRPT corresponding to the
   *     entry in the active set that has the same peer as the incoming header, which should have a flag of ACTIVE
   *   else
   *     Insert the new entry into every SRPT queue with a flag of ACTIVE
   *
   * Incoming Data:
   *   We assume that the RPC for incoming data has already been added to the queue. When new data arrives, subtract
   *   the number of bytes received from the value in the active_bytes array for the respective RPC. If the active_bytes
   *   array falls to zero (indicating that the grant has been completed), then clear the entry from the active set
   *   which shares the same peer ID (which must have been the entry that initially granted to this data for it to be
   *   received, as this data is not the unscheduled bytes). We then submit SRPT_UNBLOCK messages to each of the SRPT
   *   queues that are not the respective queue of the active set entry that we just cleared. Refilling this spot will be
   *   filled in the next cycle. 
   *
   *
   * Outgoing Grant:
   *   When there is an open element in the active set, grab the entry (but not pop) the entry from the head
   *   of the corresponding SRPT queue. Place this head entry in the open active set entry, and send an outgoing
   *   message out of the core indicating a grant packet should be sent. Then, create a new entry that is the same as
   *   that head entry, but with the granted value incremented by GRANT_SIZE. For every SRPT queue THAT IS NOT THE
   *   SRPT queue that corresponds to the open element we just filled, set the flag value of this entry to UPDATE_BLOCK
   *   which will update the relative (but blocked) value (as matched by RPC ID) in each SRPT queue. For the one SRPT
   *   queue that corresponds to the active set entry we just filled, queue the same new entry, but with a flag value
   *   of just UPDATE.
   *
   * Completed Grant:
   *   When the granted bytes reaches the total meassage length, the entry will fall to the back of the SRPT queue
   *   and eventually off the edge. This is accomplished by the comparator between the shift registers.
   *   This cleans up completed grants.
   *
   */

  // Headers from incoming DATA packets

  header_t header_in;
  //if (!header_in_i.empty()) {

  if (!header_in_i.read_nb(header_in)) {
  //header_t header_in = header_in_i.read();

    // Is this peer actively granted to?
    int peer_match = -1;
    int rpc_match = -1;
    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
#pragma HLS unroll
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
#pragma HLS unroll
	    // The initial number of grantable bytes is what is remaining after what has already been granted has been sent 
	    srpt_grant_t new_grantable = {header_in.peer_id, header_in.local_id, header_in.message_length - header_in.incoming, (peer_match == i) ? SRPT_ACTIVE : SRPT_BLOCKED};
	    srpt_queue[i].push(new_grantable);
	  }
		  
	  // If there is no peer map match, insert all as active
	} else {
	  for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
#pragma HLS unroll
	    srpt_grant_t new_grantable = {header_in.peer_id, header_in.local_id, header_in.message_length - header_in.incoming, SRPT_ACTIVE};
	    srpt_queue[i].push(new_grantable);
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
#pragma HLS unroll
	      srpt_queue[i].push(invalidate);
	    }

	    // Free the current peer ID
	    active_set_fifo.insert(peer_match);
	  } else if (recv_bytes[header_in.local_id] > GRANT_REFRESH) {
	    
	    srpt_grant_t unblock = {header_in.peer_id, header_in.local_id, 0, SRPT_UNBLOCK};
	    
	    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
#pragma HLS unroll
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

    srpt_grant_t & head = srpt_queue[idx].head();

    uint32_t new_grant = head.grantable + recv_bytes[head.rpc_id];

    if (head.priority == SRPT_ACTIVE) {

      // TODO this should be triggered by INCOMING packets
      //if (new_grant >= message_lengths[head.rpc_id]) { // Have we fully granted???

	// No need to grant more than the message length (maybe uneeded)
	//new_grant = message_lengths[head.rpc_id];

      //	srpt_grant_t invalidate = {head.peer_id, head.rpc_id, 0, SRPT_INVALIDATE}; 
      //	for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
      //#pragma HLS unroll
      //	  srpt_queue[i].push(invalidate);
      //	}
	//} else {
	srpt_grant_t update = {head.peer_id, head.rpc_id, new_grant, SRPT_UPDATE};
	srpt_queue[idx].push(update);
	
	srpt_grant_t update_block = {head.peer_id, head.rpc_id, new_grant, SRPT_UPDATE_BLOCK}; 
	for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
#pragma HLS unroll
	  if (i != idx) srpt_queue[i].push(update_block);
	}
	//}

      active_set[idx] = {head.peer_id, head.rpc_id, new_grant, SRPT_ACTIVE};

      // Create a new grant packet for the last offset + how many bytes we have recieved
      grant_pkt_o.write({head.peer_id, head.rpc_id, new_grant});
    // There is nothing grantable from this SRPT queue, so lets add that ID to the back of the queue
    } else {
      active_set_fifo.insert(idx);
    }
  } else {
    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
#pragma HLS unroll
      srpt_queue[i].order();
    }
  }
}
