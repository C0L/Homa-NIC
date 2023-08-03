#include "databuff.hh"
#include "stack.hh"
#include "fifo.hh"

/**
 * dbuff_ingress() - Buffer incoming data chunks while waiting for lookup on
 * packet header to determine DMA destination.
 * @chunk_in_o - Incoming data chunks from link destined for DMA
 * @dma_w_req_o - Outgoing requests for data to be placed in DMA
 * @header_in_i - Incoming headers that determine DMA placement
 */
void dbuff_ingress(hls::stream<in_chunk_t> & chunk_in_o,
      hls::stream<dma_w_req_raw_t> & dma_w_req_o,
      hls::stream<header_t> & header_in_i,
      hls::stream<header_t> & header_in_o) {

#pragma HLS pipeline II=1

   // TODO what size should this be actually?
   static fifo_t<in_chunk_t,128> rebuff;

   // TODO can this be problematic?
   //#pragma HLS dependence variable=rebuff inter WAR false
   //#pragma HLS dependence variable=rebuff inter RAW false

   static header_t header_in;

   if (header_in.valid && !rebuff.empty()) {
      in_chunk_t in_chunk = rebuff.remove();

      // Place chunk in DMA space at global offset + packet offset
      dma_w_req_raw_t dma_w_req;

      dma_w_req(DMA_W_REQ_OFFSET) = in_chunk.offset + ((header_in.dbuff_id) * HOMA_MAX_MESSAGE_LENGTH) + header_in.data_offset;
      dma_w_req(DMA_W_REQ_BLOCK)  = in_chunk.buff;

      dma_w_req_o.write(dma_w_req);

      if (in_chunk.last) header_in.valid = 0;
   }

   in_chunk_t in_chunk;
   if (!chunk_in_o.empty()) {
      in_chunk = chunk_in_o.read(); 
      rebuff.insert(in_chunk);
   }

   if (!header_in.valid && !header_in_i.empty()) {
      header_in = header_in_i.read();
      header_in_o.write(header_in);
   } 
}

/**
 * dbuff_egress() - Augment outgoing packet chunks with packet data
 * @dbuff_egress_i - Input stream of data that needs to be inserted into the on-chip
 * buffer. Contains the block index of the insertion, and the raw data.
 * @dbuff_notif_o  - Notification to the SRPT core that data is ready on-chip
 * @out_chunk_i    - Input stream for outgoing packets that need to be
 * augmented with data from the data buffer. Each chunk indicates the number of
 * bytes it will need to accumulate from the data buffer, which data buffer,
 * and what global byte offset in the message to send.
 * @link_egress    - Outgoing path to link. 64 byte chunks are placed on the link
 * until the final chunk in a packet is placed on the link with the "last" bit
 * set, indicating a completiton of packet transmission.
 * TODO Needs to request data from DMA to keep the RB saturated with pkt data
 */
void dbuff_egress(hls::stream<onboard_send_t> & onboard_send_i,
      hls::stream<onboard_send_t> & onboard_send_o,
      hls::stream<dbuff_in_raw_t> & dbuff_egress_i,
      hls::stream<srpt_dbuff_notif_t> & dbuff_notif_o,
      hls::stream<out_chunk_t> & out_chunk_i,
      hls::stream<out_chunk_t> & out_chunk_o, 
      hls::stream<dma_r_req_raw_t> & dma_r_req_o) {

#pragma HLS pipeline II=1

   static stack_t<dbuff_id_t, NUM_DBUFF> dbuff_stack(true);

   // 1024 x 2^14 byte buffers
   static dbuff_t dbuff[NUM_DBUFF];
   // #pragma HLS bind_storage variable=dbuff type=RAM_1WNR

   static onboard_send_t onboard_send;

   if (!onboard_send_i.empty() && onboard_send.local_id == 0) {
      onboard_send = onboard_send_i.read();
      onboard_send.dbuff_id = dbuff_stack.pop();

      uint32_t num_chunks = onboard_send.iov_size / DBUFF_CHUNK_SIZE;
      if (onboard_send.iov_size % DBUFF_CHUNK_SIZE != 0) num_chunks++;

      dma_r_req_raw_t dma_r_req;
      dma_r_req(DMA_R_REQ_OFFSET)   = onboard_send.iov;
      dma_r_req(DMA_R_REQ_BURST)    = num_chunks;
      dma_r_req(DMA_R_REQ_MSG_LEN)  = onboard_send.iov_size;
      dma_r_req(DMA_R_REQ_DBUFF_ID) = onboard_send.dbuff_id;

      dma_r_req_o.write(dma_r_req);
   } 

   // Do we need to add any data to data buffer 
   if (!dbuff_egress_i.empty()) {
      dbuff_in_raw_t dbuff_in = dbuff_egress_i.read();

      dbuff[dbuff_in(DBUFF_IN_DBUFF_ID)][dbuff_in(DBUFF_IN_CHUNK) % DBUFF_NUM_CHUNKS] = dbuff_in(DBUFF_IN_DATA); 
      ap_uint<32> message_offset = (dbuff_in(DBUFF_IN_CHUNK)+1) * DBUFF_CHUNK_SIZE;
      // dbuff[dbuff_in(DBUFF_IN_DBUFF_ID)] = message_offset;

      if (dbuff_in(DBUFF_IN_LAST)) {
         std::cerr << "MESSAGE OFFSET " << message_offset << std::endl;
         std::cerr << "MESSAGE IOV " << onboard_send.iov_size << std::endl;
         onboard_send.dbuffered = onboard_send.iov_size - MIN(message_offset, onboard_send.iov_size);
         onboard_send_o.write(onboard_send);
      }

      // TODO reintegrate this
      if (dbuff_in(DBUFF_IN_LAST)) {
         srpt_dbuff_notif_t dbuff_notif;
         dbuff_notif(SRPT_DBUFF_NOTIF_RPC_ID) = dbuff_in(DBUFF_IN_RPC_ID);
         // TODO cannot reference onboard in this process
         dbuff_notif(SRPT_DBUFF_NOTIF_OFFSET)   = MIN(message_offset, onboard_send.iov_size);

         dbuff_notif_o.write(dbuff_notif);
         // onboard_send_o.write(onboard_send);
      }
   }

   // Do we need to process any packet chunks?
   if (!out_chunk_i.empty()) {
      // TODO need to dispatch requests in response to an entire packet being read
      out_chunk_t out_chunk = out_chunk_i.read();

      // Is this a data type packet?
      if (out_chunk.type == DATA) {
         int curr_byte = out_chunk.offset % (DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS);

         int block_offset = curr_byte / DBUFF_CHUNK_SIZE;
         int subyte_offset = curr_byte - (block_offset * DBUFF_CHUNK_SIZE);

         integral_t c0 = dbuff[out_chunk.dbuff_id][block_offset];
         integral_t c1 = dbuff[out_chunk.dbuff_id][block_offset+1];

         ap_uint<1024> double_buff;

#pragma HLS array_partition variable=double_buff complete

         double_buff(511,0)    = c0(511,0);
         double_buff(1023,512) = c1(511,0);

         // There is a more obvious C implementation — results in very expensive hardware 
         switch(out_chunk.data_bytes) {
            case NO_DATA: {
               break;
            }

            case ALL_DATA: {
               out_chunk.buff(511, 0) = double_buff(((subyte_offset + ALL_DATA) * 8)-1 , subyte_offset * 8);
               break;
            }

            case PARTIAL_DATA: {
               out_chunk.buff(511, 512-PARTIAL_DATA*8) = double_buff(((subyte_offset + PARTIAL_DATA) * 8)-1, subyte_offset * 8);
               break;
            }
         }
      } 

      out_chunk_o.write(out_chunk);
   }
}
