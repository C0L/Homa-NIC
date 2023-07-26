`timescale 1ns / 1ps

// `define SRPT_GRANT      3'b000
// `define SRPT_DBUFF      3'b001
`define SRPT_INVALIDATE 3'b010
`define SRPT_EMPTY      3'b011
`define SRPT_BLOCKED    3'b100
`define SRPT_ACTIVE     3'b101

`define ENTRY_SIZE      45
`define ENTRY_REMAINING 31:0
`define ENTRY_DBUFF_ID  41:32
`define ENTRY_PRIORITY  44:42

`define SRPT_DATA_SIZE      157
`define SRPT_DATA_RPC_ID    15:0
`define SRPT_DATA_DBUFF_ID  25:16
`define SRPT_DATA_REMAINING 57:26
`define SRPT_DATA_GRANTED   89:58
`define SRPT_DATA_DBUFFERED 121:90
`define SRPT_DATA_MSG_LEN   153:122
`define SRPT_DATA_PRIORITY  156:154

`define DBUFF_SIZE     74
`define DBUFF_DBUFF_ID 9:0
`define DBUFF_MSG_LEN  41:10
`define DBUFF_OFFSET   73:42

`define GRANT_SIZE   42
`define GRANT_DBUFF_ID 9:0
`define GRANT_OFFSET 41:10

`define HOMA_PAYLOAD_SIZE 32'h56a

/**
 * srpt_grant_pkts() - Determines which data packet should be transmit next
 * based on the number of remaining bytes to send. Packets however must
 * remain eligible in that they 1) have at least 1 byte of granted data, and
 * 2) the data is availible on chip for their transmission 
 *
 * #MAX_SRPT - The size of the SRPT queue, or the maximum number of RPCs that
 * we could be trying to send to at a single point in time
 *
 * @sendmsg_in_empty_i   - 0 value indicates there are no new sendmsg requsts
 * to be read from this input stream while 1 indictes there are sendmsgs on the
 * input stream.
 * @sendmsg_in_read_en_o - Acknowledgement that the input data has been read
 * @sendmsg_in_data_i    - The actual data content of the incoming stream
 *
 * @grant_in_empty_i   - 0 value indicates there are no new grants to be read
 * from this stream while a 1 value indicates there are grants which should be
 * read
 * @grant_in_read_en_o - Acknowledgement that the input grant has been read
 * and can be discarded by the FIFO
 * @grant_in_data_i    - The actual grant content that needs to be added to
 * this queue.
 *
 * @dbuff_in_empty_i   - 0 value indicates there are no data buff
 * notifications that are availible on the input stream while 1 indicates
 * there are databuffer notifications we should integrate
 * @dbuff_in_read_en_o - Acknowledgement that the data buffer notification has
 * been read from the input stream
 * @dbuff_in_data_i    - The actual notification content of the incoming
 * stream
 *
 * @data_pkt_full_o     - 0 value indicates that the outgoing stream has room
 * for another data packet that should be sent while a 1 value indicates that
 * the stream is full
 * @data_pkt_write_en_o - Acknowledgement that the data in data_pkt_data_o
 * should be added to the outgoing stream
 * @data_pkt_data_o     - The actual outgoing pkts that should be sent onto
 * the link.
 *
 * There are 4 operations within the data SRPT queue:
 *    1) A sendmsg requests arrives which means there is a new packet whose
 *    data we want to send. A new sendmsg requests always begins with some
 *    number of granted bytes (unscheduled), and we assume that this RPCs
 *    outgoing data has already been buffered on chip as a part of the initial
 *    sendmsg requests (this should not break things if not however).
 *    2) A grant arrives which needs to update the grant offset for the
 *    respective entry in the SRPT queue, and potentially unblock it. This is
 *    accomplished by submitting an UPDATE message to the queue.
 *    3) A data buffer notification arrives which needs to update the data
 *    buffer offset for the respective entry in the SRPT queue, and
 *    potentially unblock it. This is also accomplished by submitting an
 *    UPDATE message to the queue.
 *    4) The head of the SRPT queue is active, granted, and data buffered, and
 *    we send this value out on the outgoing stream so that packet may be
 *    sent. The values in that entry are updated to reflect that a single
 *    payload size of data has been sent
 */
module srpt_data_pkts #(parameter MAX_SRPT = 1024)
     (input ap_clk, ap_rst, ap_ce, ap_start, ap_continue,
      input                            sendmsg_in_empty_i,
      output reg                       sendmsg_in_read_en_o,
      input [`SRPT_DATA_SIZE-1:0]      sendmsg_in_data_i,
      input                            grant_in_empty_i,
      output reg                       grant_in_read_en_o,
      input [`GRANT_SIZE-1:0]          grant_in_data_i,
      input                            dbuff_in_empty_i,
      output reg                       dbuff_in_read_en_o,
      input [`DBUFF_SIZE-1:0]          dbuff_in_data_i,
      input                            data_pkt_full_i,
      output reg                       data_pkt_write_en_o,
      output reg [`SRPT_DATA_SIZE-1:0] data_pkt_data_o, 
      output                         ap_idle, ap_done, ap_ready);

   reg [`ENTRY_SIZE-1:0]         srpt_queue[MAX_SRPT-1:0];
   reg [`ENTRY_SIZE-1:0]         srpt_odd[MAX_SRPT-1:0]; 
   reg [`ENTRY_SIZE-1:0]         srpt_even[MAX_SRPT-1:0]; 

   reg [`SRPT_DATA_SIZE-1:0]     entries[MAX_SRPT-1:0];

   reg                           swap_type;

   reg [`ENTRY_SIZE-1:0]        sendmsg_insert[1:0];
   reg [`ENTRY_SIZE-1:0]        dbuff_insert[1:0];
   reg [`ENTRY_SIZE-1:0]        grant_insert[1:0];
   reg [`ENTRY_SIZE-1:0]        invalidate_entry[1:0];
   reg [`ENTRY_SIZE-1:0]        decr_entry[1:0];

   wire                          will_invalidate;
   wire                          sendmsg_unblock;
   wire                          dbuff_unblock;
   wire                          grant_unblock;

   assign will_invalidate = (entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_REMAINING] 
      <= `HOMA_PAYLOAD_SIZE) 
      || ((entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_MSG_LEN] 
      - entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_GRANTED]) 
      < srpt_queue[0][`ENTRY_REMAINING])
      || ((entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_MSG_LEN] 
      - entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_DBUFFERED]) 
      <= (srpt_queue[0][`ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE))
      || (entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_DBUFFERED] 
      >= entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_MSG_LEN]);

   assign sendmsg_unblock = ((sendmsg_in_data_i[`SRPT_DATA_REMAINING] != 0)
         || ((sendmsg_in_data_i[`SRPT_DATA_MSG_LEN] 
         - sendmsg_in_data_i[`SRPT_DATA_GRANTED]) 
         < sendmsg_in_data_i[`SRPT_DATA_REMAINING])
         || ((sendmsg_in_data_i[`SRPT_DATA_MSG_LEN] 
         - sendmsg_in_data_i[`SRPT_DATA_DBUFFERED]) 
         <= (sendmsg_in_data_i[`SRPT_DATA_REMAINING] 
         - `HOMA_PAYLOAD_SIZE))
         || (sendmsg_in_data_i[`SRPT_DATA_DBUFFERED] 
         >= sendmsg_in_data_i[`SRPT_DATA_MSG_LEN]));

   assign dbuff_unblock = ((entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_REMAINING] != 0)
         || ((entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_MSG_LEN] 
         - entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_GRANTED]) 
         < entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_REMAINING])
         || ((entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_MSG_LEN] 
         - entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_DBUFFERED]) 
         <= (entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`ENTRY_REMAINING] 
         - `HOMA_PAYLOAD_SIZE))
         || (entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_DBUFFERED] 
         >= entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_MSG_LEN]));

   assign grant_unblock = ((entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_REMAINING] != 0)
         || ((entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_MSG_LEN] 
         - entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_GRANTED]) 
         < entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_REMAINING])
         || ((entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_MSG_LEN] 
         - entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_DBUFFERED]) 
         <= (entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_REMAINING] 
         - `HOMA_PAYLOAD_SIZE))
         || (entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_DBUFFERED] 
         >= entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_MSG_LEN]));

   integer                       entry;

   always @* begin

      if ((`SRPT_ACTIVE != srpt_queue[0][`ENTRY_PRIORITY]) 
        ? (`SRPT_ACTIVE < srpt_queue[0][`ENTRY_PRIORITY]) 
        : (sendmsg_in_data_i[`ENTRY_REMAINING] > srpt_queue[0][`ENTRY_REMAINING])) begin
         sendmsg_insert[0] = srpt_queue[0];
         sendmsg_insert[1][`ENTRY_REMAINING] = sendmsg_in_data_i[`ENTRY_REMAINING];
         sendmsg_insert[1][`ENTRY_DBUFF_ID]  = sendmsg_in_data_i[`ENTRY_DBUFF_ID];
         sendmsg_insert[1][`ENTRY_PRIORITY]  = `SRPT_ACTIVE;
      end else begin 
         sendmsg_insert[0][`ENTRY_REMAINING] = sendmsg_in_data_i[`ENTRY_REMAINING];
         sendmsg_insert[0][`ENTRY_DBUFF_ID]  = sendmsg_in_data_i[`ENTRY_DBUFF_ID];
         sendmsg_insert[0][`ENTRY_PRIORITY]  = `SRPT_ACTIVE;
         sendmsg_insert[1] = srpt_queue[0];
      end 

      if ((`SRPT_ACTIVE != srpt_queue[0][`ENTRY_PRIORITY]) 
        ? (`SRPT_ACTIVE < srpt_queue[0][`ENTRY_PRIORITY]) 
        : (entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_REMAINING] > srpt_queue[0][`ENTRY_REMAINING])) begin
         grant_insert[0] = srpt_queue[0];
         grant_insert[1][`ENTRY_REMAINING] = entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_REMAINING];
         grant_insert[1][`ENTRY_DBUFF_ID]  = entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_DBUFF_ID];
         grant_insert[1][`ENTRY_PRIORITY]  = `SRPT_ACTIVE;
      end else begin 
         grant_insert[0][`ENTRY_REMAINING] = entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_REMAINING];
         grant_insert[0][`ENTRY_DBUFF_ID]  = entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_DBUFF_ID];
         grant_insert[0][`ENTRY_PRIORITY]  = `SRPT_ACTIVE;
         grant_insert[1] = srpt_queue[0];
      end 

      if ((`SRPT_ACTIVE != srpt_queue[0][`ENTRY_PRIORITY]) 
        ? (`SRPT_ACTIVE < srpt_queue[0][`ENTRY_PRIORITY]) 
        : (entries[dbuff_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_REMAINING] > srpt_queue[0][`ENTRY_REMAINING])) begin
         dbuff_insert[0] = srpt_queue[0];
         dbuff_insert[1][`ENTRY_REMAINING] = entries[dbuff_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_REMAINING];
         dbuff_insert[1][`ENTRY_DBUFF_ID]  = entries[dbuff_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_DBUFF_ID];
         dbuff_insert[1][`ENTRY_PRIORITY]  = `SRPT_ACTIVE;
      end else begin 
         dbuff_insert[0][`ENTRY_REMAINING] = entries[dbuff_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_REMAINING];
         dbuff_insert[0][`ENTRY_DBUFF_ID]  = entries[dbuff_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_DBUFF_ID];
         dbuff_insert[0][`ENTRY_PRIORITY]  = `SRPT_ACTIVE;
         dbuff_insert[1] = srpt_queue[0];
      end 

      // Compute srpt_odd
      for (entry = 2; entry < MAX_SRPT-1; entry=entry+2) begin
         // If the priority differs or grantable bytes
         if ((srpt_queue[entry-1][`ENTRY_PRIORITY] != srpt_queue[entry][`ENTRY_PRIORITY]) 
            ? (srpt_queue[entry-1][`ENTRY_PRIORITY] < srpt_queue[entry][`ENTRY_PRIORITY]) 
            : (srpt_queue[entry-1][`ENTRY_REMAINING] > srpt_queue[entry][`ENTRY_REMAINING])) begin
            srpt_odd[entry] = srpt_queue[entry];
            srpt_odd[entry+1]   = srpt_queue[entry-1];
         end else begin 
            srpt_odd[entry+1]   = srpt_queue[entry];
            srpt_odd[entry] = srpt_queue[entry-1];
         end 
      end

      // Compute srpt_even
      for (entry = 1; entry < MAX_SRPT; entry=entry+2) begin
         // If the priority differs or grantable bytes
         if ((srpt_queue[entry-1][`ENTRY_PRIORITY] != srpt_queue[entry][`ENTRY_PRIORITY]) 
            ? (srpt_queue[entry-1][`ENTRY_PRIORITY] < srpt_queue[entry][`ENTRY_PRIORITY]) 
            : (srpt_queue[entry-1][`ENTRY_REMAINING] > srpt_queue[entry][`ENTRY_REMAINING])) begin
            srpt_even[entry]   = srpt_queue[entry-1];
            srpt_even[entry-1] = srpt_queue[entry];
         end else begin 
            srpt_even[entry]   = srpt_queue[entry];
            srpt_even[entry-1] = srpt_queue[entry-1];
         end 
      end
   end

   integer rst_entry;

   always @(posedge ap_clk) begin

      if (ap_rst) begin
         data_pkt_data_o       <= {`SRPT_DATA_SIZE{1'b0}};
         data_pkt_write_en_o   <= 1'b0;

         sendmsg_in_read_en_o <= 0;
         dbuff_in_read_en_o   <= 0;
         grant_in_read_en_o   <= 0;
         data_pkt_write_en_o  <= 1;

         swap_type            <= 1'b0;

         for (rst_entry = 0; rst_entry < MAX_SRPT; rst_entry=rst_entry+1) begin
            srpt_queue[rst_entry][`ENTRY_SIZE-1:0] <= {{(`ENTRY_SIZE-3){1'b0}}, `SRPT_EMPTY};
         end
      end else if (ap_ce && ap_start) begin
         // Is there room on the output stream and is the head of the queue active
         if (data_pkt_full_i && srpt_queue[0][`ENTRY_PRIORITY] == `SRPT_ACTIVE) begin

            data_pkt_data_o[`SRPT_DATA_REMAINING] <= srpt_queue[0][`ENTRY_REMAINING]; 
            data_pkt_data_o[`SRPT_DATA_RPC_ID]    <= entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_RPC_ID];
            data_pkt_data_o[`SRPT_DATA_DBUFF_ID]  <= entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_DBUFF_ID];
            data_pkt_data_o[`SRPT_DATA_GRANTED]   <= entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_GRANTED]; 
            data_pkt_data_o[`SRPT_DATA_DBUFFERED] <= entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_DBUFFERED];
            data_pkt_data_o[`SRPT_DATA_MSG_LEN]   <= entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_MSG_LEN]; 

            /* We can deactivate an entry from the SRPT queue when 
             *   1) There are no more remaining bytes
             *   2) There are not enough grantable bytes
             *   3) There are not enough databuffered bytes
             */
            if (will_invalidate) begin

               // Save the remaining bytes value 
               entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_REMAINING] <= srpt_queue[0][`ENTRY_REMAINING];
               entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_PRIORITY]  <= `SRPT_BLOCKED;

               // Invalidate and swap the srpt_queue[0] value we just read from
               for (entry = 2; entry < MAX_SRPT; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_even[entry];
               end

               srpt_queue[0] <= srpt_even[1];
               srpt_queue[1][`ENTRY_PRIORITY]  <= `SRPT_INVALIDATE;
            end 
            
            // If the RPC is complete then remove it from the entries list
            if (entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_REMAINING] <= `HOMA_PAYLOAD_SIZE) begin
               entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_RPC_ID]     <= 0;
               entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_DBUFF_ID]   <= 0;
               entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_GRANTED]    <= 0; 
               entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_DBUFFERED]  <= 0;
               entries[srpt_queue[0][`ENTRY_DBUFF_ID]][`SRPT_DATA_MSG_LEN]    <= 0; 
            end

            sendmsg_in_read_en_o <= 0;
            dbuff_in_read_en_o   <= 0;
            grant_in_read_en_o   <= 0;
            data_pkt_write_en_o  <= 1;

         // Are there new messages we should consider 
         end else if (sendmsg_in_empty_i) begin

            /* The dbuffered value is either 0, and we do not need to update
             * it here. Or, the value has been set by a dbuffer notif and we
             * should not overwrite it.
             */
            entries[sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]][`SRPT_DATA_RPC_ID]    <= sendmsg_in_data_i[`SRPT_DATA_RPC_ID];
            entries[sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]][`SRPT_DATA_DBUFF_ID]  <= sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID];
            entries[sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]][`SRPT_DATA_REMAINING] <= sendmsg_in_data_i[`SRPT_DATA_REMAINING];
            entries[sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]][`SRPT_DATA_GRANTED]   <= sendmsg_in_data_i[`SRPT_DATA_GRANTED];
            entries[sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]][`SRPT_DATA_MSG_LEN]   <= sendmsg_in_data_i[`SRPT_DATA_MSG_LEN];

            /* An entry begins as active if:
             *   1) There are remaining bytes to send
             *   2) There is at least 1 more granted byte
             *   3) There is enough data buffered for one whole packet or to
             *   reach the end of the mesage
             */
            if (sendmsg_unblock) begin

               entries[sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]][`SRPT_DATA_PRIORITY] <= `SRPT_ACTIVE;
               
               // Adds the entry to the queue  
               for (entry = 1; entry < MAX_SRPT; entry=entry+1) begin
                  srpt_queue[entry+1] <= srpt_odd[entry];
               end 
               
               srpt_queue[0] <= sendmsg_insert[0];
               srpt_queue[1] <= sendmsg_insert[1];
                  
            end else begin
               entries[sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]][`SRPT_DATA_PRIORITY] <= `SRPT_BLOCKED;
            end
  
            sendmsg_in_read_en_o <= 1;
            dbuff_in_read_en_o   <= 0;
            grant_in_read_en_o   <= 0;
            data_pkt_write_en_o  <= 0;

         // Are there new grants we should consider
         end else if (dbuff_in_empty_i) begin

            /* Fow new databuffer notifications, we:
             *  1) Update the corresponding value in entries
             *  2) Check if the RPC value is non-zero, indicating this entry is alive
             *  3) If the entry is alive and blocked, and we just made the entry re-eligible, 
             *  then re-add it to the queue
             *  4) If the entry is alive and active, do nothing
             */
            entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_DBUFFERED] <= dbuff_in_data_i[`DBUFF_OFFSET];

            if (dbuff_unblock) begin
               if (entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_PRIORITY] == `SRPT_BLOCKED) begin
                  entries[sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]][`SRPT_DATA_PRIORITY] <= `SRPT_ACTIVE;
               
                  // Adds the entry to the queue  
                  for (entry = 1; entry < MAX_SRPT; entry=entry+1) begin
                     srpt_queue[entry+1] <= srpt_odd[entry];
                  end 
                  
                  srpt_queue[0] <= dbuff_insert[0];
                  srpt_queue[1] <= dbuff_insert[1];
               end
            end 

            sendmsg_in_read_en_o <= 0;
            dbuff_in_read_en_o   <= 1;
            grant_in_read_en_o   <= 0;
            data_pkt_write_en_o  <= 0;

         // Are there new data buffer notifications we should consider
         end else if (grant_in_empty_i) begin

            entries[grant_in_data_i[`GRANT_DBUFF_ID]][`SRPT_DATA_DBUFFERED] <= grant_in_data_i[`GRANT_OFFSET];

            if (grant_unblock) begin
               if (entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_PRIORITY] == `SRPT_BLOCKED) begin
                  entries[sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]][`SRPT_DATA_PRIORITY] <= `SRPT_ACTIVE;

                  if (entries[dbuff_in_data_i[`DBUFF_DBUFF_ID]][`SRPT_DATA_PRIORITY] == `SRPT_BLOCKED) begin
                     entries[sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]][`SRPT_DATA_PRIORITY] <= `SRPT_ACTIVE;
                  
                     // Adds the entry to the queue  
                     for (entry = 1; entry < MAX_SRPT; entry=entry+1) begin
                        srpt_queue[entry+1] <= srpt_odd[entry];
                     end 
                     
                     srpt_queue[0] <= grant_insert[0];
                     srpt_queue[1] <= grant_insert[1];
                  end
               
               end 
            end 

            sendmsg_in_read_en_o <= 0;
            dbuff_in_read_en_o   <= 0;
            grant_in_read_en_o   <= 1;
            data_pkt_write_en_o  <= 0;

         end else begin

            if (swap_type == 1'b0) begin
               // Assumes that write does not keep data around
               for (entry = 0; entry < MAX_SRPT; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_even[entry];
               end

               swap_type <= 1'b1;
            end else begin
               for (entry = 1; entry < MAX_SRPT; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_odd[entry+1];
               end

               swap_type <= 1'b0;
            end

            sendmsg_in_read_en_o <= 0;
            dbuff_in_read_en_o   <= 0;
            grant_in_read_en_o   <= 0;
            data_pkt_write_en_o  <= 0;

         end
      end
   end 

   assign ap_ready = 1;
   assign ap_idle = 0;
   assign ap_done = 1;

endmodule
