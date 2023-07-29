#include "rpcmgmt.hh"
#include "hashmap.hh"
#include <iostream>

extern "C"{
   /**
    * rpc_state() - Maintains state information associated with an RPC
    * @sendmsg_i        - Input for user requested outgoing messages 
    * @sendmsg_o        - Output for user requested outgoing messages augmented with
    * rpc state data    
    * @header_out_i     - Input for outgoing headers
    * @header_out_o     - Output for outgoing headers augmented with rpc state data
    * @header_in_i      - Input for incoming headers
    * @header_in_o      - Output for incoming headers augmented with rpc state data
    * @grant_in_o       - Output for forwarding data to SRPT grant queue
    * headers to transmit
    * @data_srpt_o      - Output for forwarding data to the SRPT data queue
    */
   void rpc_state(hls::stream<sendmsg_t> & sendmsg_i,
         hls::stream<srpt_data_in_t> & sendmsg_o,
         hls::stream<header_t> & header_out_i, 
         hls::stream<header_t> & header_out_o,
         hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_dbuff_o,
         hls::stream<srpt_grant_in_t> & grant_srpt_o,
         hls::stream<srpt_grant_notif_t> & data_srpt_o) {

      static homa_rpc_t rpcs[MAX_RPCS];

#pragma HLS pipeline II=1

#pragma HLS dependence variable=rpcs inter WAR false
#pragma HLS dependence variable=rpcs inter RAW false

      /* RO Paths */
      if (!header_out_i.empty()) {
         header_t header_out = header_out_i.read();

         homa_rpc_t homa_rpc = rpcs[INDEX_FROM_RPC_ID(header_out.local_id)];

         header_out.daddr           = homa_rpc.daddr;
         header_out.dport           = homa_rpc.dport;
         header_out.saddr           = homa_rpc.saddr;
         header_out.sport           = homa_rpc.sport;
         header_out.id              = homa_rpc.id;

         header_out_o.write(header_out);
      }

      /* R/W Paths */
      if (!header_in_i.empty()) {
         header_t header_in = header_in_i.read();
         homa_rpc_t homa_rpc = rpcs[INDEX_FROM_RPC_ID(header_in.local_id)];

         switch (header_in.type) {
            case DATA: {
               homa_rpc.saddr = header_in.saddr;
               homa_rpc.daddr = header_in.daddr;
               homa_rpc.dport = header_in.dport;
               homa_rpc.sport = header_in.sport;
               homa_rpc.id    = header_in.id;
      
               if (header_in.message_length > header_in.incoming) { 
                  srpt_grant_in_t grant_in;
                  grant_in(SRPT_GRANT_IN_OFFSET)  = header_in.data_offset;
                  grant_in(SRPT_GRANT_IN_INC)     = header_in.incoming;
                  grant_in(SRPT_GRANT_IN_MSG_LEN) = header_in.message_length;
                  grant_in(SRPT_GRANT_IN_RPC_ID)  = header_in.local_id;
                  grant_in(SRPT_GRANT_IN_PEER_ID) = header_in.peer_id;

                  // Notify the grant queue of this receipt
                  grant_srpt_o.write(grant_in); 
               } 

               // Instruct the data buffer where to write this messages' data
               header_in_dbuff_o.write(header_in); 

               break;
            }

            case GRANT: {
               srpt_grant_notif_t srpt_grant_notif;
               srpt_grant_notif(SRPT_GRANT_NOTIF_RPC_ID) = header_in.local_id;
               srpt_grant_notif(SRPT_GRANT_NOTIF_OFFSET) = header_in.length - header_in.grant_offset;
               data_srpt_o.write(srpt_grant_notif); 
               break;
            }
         }

         rpcs[INDEX_FROM_RPC_ID(header_in.local_id)] = homa_rpc;
      } else if (!onboard_send.empty()) {
         onboard_send_t onboard_send = onboard_send_i.read();

         homa_rpc_t homa_rpc;
         homa_rpc.daddr           = onboard_send.daddr;
         homa_rpc.dport           = onboard_send.dport;
         homa_rpc.saddr           = onboard_send.saddr;
         homa_rpc.sport           = onboard_send.sport;
         homa_rpc.buffin          = onboard_send.buffin;
         homa_rpc.length          = onboard_send.length;
         homa_rpc.dbuff_id        = onboard_send.dbuff_id;
         homa_rpc.id              = onboard_send.id;

         rpcs[INDEX_FROM_RPC_ID(sendmsg.local_id)] = homa_rpc;

         srpt_data_in_t srpt_data_in;
         srpt_data_in(SRPT_DATA_RPC_ID)    = onboard_send.local_id;
         srpt_data_in(SRPT_DATA_DBUFF_ID)  = onboard_send.dbuff_id;
         srpt_data_in(SRPT_DATA_REMAINING) = onboard_send.length;
         srpt_data_in(SRPT_DATA_GRANTED)   = (RTT_BYTES > onboard_send.length) ? onboard_send.length : RTT_BYTES;
         srpt_data_in(SRPT_DATA_MSG_LEN)   = sendmsg.length;

         sendmsg_o.write(srpt_data_in);
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
    */
   void rpc_map(hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_o,
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

      // TODO this II can be lowered?
#pragma HLS pipeline II=1

      if (!header_in_i.empty()) {
         header_t header_in = header_in_i.read();

         /* Check if we are the server for this RPC. If we are the RPC ID is
          * not in a local form and we need to map it to a local ID.
          *
          * If we are the client, the locally meaningful ID can be derived
          * directly from the RPC ID in the header
          */
         if (!IS_CLIENT(header_in.id)) {

            // Check if this peer is already registered
            peer_hashpack_t peer_query = {header_in.saddr};
            header_in.peer_id          = peer_hashmap.search(peer_query);
            
            // If the peer is not registered, generate new ID and register it
            if (header_in.peer_id == 0) {
               header_in.peer_id = PEER_ID_FROM_INDEX(peer_stack.pop());
            
               entry_t<peer_hashpack_t, peer_id_t> peer_entry = {peer_query, header_in.peer_id};
               peer_hashmap.queue(peer_entry);
            }

            // Check if this RPC is already registered
            rpc_hashpack_t rpc_query = {header_in.saddr, header_in.id, header_in.sport, 0};
            header_in.local_id = rpc_hashmap.search(query);

            // If the rpc is not registered, generate a new RPC ID and register it 
            if (header_in.local_id == 0) 
               header_in.local_id = RPC_ID_FROM_INDEX(rpc_stack.pop());
               entry_t<rpc_hashpack_t, local_id_t> rpc_entry = {rpc_query, header_in.local_id};
               rpc_hashmap.queue(rpc_query);
            }
         } else {
            header_in.local_id = header_in.id;
         }

         header_in_o.write(header_in);
      } else if (!onboard_send_i.empty()) {
         onboard_send_t onboard_send = onboard_send_i.read();

         /* If the caller provided an ID of 0 this is a request message and we
          * need to generate a new local ID. Otherwise, this is a response
          * message and the ID is already valid in homa_rpc buffer
          */
         if (onboard_send.id == 0) {

            // Generate a new local ID, and set the RPC ID to be that
            onboard_send.local_id = RPC_ID_FROM_INDEX(rpc_stack.pop());
            onboard_send.id       = onboard_send.local_id;

            // Check if this peer is already registered
            peer_hashpack_t peer_query = {onboard_send.daddr};
            onboard_send.peer_id       = peer_hashmap.search(peer_query);

            // If the peer is not registered, generate new ID and register it
            if (onboard_send.peer_id == 0) {
               onboard_send.peer_id = PEER_ID_FROM_INDEX(peer_stack.pop());
 
               entry_t<peer_hashpack_t, peer_id_t> peer_entry = {peer_query, onboard_send.peer_id};
               peer_hashmap.queue(peer_entry);
            }
         } 
       
         onboard_send_o.write(onboard_send);
      } else {
         rpc_hashmap.process();
         peer_hashmap.process();
      }
   }
}
