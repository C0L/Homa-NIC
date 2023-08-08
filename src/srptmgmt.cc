#include "srptmgmt.hh"
#include "rpcmgmt.hh"

/**
 * WARNING: For C simulation only
 * srpt_data_pkts() - Determines what DATA packet to send next.
 * @sendmsg_i - New request or response messages from the user
 * @dbuff_notif_i - Updates about what data is held on-chip
 * @data_pkt_o - The next outgoing DATA packet that should be sent
 */
void srpt_data_pkts(hls::stream<srpt_data_in_t> & sendmsg_raw_i,
      hls::stream<srpt_dbuff_notif_t> & dbuff_notif_raw_i,
      hls::stream<srpt_data_out_t> & data_pkt_raw_o,
      hls::stream<srpt_grant_notif_t> & grant_notif_raw_i) {

   static srpt_data_in_t entries[MAX_RPCS];

   if (!dbuff_notif_raw_i.empty()) {
      srpt_dbuff_notif_t dbuff_notif = dbuff_notif_raw_i.read();
      entries[SEND_INDEX_FROM_RPC_ID(dbuff_notif(SRPT_DBUFF_NOTIF_RPC_ID))](SRPT_DATA_DBUFFERED) = dbuff_notif(SRPT_DBUFF_NOTIF_OFFSET);
      std::cerr << "DBUFF NOTIF IN " << SEND_INDEX_FROM_RPC_ID(dbuff_notif(SRPT_DBUFF_NOTIF_RPC_ID)) << std::endl;
   }

   if (!grant_notif_raw_i.empty()) {
      srpt_grant_notif_t header_in = grant_notif_raw_i.read();
      entries[SEND_INDEX_FROM_RPC_ID(header_in(SRPT_GRANT_NOTIF_RPC_ID))](SRPT_DATA_GRANTED) = header_in(SRPT_GRANT_NOTIF_OFFSET);
      std::cerr << "GRANT NOTIF IN " << SEND_INDEX_FROM_RPC_ID(header_in(SRPT_GRANT_NOTIF_RPC_ID)) << std::endl;
   }

   if (!sendmsg_raw_i.empty()) {

      srpt_data_in_t sendmsg = sendmsg_raw_i.read();
      std::cerr << "SEND MSG IN " << SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_DATA_RPC_ID)) << std::endl;


      std::cerr << sendmsg(SRPT_DATA_RPC_ID) << std::endl;
      std::cerr << sendmsg(SRPT_DATA_REMAINING) << std::endl;
      std::cerr << sendmsg(SRPT_DATA_DBUFFERED) << std::endl;
      std::cerr << sendmsg(SRPT_DATA_GRANTED) << std::endl;
      entries[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_DATA_RPC_ID))](SRPT_DATA_RPC_ID)    = sendmsg(SRPT_DATA_RPC_ID);
      entries[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_DATA_RPC_ID))](SRPT_DATA_REMAINING) = sendmsg(SRPT_DATA_REMAINING);
      // entries[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_DATA_RPC_ID))](SRPT_DATA_DBUFFERED) = sendmsg(SRPT_DATA_DBUFFERED);
      entries[SEND_INDEX_FROM_RPC_ID(sendmsg(SRPT_DATA_RPC_ID))](SRPT_DATA_GRANTED)   = sendmsg(SRPT_DATA_GRANTED);
   }

   srpt_data_in_t head;

   head(SRPT_DATA_RPC_ID)    = 0;
   head(SRPT_DATA_REMAINING) = 0xFFFFFFFF;
   head(SRPT_DATA_GRANTED)   = 0xFFFFFFFF;
   head(SRPT_DATA_DBUFFERED) = 0xFFFFFFFF;

   for (int i = 0; i < MAX_RPCS; ++i) {
      if (entries[i](SRPT_DATA_REMAINING) < head(SRPT_DATA_REMAINING) && entries[i](SRPT_DATA_RPC_ID) != 0) {
         // Is the offset of availible data 1 packetsize or more greater than the offset we have sent up to?
         bool granted   = (entries[i](SRPT_DATA_GRANTED) + 1 <= entries[i](SRPT_DATA_REMAINING)) | (entries[i](SRPT_DATA_GRANTED) == 0);
         bool dbuffered = (entries[i](SRPT_DATA_DBUFFERED) + HOMA_PAYLOAD_SIZE <= entries[i](SRPT_DATA_REMAINING)) | (entries[i](SRPT_DATA_DBUFFERED) == 0);

         if (granted && dbuffered) {
            head = entries[i];
         }
      }
   }

   if (head(SRPT_DATA_RPC_ID) != 0) {

      ap_uint<32> remaining = (HOMA_PAYLOAD_SIZE > head(SRPT_DATA_REMAINING)) 
                                 ? ((ap_uint<32>) 0) : ((ap_uint<32>) (head(SRPT_DATA_REMAINING) - HOMA_PAYLOAD_SIZE));

      data_pkt_raw_o.write(head);
      std::cerr << "DATA PKT OUT\n";
      entries[SEND_INDEX_FROM_RPC_ID(head(SRPT_DATA_RPC_ID))](SRPT_DATA_REMAINING) = remaining;

      if (remaining == 0) {
	  entries[SEND_INDEX_FROM_RPC_ID(head(SRPT_DATA_RPC_ID))] = 0;
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
      srpt_grant_in_t grant_in = grant_in_i.read();

      // The first unscheduled packet creates the entry. Only need an entry if the RPC needs grants.
      if (grant_in(SRPT_GRANT_IN_OFFSET) == 0) {
         entries[((grant_in(SRPT_GRANT_IN_RPC_ID) >> 1) - 1)] = {grant_in(SRPT_GRANT_IN_PEER_ID), grant_in(SRPT_GRANT_IN_RPC_ID), HOMA_PAYLOAD_SIZE, grant_in(SRPT_GRANT_IN_MSG_LEN) - HOMA_PAYLOAD_SIZE};
      } else {
         entries[((grant_in(SRPT_GRANT_IN_RPC_ID) >> 1) - 1)].recv_bytes += HOMA_PAYLOAD_SIZE;
         entries[((grant_in(SRPT_GRANT_IN_RPC_ID) >> 1) - 1)].grantable_bytes -= HOMA_PAYLOAD_SIZE;
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

         if (next_grant.recv_bytes > avail_bytes) {
            // Just send avail_pkts of data
            entries[(next_grant.rpc_id >> 1) - 1].recv_bytes -= avail_bytes;
            entries[(next_grant.rpc_id >> 1) - 1].grantable_bytes -= avail_bytes;

            grant_out(SRPT_GRANT_OUT_GRANT) = avail_bytes;
            grant_out(SRPT_GRANT_OUT_RPC_ID) = next_grant.rpc_id;
            grant_out(SRPT_GRANT_OUT_PEER_ID) = next_grant.peer_id;

            grant_out_o.write(grant_out);

         } else { 
            // Is this going to result in a fully granted message?
            if ((next_grant.grantable_bytes - next_grant.recv_bytes) == 0) { 
               entries[(next_grant.rpc_id >> 1) - 1] = {0, 0, 0xFFFFFFFF, 0xFFFFFFFF}; 
            }

            entries[(next_grant.rpc_id >> 1) - 1].recv_bytes = 0;
            entries[(next_grant.rpc_id >> 1) - 1].grantable_bytes -= next_grant.recv_bytes;

            srpt_grant_out_t grant_out;

            grant_out(SRPT_GRANT_OUT_GRANT)   = entries[(next_grant.rpc_id >> 1) - 1].grantable_bytes;
            grant_out(SRPT_GRANT_OUT_RPC_ID)  = next_grant.rpc_id;
            grant_out(SRPT_GRANT_OUT_PEER_ID) = next_grant.peer_id;

            grant_out_o.write(grant_out);
         } 
      }
   }
}
