#include "srptmgmt.hh"
#include "rpcmgmt.hh"

/**
 * WARNING: For C simulation only
 * srpt_data_pkts() - Determines what DATA packet to send next.
 * @sendmsg_i - New request or response messages from the user
 * @dbuff_notif_i - Updates about what data is held on-chip
 * @data_pkt_o - The next outgoing DATA packet that should be sent
 */
void srpt_data_pkts(hls::stream<sendmsg_t, VERIF_DEPTH> & sendmsg_i,
      hls::stream<dbuff_notif_t, VERIF_DEPTH> & dbuff_notif_i,
      hls::stream<ready_data_pkt_t, VERIF_DEPTH> & data_pkt_o,
      hls::stream<header_t, VERIF_DEPTH> & header_in_i) {
   
   static srpt_data_t entries[MAX_RPCS];
   static ap_uint<32> grants[MAX_RPCS]; 
   static dbuff_notif_t dbuff_notifs[NUM_DBUFF];

   dbuff_notif_t dbuff_notif;
   // if (!dbuff_notif_i.empty()) {
   //    dbuff_notif = dbuff_notif_i.read();
   if (dbuff_notif_i.read_nb(dbuff_notif)) {

      dbuff_notifs[dbuff_notif.dbuff_id] = dbuff_notif;

      //std::cerr << "GRANT UP TO: " << (dbuff_notif.dbuff_chunk+1) * DBUFF_CHUNK_SIZE << std::endl;
   }

   header_t header_in;
   // if (!header_in_i.empty()) {
   //    header_in = header_in_i.read();
   if (header_in_i.read_nb(header_in)) {

      grants[header_in.local_id] = header_in.grant_offset;
   }

   sendmsg_t sendmsg;
   //if (!sendmsg_i.empty()) {
   //   sendmsg = sendmsg_i.read();
   if (sendmsg_i.read_nb(sendmsg)) {

      srpt_data_t new_entry = {sendmsg.local_id, sendmsg.dbuff_id, sendmsg.length, sendmsg.length};
      grants[((sendmsg.local_id >> 1)-1)] = sendmsg.granted;

      // TODO need to document this translation better
      entries[((sendmsg.local_id >> 1)-1)] = new_entry;
   } 

   srpt_data_t head = {0, 0, 0xFFFFFFFF, 0xFFFFFFFF};
   for (int i = 0; i < MAX_RPCS; ++i) {
       
      if (entries[i].remaining < head.remaining && entries[i].rpc_id != 0) {
         // Is the offset of availible data 1 packetsize or more greater than the offset we have sent up to?
         bool unblocked = (((dbuff_notifs[entries[i].dbuff_id].dbuff_chunk+1) * DBUFF_CHUNK_SIZE)) >= MIN((entries[i].total - entries[i].remaining + HOMA_PAYLOAD_SIZE)(31,0), entries[i].total(31,0));
         bool granted = !(head.total - head.remaining > grants[i]);

         if (unblocked && granted) { 
            head = entries[i];
         }
      }
   }

   if (head.rpc_id != 0) {
      // if (data_pkt_o.size() < VERIF_DEPTH) {

         std::cerr << (dbuff_notifs[head.dbuff_id].dbuff_chunk+1) * DBUFF_CHUNK_SIZE << std::endl;

         ap_uint<32> remaining  = (HOMA_PAYLOAD_SIZE > head.remaining) ? ((ap_uint<32>) 0) : ((ap_uint<32>) (head.remaining - HOMA_PAYLOAD_SIZE));

         data_pkt_o.write_nb({head.rpc_id, head.dbuff_id, head.remaining, head.total, grants[head.rpc_id]});

         entries[((head.rpc_id >> 1) - 1)].remaining = remaining;

         if (remaining == 0) {
            entries[((head.rpc_id >> 1) - 1)] = {0, 0, 0xFFFFFFFF, 0xFFFFFFFF};
         }
      // } 
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
void srpt_grant_pkts(hls::stream<ap_uint<58>, VERIF_DEPTH> & header_in_i,
                     hls::stream<ap_uint<51>, VERIF_DEPTH> & grant_pkt_o) {

   // TODO testing to get rid of warning. Should have no effect
#pragma HLS pipeline style=flp

   // Because this is only used for sim we brute force grants
   static srpt_grant_t entries[MAX_RPCS];
   static ap_uint<32> avail_pkts = OVERCOMMIT_PKTS; 

   // Headers from incoming DATA packets
   ap_uint<58> header_in_raw;

   // if (!header_in_i.empty()) {
   //    header_in_raw = header_in_i.read();
   if (header_in_i.read_nb(header_in_raw)) {

      header_t header_in;

      header_in.peer_id = header_in_raw(57, 44);
      header_in.local_id = header_in_raw(43, 30);
      header_in.message_length = header_in_raw(29, 20);
      header_in.incoming = header_in_raw(19, 10);
      header_in.data_offset = header_in_raw(9, 0);

      // std::cerr << "HEADER IN: " << header_in.local_id << std::endl;

      if (header_in.message_length > RTT_PKTS) {

         // The first unscheduled packet creates the entry. Only need an entry if the RPC needs grants.
         if (header_in.data_offset == 0) {
            std::cerr << "GRANT CREATE NEW ENTRY\n";

            // TODO macro this
            entries[((header_in.local_id >> 1) - 1)] = {header_in.peer_id, header_in.local_id, 1, header_in.message_length - 1};
         } else {
            std::cerr << "REUP ENTRY\n";
            entries[((header_in.local_id >> 1) - 1)].recv_pkts += 1;
            entries[((header_in.local_id >> 1) - 1)].grantable_pkts -= 1;
         } 
      }
   } else {

      ap_uint<51> grant_pkt;
   
      srpt_grant_t best[8];

      // Fill each entry in the best queue
      for (int i = 0; i < 8; ++i) {
         srpt_grant_t curr_best = entries[0];
         for (int e = 0; e < MAX_RPCS; ++e) {
            // Is this entry better than our current best
            if (entries[e].grantable_pkts < curr_best.grantable_pkts && entries[e].rpc_id != 0) {
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
         if (best[i].grantable_pkts < next_grant.grantable_pkts && best[i].recv_pkts != 0) {
            next_grant = best[i];
         }
      }

      if (next_grant.recv_pkts > 0 && next_grant.rpc_id != 0) {
         ap_uint<51> grant_pkt;

         if (next_grant.recv_pkts > avail_pkts && grant_pkt_o.size() < VERIF_DEPTH) {
            // Just send avail_pkts of data
 
            entries[(next_grant.rpc_id >> 1) - 1].recv_pkts -= avail_pkts;
            entries[(next_grant.rpc_id >> 1) - 1].grantable_pkts -= avail_pkts;

            grant_pkt(2, 0)   = 0;
            grant_pkt(12, 3)  = 0;
            grant_pkt(22, 13) = avail_pkts;
            grant_pkt(36, 23) = next_grant.rpc_id;
            grant_pkt(50, 37) = next_grant.peer_id;

            grant_pkt_o.write_nb(grant_pkt);

         } else if (grant_pkt_o.size() < VERIF_DEPTH) { 
            // Is this going to result in a fully granted message?
            if ((next_grant.grantable_pkts - next_grant.recv_pkts) == 0) { 
               entries[(next_grant.rpc_id >> 1) - 1] = {0, 0, 0xFFFFFFFF, 0xFFFFFFFF}; 
            }
            
            entries[(next_grant.rpc_id >> 1) - 1].recv_pkts = 0;
            entries[(next_grant.rpc_id >> 1) - 1].grantable_pkts -= next_grant.recv_pkts;

            grant_pkt(2, 0)   = 0;
            grant_pkt(12, 3)  = 0;
            grant_pkt(22, 13) = next_grant.recv_pkts;
            grant_pkt(36, 23) = next_grant.rpc_id;
            grant_pkt(50, 37) = next_grant.peer_id;

            std::cerr << "DISPATCH GRANT\n";
            grant_pkt_o.write_nb(grant_pkt);
         } 
      }
   }
}
