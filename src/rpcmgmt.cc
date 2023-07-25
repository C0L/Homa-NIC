#include "rpcmgmt.hh"
#include "hashmap.hh"
#include <iostream>

extern "C"{
   /**
    * rpc_state() - Maintains state information associated with an RPC
    * @sendmsg_i        - Input for user requested outgoing messages 
    * @sendmsg_o        - Output for user requested outgoing messages augmented with
    * rpc state data    
    * @recvmsg_i        - Input for user requested recieve of message
    * @header_out_i     - Input for outgoing headers
    * @header_out_o     - Output for outgoing headers augmented with rpc state data
    * @header_in_i      - Input for incoming headers
    * @header_in_o      - Output for incoming headers augmented with rpc state data
    * @grant_in_o       - Output for forwarding data to SRPT grant queue
    * headers to transmit
    * @data_srpt_o      - Output for forwarding data to the SRPT data queue
    * TODO Ideally, the sendmsg and recvmsg requests will write to the BRAMs
    * and the header_in and header_out will only read from the BRAMs. This is
    * not currently the case however.
    */
   void rpc_state(hls::stream<sendmsg_t> & sendmsg_i,
         hls::stream<srpt_data_in_t> & sendmsg_o,
         hls::stream<recvmsg_t> & recvmsg_i,
         hls::stream<header_t> & header_out_i, 
         hls::stream<header_t> & header_out_o,
         hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_dbuff_o,
         hls::stream<srpt_grant_in_t> & grant_srpt_o,
         hls::stream<srpt_grant_notif_t> & data_srpt_o) {

      static homa_rpc_t rpcs[MAX_RPCS];

#pragma HLS pipeline II=1 style=flp
      // #pragma HLS dependence variable=rpcs inter WAR false
      // #pragma HLS dependence variable=rpcs inter RAW false

      if (!header_out_i.empty()) {
         header_t header_out = header_out_i.read();

         homa_rpc_t homa_rpc = rpcs[(header_out.local_id >> 1)-1];

         header_out.dbuff_id        = homa_rpc.dbuff_id;
         header_out.daddr           = homa_rpc.daddr;
         header_out.dport           = homa_rpc.dport;
         header_out.saddr           = homa_rpc.saddr;
         header_out.sport           = homa_rpc.sport;
         header_out.id              = homa_rpc.id;
         header_out.message_length  = homa_rpc.length;
         header_out.processed_bytes = 0;
         header_out.grant_offset = homa_rpc.length - header_out.grant_offset;

         // Convert these to 0 offset rather than MAX offset
         header_out.data_offset     = homa_rpc.length - header_out.data_offset;
         header_out.incoming        = homa_rpc.length - header_out.incoming;

         header_out_o.write(header_out);
      }

      if (!header_in_i.empty()) {
         header_t header_in = header_in_i.read();
         homa_rpc_t & homa_rpc = rpcs[(header_in.local_id >> 1)-1];

         switch (header_in.type) {
            case DATA: {
               // TODO really want to get rid of this!!!
               homa_rpc.id = header_in.id;

               header_in.dma_offset = homa_rpc.buffout;
               header_in.peer_id    = homa_rpc.peer_id;

               header_in_dbuff_o.write(header_in); // Write to data buffer

               // Does this message need to interact with the grant system?
               if (header_in.message_length > header_in.incoming) { 

                  srpt_grant_in_t grant_in;
                  grant_in(SRPT_GRANT_IN_OFFSET)   = header_in.data_offset;
                  grant_in(SRPT_GRANT_IN_INC)      = header_in.incoming;
                  grant_in(SRPT_GRANT_IN_MSG_LEN)  = header_in.message_length;
                  grant_in(SRPT_GRANT_IN_RPC_ID)   = header_in.local_id;
                  grant_in(SRPT_GRANT_IN_PEER_ID)  = header_in.peer_id;

                  grant_srpt_o.write(grant_in); // Write to SRPT grant queue
               } 

               break;
            }

            case GRANT: {
               // TODO maybe other data needs to be accumulated
               srpt_grant_notif_t srpt_grant_notif;
               // TODO 
               // srpt_grant_notif(SRPT_GRANT_NOTIF_RPC_ID) = header_in.local_id;
               // srpt_grant_notif(SRPT_GRANT_NOTIF_OFFSET) = homa_rpc.length - header_in.grant_offset;
               data_srpt_o.write(srpt_grant_notif); // Write to SRPT data queue
               break;
            }
         }
      }

      /* Ideally, the RPC store contains only data passed or accumulated in the
       * original sendmsg/recvmsg calls. It is desirable to not have the
       * incoming or outgoing packets modifying this store as there is only a
       * single write port. 
       */

      /* W Processes */
      if (!sendmsg_i.empty()) {
      
         sendmsg_t sendmsg;
         sendmsg_i.read_nb(sendmsg);
         // sendmsg_t sendmsg = sendmsg_i.read();

         homa_rpc_t homa_rpc;
         homa_rpc.daddr           = sendmsg.daddr;
         homa_rpc.dport           = sendmsg.dport;
         homa_rpc.saddr           = sendmsg.saddr;
         homa_rpc.sport           = sendmsg.sport;
         homa_rpc.buffin          = sendmsg.buffin;
         homa_rpc.length          = sendmsg.length;
         homa_rpc.dbuff_id        = sendmsg.dbuff_id;
         homa_rpc.rtt_bytes       = sendmsg.rtt_bytes; // TODO lots of duplicate data
         homa_rpc.peer_id         = sendmsg.peer_id;
         homa_rpc.id              = sendmsg.id;

         rpcs[(sendmsg.local_id >> 1)-1] = homa_rpc;

         srpt_data_in_t srpt_data_in;
         srpt_data_in(SRPT_DATA_RPC_ID)    = sendmsg.local_id;
         srpt_data_in(SRPT_DATA_DBUFF_ID)  = sendmsg.dbuff_id;
         srpt_data_in(SRPT_DATA_REMAINING) = sendmsg.length;
         srpt_data_in(SRPT_DATA_GRANTED)   = sendmsg.length - sendmsg.granted;
         srpt_data_in(SRPT_DATA_DBUFFERED) = sendmsg.length;

         sendmsg_o.write(srpt_data_in);
      } else if (!recvmsg_i.empty()) {
         recvmsg_t recvmsg;
         recvmsg_i.read_nb(recvmsg);
         // recvmsg_t recvmsg = recvmsg_i.read();

         homa_rpc_t homa_rpc;
         homa_rpc.daddr     = recvmsg.daddr;
         homa_rpc.dport     = recvmsg.dport;
         homa_rpc.saddr     = recvmsg.saddr;
         homa_rpc.sport     = recvmsg.sport;
         homa_rpc.buffout   = recvmsg.buffout;
         homa_rpc.rtt_bytes = recvmsg.rtt_bytes;
         homa_rpc.peer_id   = recvmsg.peer_id;
         homa_rpc.id        = recvmsg.id;

         rpcs[(recvmsg.local_id >> 1)-1] = homa_rpc;
      }
   }

   /**
    * rpc_map() - Maintains a map for incoming packets in which this core is
    * the server from (addr, ID, port) -> (local rpc id). This also maintains
    * the map from (addr) -> (local peer id) for the purpose of determining if
    * RPCs share a peer, which is particularly useful for the SRPT grant. This
    * core manages the unique local RPC IDs (indexes into the RPC state) and
    * the unique local peer IDs.
    * system.
    * @header_in_i      - Input for incoming headers which may need to be
    * mapped to a local ID if the ID of the header indiciates that we are the
    * server in this interaction
    * @header_in_o      - Output for incoming headers with the discovered local
    * ID if applicable
    * @sendmsg_i        - Incoming user initiated sendmsg requests which need
    * to have their peer ID discovered
    * @sendmsg_o        - Outgoing user initiated sendmsg requests
    * @recvmsg_i        - Incoming recv requests, which either need to create a
    * mapping from the tuple using the provided ID, or create a "catch all"
    * mapping which can be used to recieve the next piece of data from the
    * specified peer/port.
    * TODO The hashmap needs to be more robustly tested to determine its collision properties
    * TODO need to return the first match of recv(0) to the caller?
    */
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

      //#pragma HLS dependence variable=hashmap inter WAR false
      //#pragma HLS dependence variable=hashmap inter RAW false

// #pragma HLS pipeline II=1 style=flp

      if (!header_in_i.empty()) {
         header_t header_in = header_in_i.read();

         // Are we the server for this RPC?
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
            }
         } else {
            header_in.local_id = header_in.id;
         }

         // If the incoming message is a response, the RPC ID is already valid in the local store
         header_in_o.write(header_in);
      } else if (!recvmsg_i.empty()) {

         static int count = -1;

         recvmsg_t recvmsg;
         recvmsg_i.read_nb(recvmsg);
         count++;

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

            rpc_hashpack_t rpc_query = {recvmsg.daddr, 0, recvmsg.dport, 0};
            entry_t<rpc_hashpack_t, rpc_id_t> rpc_entry = {rpc_query, recvmsg.local_id};
            rpc_hashmap.queue(rpc_entry);
         } else {
            recvmsg.local_id = (rpc_id_t) recvmsg.id;
         }

         if (count == 0) recvmsg_o.write(recvmsg);
      }  else if (!sendmsg_i.empty()) {
         // sendmsg_t sendmsg = sendmsg_i.read();
         
         sendmsg_t sendmsg;
         sendmsg_i.read_nb(sendmsg);

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
         } 

         // TODO if the user calls sendmsg with a non-zero ID, can we gaurentee
         // that it is a local ID? Should the result of a recv call be a local ID always?
         // Or, should we be performing a search operation if sendmsg != 0?
        
         sendmsg_o.write(sendmsg);
      } else {
         rpc_hashmap.process();
         peer_hashmap.process();
      }
   }
}
