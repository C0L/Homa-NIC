#include "rpcmgmt.hh"
#include "hashmap.hh"
#include <iostream>

extern "C"{
   void rpc_state(hls::stream<sendmsg_t> & sendmsg_i,
         hls::stream<sendmsg_t> & sendmsg_o,
         hls::stream<recvmsg_t> & recvmsg_i,
         hls::stream<recvmsg_t> & recvmsg_o,
         hls::stream<header_t> & header_out_i, 
         hls::stream<header_t> & header_out_o,
         hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_dbuff_o,
         hls::stream<grant_in_t> & grant_in_o,
         hls::stream<header_t> & header_in_srpt_o) {

      static stack_t<rpc_id_t, MAX_RPCS> rpc_stack(true);
      static homa_rpc_t rpcs[MAX_RPCS];

      // #pragma HLS pipeline II=1 style=flp
      // #pragma HLS dependence variable=rpcs inter WAR false
      // #pragma HLS dependence variable=rpcs inter RAW false

      header_t header_out;
      /* Read only process */
      if (!header_out_i.empty()) {
         header_out = header_out_i.read();

         homa_rpc_t homa_rpc = rpcs[(header_out.local_id >> 1)-1];

         header_out.dbuff_id = homa_rpc.msgout.dbuff_id;
         header_out.daddr = homa_rpc.daddr;
         header_out.dport = homa_rpc.dport;
         header_out.saddr = homa_rpc.saddr;
         header_out.sport = homa_rpc.sport;

         // The grant offset as it comes out of the SRPT grant core is an increment of pkts
         header_out.grant_offset = homa_rpc.msgin.incoming + (header_out.grant_offset * HOMA_PAYLOAD_SIZE);

         header_out_o.write(header_out);
      }

      header_t header_in;
      if (!header_in_i.empty()) {
         header_in = header_in_i.read();
         homa_rpc_t homa_rpc = rpcs[(header_in.local_id >> 1)-1];

         switch (header_in.type) {
            case DATA: {
               homa_rpc.msgin.incoming = header_in.incoming;

               // TODO maybe other data needs to be accumulated
               header_in.dma_offset = homa_rpc.buffout;

               header_in_dbuff_o.write(header_in); // Write to data buffer

               // std::cerr << "MESSAGE LENGTH: " << header_in.message_length << std::endl;
               // std::cerr << "INCOMING: " << header_in.incoming << std::endl;
               
               // Does this RPC need to interact with the grant system?
               if (header_in.message_length > header_in.incoming) { 

                  grant_in_t grant_in;
                  grant_in(GRANT_IN_OFFSET)   = header_in.data_offset;
                  grant_in(GRANT_IN_INC)      = header_in.incoming;
                  grant_in(GRANT_IN_MSG_LEN)  = header_in.message_length;
                  grant_in(GRANT_IN_RPC_ID)   = header_in.local_id;
                  grant_in(GRANT_IN_PEER_ID)  = header_in.peer_id;

                  std::cerr << "FORWARDED TO GRANT SYSTEM\n";
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

      sendmsg_t sendmsg;
      recvmsg_t recvmsg;
      /* R/W Processes */
      if (!sendmsg_i.empty()) {
         sendmsg = sendmsg_i.read();

         if (sendmsg.id == 0) { // Is this a request message?
                               
            // TODO define macro for this translation (and back)
            sendmsg.local_id = ((rpc_stack.pop() + 1) << 1);

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

            rpcs[(sendmsg.local_id >> 1)-1] = homa_rpc;
         } else { // Is this a response message?
            homa_rpc_t & homa_rpc = rpcs[(sendmsg.local_id >> 1)-1];
            homa_rpc.state           = homa_rpc_t::RPC_OUTGOING;
            homa_rpc.msgout.dbuff_id = sendmsg.dbuff_id;
            homa_rpc.rtt_bytes       = sendmsg.rtt_bytes;
         }

         sendmsg_o.write(sendmsg);
      } else if (!recvmsg_i.empty()) {
         recvmsg = recvmsg_i.read();

         // If the caller ID determines if this machine is client or server
         recvmsg.local_id = (recvmsg.id == 0) ? (rpc_id_t) ((rpc_stack.pop() + 1) << 1) : (rpc_id_t) recvmsg.id;

         homa_rpc_t homa_rpc;
         homa_rpc.state = homa_rpc_t::RPC_INCOMING;
         homa_rpc.daddr = recvmsg.daddr;
         homa_rpc.dport = recvmsg.dport;
         homa_rpc.saddr = recvmsg.saddr;
         homa_rpc.sport = recvmsg.sport;
         homa_rpc.buffout = recvmsg.buffout;
         homa_rpc.rtt_bytes = recvmsg.rtt_bytes;

         rpcs[(recvmsg.local_id >> 1)-1] = homa_rpc;

         recvmsg_o.write(recvmsg);
      }
   }

   void rpc_map(hls::stream<header_t> & header_in_i,
         hls::stream<header_t> & header_in_o,
         hls::stream<recvmsg_t> & recvmsg_i) {

   // TODO is there a way to avoid double searches here??
      static stack_t<peer_id_t, MAX_PEERS> peer_stack(true);
      static hashmap_t<peer_hashpack_t, peer_id_t, PEER_SUB_TABLE_SIZE, PEER_SUB_TABLE_INDEX, PEER_HP_SIZE, MAX_OPS> hashmap;
      static hashmap_t<rpc_hashpack_t, rpc_id_t, RPC_SUB_TABLE_SIZE, RPC_SUB_TABLE_INDEX, RPC_HP_SIZE, MAX_OPS> hashmap;

      // TODO can this be problematic?
      //#pragma HLS dependence variable=hashmap inter WAR false
      //#pragma HLS dependence variable=hashmap inter RAW false

#pragma HLS pipeline II=1 style=flp

      header_t header_in;
      recvmsg_t recvmsg;
      if (!header_in_i.empty()) {
         header_in = header_in_i.read();
         /* The incoming message is a request */
         if (!IS_CLIENT(header_in.sender_id)) {

            // Unscheduled packets need to be mapped to tmp recv entry
            if (header_in.type == DATA) {
               if (header_in.data_offset == 0) {
                  rpc_hashpack_t recvfind = {header_in.daddr, 0, header_in.dport, 0};

                  header_in.local_id = hashmap.search(recvfind);

                  rpc_hashpack_t recvspecialize = {header_in.daddr, header_in.sender_id, header_in.dport, 0};

                  // TODO return
                  hashmap.queue({recvspecialize, header_in.local_id});
               } else {
                  rpc_hashpack_t recvspecialize = {header_in.daddr, header_in.sender_id, header_in.dport, 0};

                  header_in.local_id = hashmap.search(recvspecialize);
               }
            } else {
               rpc_hashpack_t query = {header_in.daddr, header_in.sender_id, header_in.dport, 0};
               header_in.local_id = hashmap.search(query);
            }
         }

         // If the incoming message is a response, the RPC ID is already valid in the local store
         header_in_o.write(header_in);
      } else if (!recvmsg_i.empty()) {
         recvmsg = recvmsg_i.read();

         // Create a mapping for incoming data
         rpc_hashpack_t query = {recvmsg.daddr, recvmsg.id, recvmsg.dport, 0};

         hashmap.queue({query, recvmsg.local_id}); 
      } else {
         hashmap.process();
      }
   }
}
