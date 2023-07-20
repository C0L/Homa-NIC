`timescale 1ns / 1ps

`define SRPT_GRANT 3'b000
`define SRPT_DBUFF 3'b001
`define SRPT_INVALIDATE 3'b010
`define SRPT_EMPTY 3'b011
`define SRPT_BLOCKED 3'b100
`define SRPT_ACTIVE 3'b101

`define ENTRY_SIZE      115
`define ENTRY_RPC_ID    15:0
`define ENTRY_REMAINING 47:16
`define ENTRY_GRANTED   79:48
`define ENTRY_DBUFFERED 111:80
`define ENTRY_PRIORITY 114:112
`define PRIORITY_SIZE 3

`define DBUFF_SIZE 48
`define DBUFF_RPC_ID 15:0
`define DBUFF_OFFSET 47:16

`define GRANT_SIZE 48
`define GRANT_RPC_ID 15:0
`define GRANT_OFFSET 47:16

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
      input                          sendmsg_in_empty_i,
      output reg                     sendmsg_in_read_en_o,
      input [`ENTRY_SIZE-1:0]        sendmsg_in_data_i,
      input                          grant_in_empty_i,
      output reg                     grant_in_read_en_o,
      input [`GRANT_SIZE-1:0]        grant_in_data_i,
      input                          dbuff_in_empty_i,
      output reg                     dbuff_in_read_en_o,
      input [`DBUFF_SIZE-1:0]        dbuff_in_data_i,
      input                          data_pkt_full_i,
      output reg                     data_pkt_write_en_o,
      output reg [`ENTRY_SIZE-1:0] data_pkt_data_o, 
      output                         ap_idle, ap_done, ap_ready);

   reg [`ENTRY_SIZE-1:0]         srpt_queue[MAX_SRPT-1:0];
   reg [`ENTRY_SIZE-1:0]         srpt_odd[MAX_SRPT-1:0]; 
   reg [`ENTRY_SIZE-1:0]         srpt_even[MAX_SRPT-1:0]; 

   reg                           swap_type;

   reg [`ENTRY_SIZE-1:0]        new_entry;

   integer                       entry;

   always @* begin
   
      // srpt_odd[MAX_SRPT-1] = srpt_queue[MAX_SRPT-1];
      
      if (sendmsg_in_empty_i) begin
         new_entry = sendmsg_in_data_i;
         new_entry[`ENTRY_PRIORITY] = `SRPT_ACTIVE;
      // Are there new grants we should consider
      end else if (dbuff_in_empty_i) begin
         new_entry[`ENTRY_RPC_ID]    = 0;
         new_entry[`ENTRY_REMAINING] = 0;
         new_entry[`ENTRY_DBUFFERED] = dbuff_in_data_i[`DBUFF_OFFSET];
         new_entry[`ENTRY_PRIORITY]  = `SRPT_DBUFF;

      // Are there new data buffer notifications we should consider
      end else if (grant_in_empty_i) begin
         new_entry[`ENTRY_PRIORITY] = `SRPT_GRANT;
         new_entry[`ENTRY_GRANTED]  = grant_in_data_i[`GRANT_OFFSET];
      end else begin
         new_entry = 0;
      end

      if ((new_entry[`ENTRY_PRIORITY] != srpt_queue[0][`ENTRY_PRIORITY]) 
         ? (new_entry[`ENTRY_PRIORITY] < srpt_queue[0][`ENTRY_PRIORITY]) 
         : (new_entry[`ENTRY_REMAINING] > srpt_queue[0][`ENTRY_REMAINING])) begin
            srpt_odd[0] = srpt_queue[0];
            srpt_odd[1] = new_entry;

         if ((new_entry[`ENTRY_RPC_ID] == srpt_queue[0][`ENTRY_RPC_ID]) 
            && (new_entry[`ENTRY_PRIORITY] == `SRPT_GRANT)) begin

            srpt_odd[0][`ENTRY_GRANTED] = new_entry[`ENTRY_GRANTED];

            if ((new_entry[`ENTRY_DBUFFERED] <= (srpt_queue[0][`ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE))
               || (new_entry[`ENTRY_DBUFFERED] == 0)) begin
               srpt_odd[0][`ENTRY_PRIORITY] = `SRPT_ACTIVE;
            end
         end else if ((new_entry[`ENTRY_RPC_ID] == srpt_queue[0][`ENTRY_RPC_ID]) &&
                     (new_entry[`ENTRY_PRIORITY] == `SRPT_DBUFF)) begin

            srpt_odd[0][`ENTRY_DBUFFERED] = new_entry[`ENTRY_DBUFFERED];
         
            if (new_entry[`ENTRY_GRANTED] < srpt_queue[0][`ENTRY_REMAINING]) begin
               srpt_odd[0][`ENTRY_PRIORITY] = `SRPT_ACTIVE;
            end
         end
      end else begin 
         srpt_odd[0] = new_entry;
         srpt_odd[1] = srpt_queue[0];
      end 
      
      // Compute srpt_odd
      for (entry = 2; entry < MAX_SRPT-1; entry=entry+2) begin
         // If the priority differs or grantable bytes
         if ((srpt_queue[entry-1][`ENTRY_PRIORITY] != srpt_queue[entry][`ENTRY_PRIORITY]) 
            ? (srpt_queue[entry-1][`ENTRY_PRIORITY] < srpt_queue[entry][`ENTRY_PRIORITY]) 
            : (srpt_queue[entry-1][`ENTRY_REMAINING] > srpt_queue[entry][`ENTRY_REMAINING])) begin
            srpt_odd[entry-1+1] = srpt_queue[entry];
            srpt_odd[entry+1]   = srpt_queue[entry-1];

            if ((srpt_queue[entry-1][`ENTRY_RPC_ID] == srpt_queue[entry][`ENTRY_RPC_ID]) 
               && (srpt_queue[entry-1][`ENTRY_PRIORITY] == `SRPT_GRANT)) begin

               srpt_odd[entry-1+1][`ENTRY_GRANTED] = srpt_queue[entry-1][`ENTRY_GRANTED];

               if ((srpt_queue[entry-1][`ENTRY_DBUFFERED] <= (srpt_queue[entry][`ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE))
                  || (srpt_queue[entry-1][`ENTRY_DBUFFERED] == 0)) begin
                  srpt_odd[entry-1+1][`ENTRY_PRIORITY] = `SRPT_ACTIVE;
               end
            end else if ((srpt_queue[entry-1][`ENTRY_RPC_ID] == srpt_queue[entry][`ENTRY_RPC_ID]) &&
               (srpt_queue[entry-1][`ENTRY_PRIORITY] == `SRPT_DBUFF)) begin

               srpt_odd[entry-1+1][`ENTRY_DBUFFERED] = srpt_queue[entry-1][`ENTRY_DBUFFERED];
            
               if (srpt_queue[entry-1][`ENTRY_GRANTED] < srpt_queue[entry][`ENTRY_REMAINING]) begin
                  srpt_odd[entry-1+1][`ENTRY_PRIORITY] = `SRPT_ACTIVE;
               end
            end
            
         end else begin 
            srpt_odd[entry+1]   = srpt_queue[entry];
            srpt_odd[entry-1+1] = srpt_queue[entry-1];
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

            if ((srpt_queue[entry-1][`ENTRY_RPC_ID] == srpt_queue[entry][`ENTRY_RPC_ID]) 
               && (srpt_queue[entry-1][`ENTRY_PRIORITY] == `SRPT_GRANT)) begin

               srpt_even[entry-1][`ENTRY_GRANTED] = srpt_queue[entry-1][`ENTRY_GRANTED];

               if ((srpt_queue[entry-1][`ENTRY_DBUFFERED] <= (srpt_queue[entry][`ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE))
                  || (srpt_queue[entry-1][`ENTRY_DBUFFERED] == 0)) begin
                  srpt_even[entry-1][`ENTRY_PRIORITY] = `SRPT_ACTIVE;
               end
            end else if ((srpt_queue[entry-1][`ENTRY_RPC_ID] == srpt_queue[entry][`ENTRY_RPC_ID]) &&
               (srpt_queue[entry-1][`ENTRY_PRIORITY] == `SRPT_DBUFF)) begin

               srpt_even[entry-1][`ENTRY_DBUFFERED] = srpt_queue[entry-1][`ENTRY_DBUFFERED];
            
               if (srpt_queue[entry-1][`ENTRY_GRANTED] < srpt_queue[entry][`ENTRY_REMAINING]) begin
                  srpt_even[entry-1][`ENTRY_PRIORITY] = `SRPT_ACTIVE;
               end
            end
         end else begin 
            srpt_even[entry]   = srpt_queue[entry];
            srpt_even[entry-1] = srpt_queue[entry-1];
         end 
      end
   end

   integer rst_entry;

   always @(posedge ap_clk) begin

      if (ap_rst) begin
         data_pkt_data_o       <= {`ENTRY_SIZE{1'b0}};
         data_pkt_write_en_o   <= 1'b0;

         sendmsg_in_read_en_o <= 0;
         dbuff_in_read_en_o   <= 0;
         grant_in_read_en_o   <= 0;
         data_pkt_write_en_o  <= 1;

         swap_type             <= 1'b0;

         for (rst_entry = 0; rst_entry < MAX_SRPT; rst_entry=rst_entry+1) begin
            srpt_queue[rst_entry][`ENTRY_SIZE-1:0] <= {{(`ENTRY_SIZE-3){1'b0}}, `SRPT_EMPTY};
         end
      end else if (ap_ce && ap_start) begin
         // TODO first check for sending
         if (data_pkt_full_i && srpt_queue[0][`ENTRY_PRIORITY] == `SRPT_ACTIVE) begin
            if ((srpt_queue[0][`ENTRY_GRANTED] < srpt_queue[0][`ENTRY_REMAINING]) 
               && ((srpt_queue[0][`ENTRY_DBUFFERED] <= (srpt_queue[0][`ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE))
               || (srpt_queue[0][`ENTRY_DBUFFERED] == 0))) begin

               data_pkt_data_o <= srpt_queue[0];
               srpt_queue[0][`ENTRY_REMAINING] <= srpt_queue[0][`ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE;

               if (srpt_queue[0][`ENTRY_REMAINING] == 0) begin
                  srpt_queue[0][`ENTRY_PRIORITY] <= `SRPT_INVALIDATE;
               end else if (!((srpt_queue[0][`ENTRY_GRANTED] < srpt_queue[0][`ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE) 
                              && ((srpt_queue[0][`ENTRY_DBUFFERED] <= (srpt_queue[0][`ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE - `HOMA_PAYLOAD_SIZE))
                                 || (srpt_queue[0][`ENTRY_DBUFFERED] == 0)))) begin
                  srpt_queue[0][`ENTRY_PRIORITY] <= `SRPT_BLOCKED;
               end
            end

            sendmsg_in_read_en_o <= 0;
            dbuff_in_read_en_o   <= 0;
            grant_in_read_en_o   <= 0;
            data_pkt_write_en_o  <= 1;

         // Are there new messages we should consider 
         end else if (sendmsg_in_empty_i) begin
            for (entry = 0; entry < MAX_SRPT; entry=entry+1) begin
               srpt_queue[entry] <= srpt_odd[entry];
            end

            sendmsg_in_read_en_o <= 1;
            dbuff_in_read_en_o   <= 0;
            grant_in_read_en_o   <= 0;
            data_pkt_write_en_o  <= 0;

         // Are there new grants we should consider
         end else if (dbuff_in_empty_i) begin

            for (entry = 0; entry < MAX_SRPT; entry=entry+1) begin
               srpt_queue[entry] <= srpt_odd[entry];
            end

            sendmsg_in_read_en_o <= 0;
            dbuff_in_read_en_o   <= 1;
            grant_in_read_en_o   <= 0;
            data_pkt_write_en_o  <= 0;

         // Are there new data buffer notifications we should consider
         end else if (grant_in_empty_i) begin

            for (entry = 0; entry < MAX_SRPT; entry=entry+1) begin
               srpt_queue[entry] <= srpt_odd[entry];
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

endmodule // srpt_grant_queue
