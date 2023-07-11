#include "link.hh"

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "hls_task.h"


/**
 * egress_selector() - Chose which of data packets, grant packets, retransmission
 * packets, and control packets to send next.
 * @data_pkt_i - Next highest priority data packet input
 * @grant_pkt_i - Next highest priority grant packet input
 * @rexmit_pkt_i - Packets requiring retransmission input
 * @header_out_o - Output stream to the RPC store to get info
 * about the RPC before the packet can be sent
 */
void egress_selector(hls::stream<ready_data_pkt_t, VERIF_DEPTH> & data_pkt_i,
      hls::stream<ap_uint<51>, VERIF_DEPTH> & grant_pkt_i,
      hls::stream<header_t, VERIF_DEPTH> & header_out_o) {

#pragma HLS pipeline II=1 style=flp

   ap_uint<51> ready_grant_pkt_raw;
   ready_data_pkt_t ready_data_pkt;

   if (!grant_pkt_i.empty()) {
      ready_grant_pkt_raw = grant_pkt_i.read();
      ready_grant_pkt_t ready_grant_pkt = {ready_grant_pkt_raw(PEER_ID), ready_grant_pkt_raw(RPC_ID), ready_grant_pkt_raw(RECV_PKTS)};

      header_t header_out;
      header_out.type = GRANT;
      header_out.local_id = ready_grant_pkt.rpc_id;
      header_out.grant_offset = ready_grant_pkt.grant_increment;

      header_out.valid = 1;
   
      header_out_o.write(header_out);
   } else if (!data_pkt_i.empty()) {
      ready_data_pkt = data_pkt_i.read();

      ap_uint<32> data_bytes = MIN(ready_data_pkt.remaining, (ap_uint<32>) HOMA_PAYLOAD_SIZE);

      header_t header_out;
      header_out.type = DATA;
      header_out.local_id = ready_data_pkt.rpc_id;
      header_out.packet_bytes = DATA_PKT_HEADER + data_bytes;
      header_out.payload_length = data_bytes + HOMA_DATA_HEADER;
      header_out.grant_offset = ready_data_pkt.granted;
      header_out.message_length = ready_data_pkt.total;
      header_out.processed_bytes = 0;
      header_out.data_offset = ready_data_pkt.total - ready_data_pkt.remaining;
      header_out.segment_length = data_bytes;
      header_out.incoming = ready_data_pkt.granted;
      header_out.valid = 1;

      header_out_o.write(header_out);
   } 
}

/**
 * pkt_chunk_egress() - Take in the packet header data and output
 * structured 64 byte chunks of that packet. Chunks requiring message
 * data will be augmented with that data in the next step. 
 * @header_out_i - Header data that should be sent on the link
 * @chunk_out_o - 64 byte structured packet chunks output to
 * this stream to be augmented with data from the data buffer
 */
void pkt_chunk_egress(hls::stream<header_t, VERIF_DEPTH> & header_out_i,
      hls::stream<out_chunk_t, VERIF_DEPTH> & chunk_out_o) {

   // TODO I am not sure this ping pong would make its way to RTL

   static header_t doublebuff[2];
#pragma HLS array_partition variable=doublebuff type=complete
// #pragma HLS dependence variable=doublebuff intra RAW false
// #pragma HLS dependence variable=doublebuff intra WAR false

   static ap_uint<1> w_pkt = 0;
   static ap_uint<1> r_pkt = 0;

   // Need to decouple reading from input stream and reading from current packet
   if (doublebuff[w_pkt].valid == 0 && !header_out_i.empty()) {
      doublebuff[w_pkt] = header_out_i.read();
      w_pkt++;
   }

#pragma HLS pipeline II=1 style=flp

   // We have data to send
   if (doublebuff[r_pkt].valid == 1) {
   // if (doublebuff[r_pkt].valid == 1 && chunk_out_o.size() < VERIF_DEPTH) {
      header_t & header_out = doublebuff[r_pkt];

      out_chunk_t out_chunk;
      out_chunk.offset = header_out.data_offset;

      ap_uint<512> natural_chunk;

      switch(header_out.type) {
         case DATA: {
            if (header_out.processed_bytes == 0) {


               // Ethernet header
               //htno_set<6>(*((ap_uint<48>*) (out_chunk.buff.data)), MAC_DST); // TODO MAC Dest (set by phy?)
               //htno_set<6>(*((ap_uint<48>*) (out_chunk.buff.data + 6)), MAC_SRC); // TODO MAC Src (set by phy?)
               //htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 12)), ETHERTYPE_IPV6); // Ethertype

               // LSB of packet data is first on link. Should be MSB of header data
               //out_chunk.buff.data(0*8, 6*8-1) = MAC_DST(6*8-1,0);
               //out_chunk.buff.data(6*8, 12*8-1) = MAC_SRC(6*8-1,0);
               natural_chunk(8*64 - 0*8-1 , 8*64 - 6*8) = MAC_DST(6*8-1,0);
               natural_chunk(8*64 - 6*8-1 , 8*64 - 12*8) = MAC_SRC(6*8-1,0);


               //out_chunk.buff.data(14*8-1, 12*8 ) = ETHERTYPE_IPV6(2*8-1,0);
               natural_chunk(8*64 - 12*8 - 1, 8*64 - 14*8) = ETHERTYPE_IPV6(2*8-1,0);

               // IPv6 header
               //htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data + 14)), VTF); // TODO Version/Traffic Class/Flow Label
               //htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 18)), header_out.packet_bytes - PREFACE_HEADER); // Payload Length
               //htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data + 20)), IPPROTO_HOMA); // Next Header
               //htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data + 21)), HOP_LIMIT); // Hop Limit
               //htno_set<16>(*((ap_uint<128>*) (out_chunk.buff.data + 22)), header_out.saddr); // Sender Address
               //htno_set<16>(*((ap_uint<128>*) (out_chunk.buff.data + 38)), header_out.daddr); // Destination Address

               natural_chunk(8*64 - 14*8-1,8*64 - 18*8) = VTF(4*8-1, 0); // TODO split this up now
               natural_chunk(8*64 - 18*8-1,8*64 - 20*8) = (header_out.packet_bytes - PREFACE_HEADER)(2*8-1,0);
               natural_chunk(8*64 - 20*8-1,8*64 - 21*8) = IPPROTO_HOMA(8-1,0);
               natural_chunk(8*64 - 21*8-1,8*64 - 22*8) = HOP_LIMIT(8-1,0);
               natural_chunk(8*64 - 22*8-1,8*64 - 38*8) = header_out.saddr(16*8-1,0);
               natural_chunk(8*64 - 38*8-1,8*64 - 54*8) = header_out.daddr(16*8-1,0);

               // Start of common header
               //htno_set<2>(*((ap_uint<16>*) (natural_chunk + 54)), header_out.sport); // Sender Port
               //htno_set<2>(*((ap_uint<16>*) (natural_chunk + 56)), header_out.dport); // Destination Port

               natural_chunk( 8*64 - 54*8-1, 8*64 - 56*8) = header_out.sport(2*8-1,0);
               natural_chunk( 8*64 - 56*8-1, 8*64 - 58*8) = header_out.dport(2*8-1,0);

               // Unused
               natural_chunk( 8*64 - 58*8-1, 8*64 - 64*8) = 0;

               // Packet block configuration — no data bytes needed
               out_chunk.type = DATA;
               out_chunk.data_bytes = NO_DATA;
               out_chunk.dbuff_id = header_out.dbuff_id;
               header_out.data_offset += NO_DATA;

            } else if (header_out.processed_bytes == 64) {
               // Rest of common header
               //*((ap_uint<16>*) (out_chunk.buff.data)) = 0; // Unused (2 bytes)
               //htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data+2)), DOFF);      // doff (4 byte chunks in data header)
               //htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data+3)), DATA_TYPE); // Type
               //*((ap_uint<16>*) (out_chunk.buff.data+4)) = 0; // Unused (2 bytes)
               //*((ap_uint<16>*) (out_chunk.buff.data+6)) = 0; // Checksum (unused) (2 bytes)
               //*((ap_uint<16>*) (out_chunk.buff.data+8)) = 0; // Unused  (2 bytes)
               //htno_set<8>(*((ap_uint<64>*) (out_chunk.buff.data+10)), header_out.sender_id); // Sender ID

               natural_chunk(8*64 - 0*8-1,8*64 - 2*8) = 0;
               natural_chunk(8*64 - 2*8-1,8*64 - 3*8) = DOFF(8-1,0);
               natural_chunk(8*64 - 3*8-1,8*64 - 4*8) = DATA_TYPE(8-1,0);
               natural_chunk(8*64 - 4*8-1,8*64 - 6*8) = 0;
               natural_chunk(8*64 - 6*8-1,8*64 - 8*8) = 0;
               natural_chunk(8*64 - 8*8-1,8*64 - 10*8) = 0;
               natural_chunk(8*64 - 10*8-1,  8*64 - 18*8) = header_out.sender_id(8*8-1, 0);

               // Data header
               //htno_set<4>(*((ap_uint<3 2>*) (out_chunk.buff.data+18)), header_out.message_length); // Message Length (entire message)
               //htno_set<4>(*((ap_uint<3 2>*) (out_chunk.buff.data+22)), header_out.incoming); // Incoming
               //*((ap_uint<16>*) (out_ch unk.buff.data+26)) = 0; // Cutoff Version (unimplemented) (2 bytes) TODO
               //*((ap_uint<8>*) (out_chunk.buff.data+27)) = 0; // Retransmit (unimplemented) (1 byte) TODO
               //*((ap_uint<8>*) (out_chunk.buff.data+28)) = 0; // Pad (1 byte)
               natural_chunk(8*64 - 18*8-1,8*64 - 22*8) = header_out.message_length(4*8-1, 0);
               natural_chunk(8*64 - 22*8-1,8*64 - 26*8) = header_out.incoming(4*8-1, 0);
               natural_chunk(8*64 - 26*8-1,8*64 - 28*8) = 0;
               natural_chunk(8*64 - 28*8-1,8*64 - 29*8) = 0;
               natural_chunk(8*64 - 29*8-1,8*64 - 30*8) = 0;

               // Data Segment
               //htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data+29)), header_out.data_offset);    // Offset
               //htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data+33)), header_out.segment_length); // Segment Length
               natural_chunk(8*64 - 30*8-1,8*64 - 34*8) = header_out.data_offset(4*8-1, 0);
               natural_chunk(8*64 - 34*8-1,8*64 - 38*8) = header_out.segment_length(4*8-1, 0);

               std::cerr << "WRITING OUT HEADER OFFSET " << header_out.data_offset << std::endl;

               // Ack header
               //*((ap_uint<64>*) (out_chunk.buff.data+37)) = 0; // Client ID (8 bytes) TODO
               //*((ap_uint<16>*) (out_chunk.buff.data+45)) = 0; // Client Port (2 bytes) TODO
               //*((ap_uint<16>*) (out_chunk.buff.data+47)) = 0; // Server Port (2 bytes) TODO
               natural_chunk(8*64 - 38*8-1,8*64 - 46*8) = 0;
               natural_chunk(8*64 - 46*8-1,8*64 - 48*8) = 0;
               natural_chunk(8*64 - 48*8-1,8*64 - 50*8) = 0;

               natural_chunk( 8*64 - 50*8-1, 8*64 - 64*8) = 0;
               // *((ap_uint<112>*) (out_chunk.buff.data+49)) = 0; // Data

               // Packet block configuration — 14 data bytes needed
               out_chunk.type = DATA;
               out_chunk.data_bytes = PARTIAL_DATA;
               out_chunk.dbuff_id = header_out.dbuff_id;
               header_out.data_offset += PARTIAL_DATA;
            } else {
               out_chunk.type = DATA;
               out_chunk.data_bytes = ALL_DATA;
               out_chunk.dbuff_id = header_out.dbuff_id;
               header_out.data_offset += ALL_DATA;
            }
            break;
         }

         //case GRANT: {
         //   if (header_out.processed_bytes == 0) {
         //      // TODO very repetitive

         //      // Ethernet header
         //      // htno_set<6>(*((ap_uint<48>*) (out_chunk.buff.data)), MAC_DST); // TODO MAC Dest (set by phy?)
         //      // htno_set<6>(*((ap_uint<48>*) (out_chunk.buff.data + 6)), MAC_SRC); // TODO MAC Src (set by phy?)
         //      // htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 12)), ETHERTYPE_IPV6); // Ethertype

         //      // // IPv6 header
         //      // htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data + 14)), VTF); // Version/Traffic Class/Flow Label
         //      // htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 18)), header_out.packet_bytes - PREFACE_HEADER); // Payload Length
         //      // htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data + 20)), IPPROTO_HOMA); // Next Header
         //      // htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data + 21)), HOP_LIMIT); // Hop Limit
         //      // htno_set<16>(*((ap_uint<128>*) (out_chunk.buff.data + 22)), header_out.saddr); // Sender Address
         //      // htno_set<16>(*((ap_uint<128>*) (out_chunk.buff.data + 38)), header_out.daddr); // Destination Address

         //      // // Start of common header
         //      // htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 54)), header_out.sport); // Sender Port
         //      // htno_set<2>(*((ap_uint<16>*) (out_chunk.buff.data + 56)), header_out.dport); // Destination Port
         //      // Unused

         //      // Packet block configuration — no data bytes needed
         //      out_chunk.type = GRANT;
         //      out_chunk.data_bytes = NO_DATA;
         //      header_out.data_offset += NO_DATA;

         //   } else if (header_out.processed_bytes == 64) {
         //      // TODO very repetitive

         //      // Rest of common header
         //      //*((ap_uint<16>*) (out_chunk.buff.data)) = 0; // Unused (2 bytes)
         //      //htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data+2)), DOFF);      // doff (4 byte chunks in data header)
         //      //htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data+3)), DATA_TYPE); // Type
         //      //*((ap_uint<16>*) (out_chunk.buff.data+4)) = 0; // Unused (2 bytes)
         //      //*((ap_uint<16>*) (out_chunk.buff.data+6)) = 0; // Checksum (unused) (2 bytes)
         //      //*((ap_uint<16>*) (out_chunk.buff.data+8)) = 0; // Unused  (2 bytes)
         //      //htno_set<8>(*((ap_uint<64>*) (out_chunk.buff.data+10)), header_out.sender_id); // Sender ID

         //      //// Grant header
         //      //htno_set<4>(*((ap_uint<32>*) (out_chunk.buff.data+18)), header_out.grant_offset); // Byte offset of grant
         //      //htno_set<1>(*((ap_uint<8>*) (out_chunk.buff.data+22)), header_out.grant_offset); // TODO Priority

         //      // Packet block configuration — 14 data bytes needed
         //      out_chunk.type = GRANT;
         //      out_chunk.data_bytes = NO_DATA;
         //      header_out.data_offset += NO_DATA;
         //   } 
         //   break;
         //}
      }

      header_out.processed_bytes += 64;

      if (header_out.processed_bytes >= header_out.packet_bytes) {
         std::cerr << "processed_bytes " << header_out.processed_bytes << " packet_bytes " << header_out.packet_bytes << std::endl;
         header_out.valid = 0;
         out_chunk.last = 1;
         doublebuff[r_pkt].valid = 0; // TODO maybe not needed?
         r_pkt++;
      } else {
         out_chunk.last = 0;
      }

      for (int i = 0; i < 64; ++i) {
#pragma HLS  unroll
         out_chunk.buff.data(512 - (i*8) - 1, 512 - 8 - (i*8)) = natural_chunk(7 + (i*8), (i*8));
      }
      
      //out_chunk.buff.data = natural_chunk;
      std::cerr << header_out.processed_bytes << std::endl;
      chunk_out_o.write(out_chunk);
   }
}

/**
 * pkt_chunk_ingress() - Take packet chunks from the link, reconstruct packets,
 * write data to a temporary holding buffer
 * @link_ingress - Raw 64B chunks from the link of incoming packets
 * @header_in_o - Outgoing stream for incoming headers to peer map
 * @chunk_in_o - Outgoing stream for storing data until we know
 * where to place it in DMA space
 *
 * We can't just write directly to DMA space as we don't know where until
 * we have performed the reverse mapping to get a local RPC ID.
 *
 * Could alternatively send all packets through the same path but this approach
 * seems simpler
 */
void pkt_chunk_ingress(hls::stream<raw_stream_t> & link_ingress,
      hls::stream<header_t, VERIF_DEPTH> & header_in_o,
      hls::stream<in_chunk_t, VERIF_DEPTH> & chunk_in_o) {
#pragma HLS pipeline II=1 style=flp

   static header_t header_in;
   static in_chunk_t data_block;

   raw_stream_t raw_stream;
   if (!link_ingress.empty()) {
      raw_stream = link_ingress.read();
   // raw_stream_t raw_stream = link_ingress.read();
      std::cerr << "PROCESSED BYTES: " << header_in.processed_bytes << std::endl;

      ap_uint<512> natural_chunk;

      for (int i = 0; i < 64; ++i) {
#pragma hls unroll
         natural_chunk(512 - (i*8) - 1, 512 - 8 - (i*8)) = raw_stream.data(7 + (i*8), (i*8));
      }

      // For every type of homa packet we need to read at least two blocks
      if (header_in.processed_bytes == 0) {

         header_in.processed_bytes += 64;

         header_in.payload_length(2*8-1,0) = natural_chunk(8*64 - 18*8-1,8*64 - 20*8);
         header_in.saddr(16*8-1,0) = natural_chunk(8*64 - 22*8-1,8*64 - 38*8);
         header_in.daddr(16*8-1,0) = natural_chunk(8*64 - 38*8-1,8*64 - 54*8);
         header_in.sport(2*8-1,0) =  natural_chunk( 8*64 - 54*8-1, 8*64 - 56*8);
         header_in.dport(2*8-1,0) =  natural_chunk( 8*64 - 56*8-1, 8*64 - 58*8);




         //header_in.payload_length(2*8-1,) = raw_stream.data.data(18*8-1,20*8);
         //header_in.saddr(16*8-1,0) = raw_stream.data.data(22*8-1,38*8);
         //header_in.daddr(16*8-1,0) = raw_stream.data.data(38*8-1,54*8);
         //header_in.sport(2*8-1,0) = raw_stream.data.data(54*8-1, 56*8);
         //header_in.dport(2*8-1,0) = raw_stream.data.data(56*8-1, 58*8);

         //header_in.saddr = raw_stream.data.data(303,176);
         //header_in.daddr = raw_stream.data.data(431,304);
         //header_in.sport = raw_stream.data.data(447,432);
         //header_in.dport = raw_stream.data.data(463,448);

      } else if (header_in.processed_bytes == 64) {
         header_in.processed_bytes += 64;

         header_in.type(8-1,0) =  natural_chunk(8*64 - 3*8-1,8*64 - 4*8);        // Packet type
         header_in.sender_id(8*8-1,0) = natural_chunk(8*64 - 10*8-1,  8*64 - 18*8); // Sender RPC ID
         header_in.sender_id = LOCALIZE_ID(header_in.sender_id);

         header_in.valid = 1;

         switch(header_in.type) {
            case GRANT: {

               // TODO
               // Grant header
               //header_in.offset = raw_stream.data.data(175, 144);   // Byte offset of grant
               //header_in.priority = raw_stream.data.data(183, 176); // TODO priority


               //header_in.data_offset(4*8-1,0) = header_in.sender_id( 8*64 - 18*8, 8*64 - 22*8-1);

               break;
            }

            case DATA: {
               header_in.message_length(4*8-1,0) = natural_chunk(8*64 - 18*8-1,8*64 - 22*8);
               header_in.incoming(4*8-1,0) =  natural_chunk(8*64 - 22*8-1,8*64 - 26*8) ; 
               header_in.data_offset(4*8-1,0) = natural_chunk(8*64 - 30*8-1,8*64 - 34*8)  ;
               header_in.segment_length(4*8-1,0) = (8*64 - 34*8-1,8*64 - 38*8)  ;

               std::cerr << "READ NEW PACKET: " << header_in.data_offset << std::endl;

               //header_in.message_length = raw_stream.data.data(175, 144); // Message Length (entire message)
               //header_in.incoming = raw_stream.data.data(207, 176); // Expected Incoming Bytes
               //header_in.data_offset = raw_stream.data.data(263, 232); // Offset in message of segment
               //header_in.segment_length = raw_stream.data.data(295, 264); // Segment Length

               // TODO parse acks

               //std::cerr << "READ PARTIAL BLOCK\n";
               //for (int i = 0; i < 64; ++i) {
               //  printf("%02x", (unsigned char) raw_stream.data.data((i+1)*8 - 1,i*8));
               //}
               //std::cerr << std::endl;

               //out_chunk.buff.data(511, 512-PARTIAL_DATA*8) = double_buff(((subyte_offset + PARTIAL_DATA) * 8)-1, subyte_offset * 8);
               data_block.buff.data(PARTIAL_DATA*8, 0) = raw_stream.data(511, 512-PARTIAL_DATA*8);
               //data_block.buff.data(511, 512-PARTIAL_DATA*8) = raw_stream.data.data(511, 512-PARTIAL_DATA*8);

               //std::cerr << "DATA BLOCK\n";
               //for (int i = 0; i < 64; ++i) {
               //  printf("%02x", (unsigned char) data_block.buff.data((i+1)*8 - 1,i*8));
               //}
               //std::cerr << std::endl;

               data_block.last = raw_stream.last;
               chunk_in_o.write(data_block);

               // std::cerr << "PARTIAL BLOCK IN\n";

               data_block.offset += 14;

               break;
            }
         }

         header_in_o.write(header_in);
      } else {
   
         // TODO need to test TKEEP and change raw_stream def
         data_block.buff.data = raw_stream.data;

         data_block.last = raw_stream.last;

         //std::cerr << "FULL BLOCK IN \n";
         chunk_in_o.write(data_block);

         data_block.offset += 64;
      }

      if (raw_stream.last) {
         std::cerr << "IN STREAM LAST!!!\n";
         data_block.offset = 0;
         header_in.processed_bytes = 0;
      }
   }
}
