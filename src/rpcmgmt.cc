#include "rpcmgmt.hh"
#include "hashmap.hh"
#include <iostream>

extern "C"{
   void rpc_state(hls::stream<sendmsg_t> & sendmsg_i,
         hls::stream<sendmsg_t> & sendmsg_o,
         hls::stream<recvmsg_t> & recvmsg_i,
         hls::stream<header_t> & header_out_i, 
         hls::stream<header_t> & header_out_o,
         hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_dbuff_o,
         hls::stream<grant_in_t> & grant_in_o,
         hls::stream<header_t> & header_in_srpt_o) {

      static homa_rpc_t rpcs[MAX_RPCS];

      // #pragma HLS pipeline II=1 style=flp
      // #pragma HLS dependence variable=rpcs inter WAR false
      // #pragma HLS dependence variable=rpcs inter RAW false

      if (!header_out_i.empty()) {
         header_t header_out = header_out_i.read();

         homa_rpc_t homa_rpc = rpcs[(header_out.local_id >> 1)-1];

         header_out.dbuff_id  = homa_rpc.msgout.dbuff_id;
         header_out.daddr     = homa_rpc.daddr;
         header_out.dport     = homa_rpc.dport;
         header_out.saddr     = homa_rpc.saddr;
         header_out.sport     = homa_rpc.sport;
         header_out.id        = homa_rpc.id;

         // The grant offset as it comes out of the SRPT grant core is an increment of pkts
         // header_out.grant_offset = homa_rpc.msgin.incoming + (header_out.grant_offset * HOMA_PAYLOAD_SIZE);

         header_out_o.write(header_out);
      }

      if (!header_in_i.empty()) {
         header_t header_in = header_in_i.read();
         homa_rpc_t & homa_rpc = rpcs[(header_in.local_id >> 1)-1];

         switch (header_in.type) {
            case DATA: {
               // homa_rpc.msgin.incoming = header_in.incoming;
               homa_rpc.id = header_in.id;

               // TODO maybe other data needs to be accumulated
               header_in.dma_offset = homa_rpc.buffout;
               header_in.peer_id    = homa_rpc.peer_id;

               header_in_dbuff_o.write(header_in); // Write to data buffer

               // Does this message need to interact with the grant system?
               if (header_in.message_length > header_in.incoming) { 

                  grant_in_t grant_in;
                  grant_in(GRANT_IN_OFFSET)   = header_in.data_offset;
                  grant_in(GRANT_IN_INC)      = header_in.incoming;
                  grant_in(GRANT_IN_MSG_LEN)  = header_in.message_length;
                  grant_in(GRANT_IN_RPC_ID)   = header_in.local_id;
                  grant_in(GRANT_IN_PEER_ID)  = header_in.peer_id;

                  grant_in_o.write(grant_in); // Write to SRPT grant queue
               } 

               break;
            }

            case GRANT: {
               // TODO maybe other data needs to be accumulated
               header_in_srpt_o.write(header_in); // Write to SRPT data queue
               break;
            }
         }
      }

      /* R/W Processes */
      if (!sendmsg_i.empty()) {
      
         // TODO store peer in here

         sendmsg_t sendmsg = sendmsg_i.read();

         homa_rpc_t homa_rpc;
         homa_rpc.state           = homa_rpc_t::RPC_OUTGOING; // TODO is this needed?
         homa_rpc.daddr           = sendmsg.daddr;
         homa_rpc.dport           = sendmsg.dport;
         homa_rpc.saddr           = sendmsg.saddr;
         homa_rpc.sport           = sendmsg.sport;
         homa_rpc.buffin          = sendmsg.buffin;
         homa_rpc.msgout.length   = sendmsg.length;
         homa_rpc.msgout.dbuff_id = sendmsg.dbuff_id;
         homa_rpc.rtt_bytes       = sendmsg.rtt_bytes;
         homa_rpc.peer_id         = sendmsg.peer_id;
         homa_rpc.id              = sendmsg.id;
         // homa_rpc.id              = sendmsg.peer_id;

         rpcs[(sendmsg.local_id >> 1)-1] = homa_rpc;

         sendmsg_o.write(sendmsg);
      } else if (!recvmsg_i.empty()) {
         recvmsg_t recvmsg = recvmsg_i.read();

         // If the caller ID determines if this machine is client or server
         // recvmsg.local_id = (recvmsg.id == 0) ? (rpc_id_t) ((rpc_stack.pop() + 1) << 1) : (rpc_id_t) recvmsg.id;

         homa_rpc_t homa_rpc;
         homa_rpc.state     = homa_rpc_t::RPC_INCOMING;
         homa_rpc.daddr     = recvmsg.daddr;
         homa_rpc.dport     = recvmsg.dport;
         homa_rpc.saddr     = recvmsg.saddr;
         homa_rpc.sport     = recvmsg.sport;
         homa_rpc.buffout   = recvmsg.buffout;
         homa_rpc.rtt_bytes = recvmsg.rtt_bytes;
         homa_rpc.peer_id   = recvmsg.peer_id;
         // TODO also set ID

         rpcs[(recvmsg.local_id >> 1)-1] = homa_rpc;

         // recvmsg_o.write(recvmsg);
      }
   }

   void rpc_map(hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_o,
         hls::stream<recvmsg_t> & recvmsg_i,
         hls::stream<recvmsg_t> & recvmsg_o,
         hls::stream<sendmsg_t> & sendmsg_i,
         hls::stream<sendmsg_t> & sendmsg_o) {

      // Unique local RPC IDs
      static stack_t<rpc_id_t, MAX_RPCS> rpc_stack(true);

      // Unique local Peer IDs
      static stack_t<peer_id_t, MAX_PEERS> peer_stack(true);

      // hash(dest addr) -> peer ID
      static hashmap_t<peer_hashpack_t, peer_id_t, PEER_SUB_TABLE_SIZE, PEER_SUB_TABLE_INDEX, PEER_HP_SIZE, MAX_OPS> peer_hashmap;

      // hash(dest addr, sender ID, dest port) -> rpc ID
      static hashmap_t<rpc_hashpack_t, rpc_id_t, RPC_SUB_TABLE_SIZE, RPC_SUB_TABLE_INDEX, RPC_HP_SIZE, MAX_OPS> rpc_hashmap;

      // TODO can this be problematic?
      //#pragma HLS dependence variable=hashmap inter WAR false
      //#pragma HLS dependence variable=hashmap inter RAW false

#pragma HLS pipeline II=1 style=flp

      if (!header_in_i.empty()) {
         header_t header_in = header_in_i.read();

         // TODO This is clumsy 

         /* Are we the server for this RPC? */
         if (!IS_CLIENT(header_in.id)) {

            // Unscheduled packets need to be mapped to tmp recv entry
            if (header_in.type == DATA) {
               if (header_in.data_offset == 0) {

                  rpc_hashpack_t recvfind = {header_in.daddr, 0, header_in.dport, 0};

                  header_in.local_id = rpc_hashmap.search(recvfind);

                  rpc_hashpack_t recvspecialize = {header_in.daddr, header_in.id, header_in.dport, 0};

                  rpc_hashmap.queue({recvspecialize, header_in.local_id});
               } else {
                  rpc_hashpack_t recvspecialize = {header_in.daddr, header_in.id, header_in.dport, 0};

                  header_in.local_id = rpc_hashmap.search(recvspecialize);
               }
            } else {
               rpc_hashpack_t query = {header_in.saddr, header_in.id, header_in.sport, 0};
               header_in.local_id = rpc_hashmap.search(query);

               std::cerr << "GRANT IN BACK MAP TO " << header_in.local_id << std::endl;
            }
         } else {
            header_in.local_id = header_in.id;
            std::cerr << "WE ARE THE CLIENT FOR TYPE " << header_in.type << std::endl;
         }

         // If the incoming message is a response, the RPC ID is already valid in the local store
         header_in_o.write(header_in);
      } else if (!recvmsg_i.empty()) {
         recvmsg_t recvmsg = recvmsg_i.read();

         /* If the caller provided an ID of 0 we want to match on the first RPC
          * from the matching peer and port combination Otherwise recv has been
          * called on an already active ID which is local.
          */
         if (recvmsg.id == 0) {

            // Create a local ID that will be used when that match is found
            recvmsg.local_id = ((rpc_stack.pop() + 1) << 1);

            peer_hashpack_t peer_query = {recvmsg.daddr};
            recvmsg.peer_id = peer_hashmap.search(peer_query);

            if (recvmsg.peer_id == 0) {
               // TODO macro this
               recvmsg.peer_id = (peer_stack.pop() + 1);

               entry_t<peer_hashpack_t, peer_id_t> peer_entry = {peer_query, recvmsg.peer_id};
               peer_hashmap.queue(peer_entry);
            }

            std::cerr << "RECVMSG " << recvmsg.local_id << std::endl;

            rpc_hashpack_t rpc_query = {recvmsg.daddr, 0, recvmsg.dport, 0};
            entry_t<rpc_hashpack_t, rpc_id_t> rpc_entry = {rpc_query, recvmsg.local_id};
            rpc_hashmap.queue(rpc_entry);
         } else {
            recvmsg.local_id = (rpc_id_t) recvmsg.id;
         }

         recvmsg_o.write(recvmsg);
      }  else if (!sendmsg_i.empty()) {
         sendmsg_t sendmsg = sendmsg_i.read();

         /* If the caller provided an ID of 0 this is a request message and we
          * need to generate a new local ID Otherwise, this is a response
          * message and the ID already exists
          */
         if (sendmsg.id == 0) {

            sendmsg.local_id = ((rpc_stack.pop() + 1) << 1);
            sendmsg.id = sendmsg.local_id;

            peer_hashpack_t peer_query = {sendmsg.daddr};
            sendmsg.peer_id = peer_hashmap.search(peer_query);

            if (sendmsg.peer_id == 0) {
               // TODO macro this
               sendmsg.peer_id = (peer_stack.pop() + 1);
 
               entry_t<peer_hashpack_t, peer_id_t> peer_entry = {peer_query, sendmsg.peer_id};
               peer_hashmap.queue(peer_entry);
            }

            std::cerr << "SENDMSG " << sendmsg.local_id << std::endl;
   
            // TODO a send msg never needs to create a mapping????
            // rpc_hashpack_t rpc_query = {sendmsg.daddr, sendmsg.local_id, sendmsg.dport, 0};
            // rpc_hashpack_t rpc_query = {sendmsg.daddr, sendmsg.id, sendmsg.dport, 0};
            // entry_t<rpc_hashpack_t, rpc_id_t> rpc_entry = {rpc_query, sendmsg.local_id};
            // rpc_hashmap.queue(rpc_entry); 
         } else {
            // sendmsg.local_id = (rpc_id_t) sendmsg.id;
            // TODO The local already exists, we just need to find it!?
            // sendmsg.id = sendmsg.id;
         }
        
         sendmsg_o.write(sendmsg);
      } else {
         rpc_hashmap.process();
         peer_hashmap.process();
      }
   }
}
