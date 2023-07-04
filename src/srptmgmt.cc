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

   //if (!dbuff_notif_i.empty()) {
   if (dbuff_notif_i.size() > 0) {
      dbuff_notif_t dbuff_notif = dbuff_notif_i.read();
      dbuff_notifs[dbuff_notif.dbuff_id] = dbuff_notif;
       std::cerr << "DATA BUFFER IN " << (dbuff_notif.dbuff_chunk + 1) * DBUFF_CHUNK_SIZE << std::endl;
   }

   //if (!data_pkt_o.full() && !srpt_queue.empty()) {
   if (data_pkt_o.size() <= 1 && !srpt_queue.empty()) {
      srpt_data_t & head = srpt_queue.head();

      ap_uint<32> remaining  = (HOMA_PAYLOAD_SIZE > head.remaining) ? ((ap_uint<32>) 0) : ((ap_uint<32>) (head.remaining - HOMA_PAYLOAD_SIZE));

      // Check if the offset of the highest byte needed has been received
      dbuff_notif_t dbuff_notif = dbuff_notifs[head.dbuff_id];
      bool blocked = ((dbuff_notif.dbuff_chunk + 1) * DBUFF_CHUNK_SIZE < (head.total - remaining));

      if (blocked) {
         exhausted.insert(head);
         srpt_queue.pop();
         std::cerr << "HEADER OUT BLOCKED\n";
      } else {

         std::cerr << "HEADER OUT SEND\n";
         ap_uint<32> grant = grants[head.rpc_id];
         data_pkt_o.write({head.rpc_id, head.dbuff_id, head.remaining, head.total, grant});

         head.remaining = remaining;

         std::cerr << "REMAINING: " << head.remaining << std::endl;
         std::cerr << "TOTAL: " << head.total << std::endl;
         std::cerr << "GRANT: " << grant << std::endl;

         /*
            grant is the byte up to which we are eligable to send
            head.total - grant determines the maximum number of
            remaining bytes we can send up to
            */

         bool ungranted = (head.total - head.remaining > grant);
         // bool ungranted = (head.total - grant < head.remaining);
         bool complete  = (head.remaining == 0);

         if (!complete && ungranted) {
            std::cerr << "REBLOCKED\n";
            exhausted.insert(head);
         }

         // Are we done sending this message, or are we out of granted bytes?
         if (complete || ungranted) {
            // TODO also wipe the dbuff notifs (so that a future message does not pickup garbage data)
            srpt_queue.pop();
         }
      }
   } else if (header_in_i.size() > 0) {
      std::cerr << "RECEIVED GRANTS\n";
      // Incoming grants
      header_t header_in = header_in_i.read();
      grants[header_in.local_id] = header_in.grant_offset;

      // TODO maybe combine this process with the sendmsg somehow?? 
   } else if (sendmsg_i.size() > 0) {
      sendmsg_t sendmsg = sendmsg_i.read();

      std::cerr << "ON BOARD NEW MESSAGE " << sendmsg.length << " " << sendmsg.granted << std::endl;
      srpt_data_t new_entry = {sendmsg.local_id, sendmsg.dbuff_id, sendmsg.length, sendmsg.length};
      grants[new_entry.rpc_id] = sendmsg.granted;
      srpt_queue.push(new_entry);
   } else if (!exhausted.empty()) {

      srpt_data_t exhausted_entry = exhausted.remove();
      ap_uint<32> grant = grants[exhausted_entry.rpc_id];
      dbuff_notif_t dbuff_notif = dbuff_notifs[exhausted_entry.dbuff_id];
      ap_uint<32> remaining  = (HOMA_PAYLOAD_SIZE > exhausted_entry.remaining) ? ((ap_uint<32>) 0) : ((ap_uint<32>) (exhausted_entry.remaining - HOMA_PAYLOAD_SIZE));

      // Even a single byte more of grant will enable one more packet of data
      // bool granted = (grant >= (exhausted_entry.total - exhausted_entry.remaining));

      bool granted = !(exhausted_entry.total - exhausted_entry.remaining > grant);

      // Regardless, we need to have enough packet data on hand to send this message
      bool unblocked = (((dbuff_notif.dbuff_chunk+1) * DBUFF_CHUNK_SIZE) >= (exhausted_entry.total - remaining));

      if (unblocked && granted) {
         std::cerr << "UNBLOCKED\n";
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
void srpt_grant_pkts(hls::stream<ap_uint<58>> & header_in_i,
                     hls::stream<ap_uint<51>> & grant_pkt_o) {

   // Because this is only used for sim we brute force grants
   static srpt_grant_t entries[MAX_RPCS];
   static ap_uint<32> avail_pkts = OVERCOMMIT_PKTS; 

   // Headers from incoming DATA packets
   //ap_uint<58> header_in_raw;

   //if (!header_in_i.read_nb(header_in_raw)) {
   //   header_t header_in;
   //
   //   header_in.peer_id = header_in_raw(57, 44);
   //   header_in.local_id = header_in_raw(43, 30);
   //   header_in.message_length = header_in_raw(29, 20);
   //   header_in.incoming = header_in_raw(19, 10);
   //   header_in.data_offset = header_in_raw(9, 0);

   //   if (header_in.message_length > RTT_PKTS) {

   //      // The first unscheduled packet creates the entry. Only need an entry if the RPC needs grants.
   //      if (header_in.data_offset == 0) {
   //         entries[header_in.local_id] = {header_in.peer_id, header_in.local_id, 1, header_in.message_length - 1};
   //      } else {
   //         entries[header_in.local_id].recv_pkts += 1;
   //         entries[header_in.local_id].grantable_pkts -= 1;
   //      } 
   //   }
   //} else if (!grant_pkt_o.full()) {

   //   ap_uint<51> grant_pkt;
   //
   //   srpt_grant_t best[8];

   //   // All slots begin empty
   //   for (int i = 0; i < 8; ++i)  {
   //      best[i] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
   //   }

   //   // Check all the RPCs we need to grant to 
   //   for (int i = 0; i < MAX_RPCS; ++i) { 
   //     // Is this peer already in our best list?
   //      int match = -1;
   //      int max_entry = 0;
   //      for (int l = 0; l < 8; ++l) {
   //         if (best[max_entry].grantable_pkts < best[l].grantable_pkts) {
   //            max_entry = l;
   //         }

   //         if (best[l].rpc_id == entries[i].rpc_id) {
   //            match = l;
   //         }
   //      }

   //      // If we matched, should we unseat the matched entry?
   //      if (match != -1) {
   //         if (best[match].grantable_pkts > entries[i].grantable_pkts) {
   //            best[match] = entries[i];
   //         }
   //      } else {
   //         if (best[max_entry].grantable_pkts > entries[i].grantable_pkts) {
   //            best[max_entry] = entries[i]; 
   //         }
   //      }
   //   }

   //   int min_entry = 0;
   //   for (int l = 0; l < 8; ++l) {
   //      if (best[min_entry].grantable_pkts < best[l].grantable_pkts 
   //         && best[l].recv_pkts > 0) {
   //         min_entry = l;
   //      }
   //   }

   //   if (best[min_entry].recv_pkts > 0
   //      && best[min_entry].rpc_id != 0xFFFFFFFF) {
   //      ap_uint<51> grant_pkt;

   //      if (best[min_entry].recv_pkts > avail_pkts) {
   //         // Just send avail_pkts of data

   //         entries[best[min_entry].rpc_id].recv_pkts -= avail_pkts;
   //         entries[best[min_entry].rpc_id].grantable_pkts -= avail_pkts;

   //         grant_pkt(2, 0)   = 0;
   //         grant_pkt(12, 3)  = 0;
   //         grant_pkt(22, 13) = avail_pkts;
   //         grant_pkt(36, 23) = best[min_entry].rpc_id;
   //         grant_pkt(50, 37) = best[min_entry].peer_id;

   //         grant_pkt_o.write(grant_pkt);

   //      } else { 
   //         // Is this going to result in a fully granted message?
   //         if ((best[min_entry].grantable_pkts - best[min_entry].recv_pkts) == 0) { 
   //            entries[best[min_entry].rpc_id] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}; 
   //         }
   //         
   //         entries[best[min_entry].rpc_id].recv_pkts = 0;
   //         entries[best[min_entry].rpc_id].grantable_pkts -= best[min_entry].recv_pkts;

   //         grant_pkt(2, 0)   = 0;
   //         grant_pkt(12, 3)  = 0;
   //         grant_pkt(22, 13) = best[min_entry].recv_pkts;
   //         grant_pkt(36, 23) = best[min_entry].rpc_id;
   //         grant_pkt(50, 37) = best[min_entry].peer_id;

   //         grant_pkt_o.write(grant_pkt);
   //      } 
   //   }
   //}
}
