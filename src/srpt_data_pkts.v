`timescale 1ns / 1ps

`define SRPT_INVALIDATE   3'b000
`define SRPT_DBUFF_UPDATE 3'b001
`define SRPT_GRANT_UPDATE 3'b010
`define SRPT_EMPTY        3'b011
`define SRPT_BLOCKED      3'b100
`define SRPT_ACTIVE       3'b101

`define SRPT_DATA_SIZE      76
`define SRPT_DATA_RPC_ID    15:0
`define SRPT_DATA_REMAINING 35:16
`define SRPT_DATA_GRANTED   55:36
`define SRPT_DATA_DBUFFERED 75:56

`define ENTRY_SIZE      79
`define ENTRY_RPC_ID    15:0
`define ENTRY_REMAINING 35:16
`define ENTRY_DBUFFERED 55:36
`define ENTRY_GRANTED   75:56
`define ENTRY_PRIORITY  78:76

`define DBUFF_SIZE   36
`define DBUFF_RPC_ID 15:0
`define DBUFF_OFFSET 35:16

`define GRANT_SIZE   36
`define GRANT_RPC_ID 15:0
`define GRANT_OFFSET 35:16

`define HOMA_PAYLOAD_SIZE 20'h56a

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
      input			       sendmsg_in_empty_i,
      output reg		       sendmsg_in_read_en_o,
      input [`SRPT_DATA_SIZE-1:0]      sendmsg_in_data_i,
      input			       dbuff_in_empty_i,
      output reg		       dbuff_in_read_en_o,
      input [`DBUFF_SIZE-1:0]	       dbuff_in_data_i,
      input			       data_pkt_full_i,
      output reg		       data_pkt_write_en_o,
      output reg [`SRPT_DATA_SIZE-1:0] data_pkt_data_o,
      input                            grant_in_empty_i,
      output reg                       grant_in_read_en_o,
      input [`GRANT_SIZE-1:0]          grant_in_data_i,
      output			       ap_idle, ap_done, ap_ready);

   reg [`ENTRY_SIZE-1:0]        srpt_queue[MAX_SRPT-1:0];
   reg [`ENTRY_SIZE-1:0]        srpt_odd  [MAX_SRPT-1:0]; 
   reg [`ENTRY_SIZE-1:0]        srpt_even [MAX_SRPT-1:0]; 
   reg [`ENTRY_SIZE-1:0]        new_entry;

   reg                          swap_type;

   wire [`ENTRY_SIZE-1:0]       head;

   wire                         head_granted;
   wire                         head_dbuffered;
   wire                         head_ripe;
   wire                         head_empty;

   task prioritize;
      output [`ENTRY_SIZE-1:0] low_o, high_o;
      input  [`ENTRY_SIZE-1:0] low_i, high_i;
      begin
         // If the priority differs or grantable bytes
         if ((low_i[`ENTRY_PRIORITY] != high_i[`ENTRY_PRIORITY]) 
            ? (low_i[`ENTRY_PRIORITY] < high_i[`ENTRY_PRIORITY]) 
            : (low_i[`ENTRY_REMAINING] > high_i[`ENTRY_REMAINING])) begin
            high_o = low_i;
            low_o  = high_i;
            if (low_i[`ENTRY_RPC_ID] == high_i[`ENTRY_RPC_ID]) begin
               if (low_i[`ENTRY_PRIORITY] == `SRPT_DBUFF_UPDATE) begin
                  low_o[`ENTRY_DBUFFERED] = low_i[`ENTRY_DBUFFERED];
                  low_o[`ENTRY_PRIORITY]  = `SRPT_ACTIVE;
               end else if (low_i[`ENTRY_PRIORITY] == `SRPT_GRANT_UPDATE) begin
                  low_o[`ENTRY_GRANTED]   = low_i[`ENTRY_GRANTED];
                  low_o[`ENTRY_PRIORITY]  = `SRPT_ACTIVE;
               end 
            end
         end else begin 
            high_o = high_i;
            low_o  = low_i;
         end 
      end
   endtask 

   assign head           = srpt_queue[0];
   assign head_granted   = (head[`ENTRY_GRANTED] + 1 <= head[`ENTRY_REMAINING])
                           | (head[`ENTRY_GRANTED] == 0);
   assign head_dbuffered = (head[`ENTRY_DBUFFERED] + `HOMA_PAYLOAD_SIZE <= head[`ENTRY_REMAINING]) 
                           | (head[`ENTRY_DBUFFERED] == 0);
   assign head_empty     = head[`ENTRY_REMAINING] == 0;
   assign head_ripe      = head_granted & head_dbuffered & !head_empty;

   integer entry;

   always @* begin
      sendmsg_in_read_en_o = 0;
      dbuff_in_read_en_o   = 0;
      grant_in_read_en_o   = 0;
      data_pkt_write_en_o  = 0;

      data_pkt_data_o[`SRPT_DATA_RPC_ID]    = head[`ENTRY_RPC_ID];
      data_pkt_data_o[`SRPT_DATA_REMAINING] = head[`ENTRY_REMAINING]; 

      new_entry  = 0;

      if (sendmsg_in_empty_i) begin
         new_entry[`ENTRY_RPC_ID]    = sendmsg_in_data_i[`SRPT_DATA_RPC_ID];
         new_entry[`ENTRY_REMAINING] = sendmsg_in_data_i[`SRPT_DATA_REMAINING];
         new_entry[`ENTRY_DBUFFERED] = sendmsg_in_data_i[`SRPT_DATA_DBUFFERED];
         new_entry[`ENTRY_PRIORITY] = `SRPT_ACTIVE;
         sendmsg_in_read_en_o = 1;
      end else if (dbuff_in_empty_i) begin
         new_entry[`ENTRY_RPC_ID]    = dbuff_in_data_i[`DBUFF_RPC_ID];
         new_entry[`ENTRY_DBUFFERED] = dbuff_in_data_i[`DBUFF_OFFSET];
         new_entry[`ENTRY_PRIORITY]  = `SRPT_DBUFF_UPDATE;
         dbuff_in_read_en_o = 1;
      end else if (grant_in_empty_i) begin
         new_entry[`ENTRY_RPC_ID]   = grant_in_data_i[`GRANT_RPC_ID];
         new_entry[`ENTRY_GRANTED]  = grant_in_data_i[`GRANT_OFFSET];
         new_entry[`ENTRY_PRIORITY] = `SRPT_GRANT_UPDATE;
         grant_in_read_en_o = 1;
      end else if (data_pkt_full_i && head[`ENTRY_PRIORITY] == `SRPT_ACTIVE) begin
         if (head_ripe) begin
            data_pkt_write_en_o = 1;
         end
      end 

      prioritize(srpt_odd[0], srpt_odd[1], new_entry, srpt_queue[0]);

      // Compute srpt_odd
      for (entry = 2; entry < MAX_SRPT-1; entry=entry+2) begin
         prioritize(srpt_odd[entry], srpt_odd[entry+1], srpt_queue[entry-1], srpt_queue[entry]);
      end

      // Compute srpt_even
      for (entry = 1; entry < MAX_SRPT; entry=entry+2) begin
         prioritize(srpt_even[entry-1], srpt_even[entry], srpt_queue[entry-1], srpt_queue[entry]);
      end
   end

   integer rst_entry;

   always @(posedge ap_clk) begin

      if (ap_rst) begin
         swap_type <= 0;

         for (rst_entry = 0; rst_entry < MAX_SRPT; rst_entry = rst_entry + 1) begin
            srpt_queue[rst_entry][`ENTRY_RPC_ID]    <= 0;
            srpt_queue[rst_entry][`ENTRY_REMAINING] <= 0; 
            srpt_queue[rst_entry][`ENTRY_DBUFFERED] <= 0; 
            srpt_queue[rst_entry][`ENTRY_GRANTED]   <= 0; 
            srpt_queue[rst_entry][`ENTRY_PRIORITY]  <= `SRPT_EMPTY;
         end
      end else if (ap_ce && ap_start) begin
         if (sendmsg_in_empty_i || dbuff_in_empty_i || grant_in_empty_i) begin
            // Adds either the reactivated message or the update to the queue
            for (entry = 0; entry < MAX_SRPT-1; entry=entry+1) begin
               srpt_queue[entry] <= srpt_odd[entry];
            end 
         end else if (data_pkt_full_i && head[`ENTRY_PRIORITY] == `SRPT_ACTIVE) begin
            if (head_ripe) begin
               if (head[`ENTRY_REMAINING] < `HOMA_PAYLOAD_SIZE) begin
                  srpt_queue[0] <= srpt_queue[1];
                  srpt_queue[1][`ENTRY_PRIORITY]  <= `SRPT_INVALIDATE;
               end else begin
                  srpt_queue[0][`ENTRY_REMAINING] <= head[`ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE;
               end
            end else if (head_empty) begin
               srpt_queue[0] <= srpt_queue[1];
               srpt_queue[1][`ENTRY_PRIORITY]  <= `SRPT_INVALIDATE;
            end else begin
               srpt_queue[0] <= srpt_queue[1];

               srpt_queue[1] <= srpt_queue[0];
               srpt_queue[1][`ENTRY_PRIORITY]  <= `SRPT_BLOCKED;
            end
         end else begin
            if (swap_type == 1'b0) begin
               // Assumes that write does not keep data around
               for (entry = 0; entry < MAX_SRPT; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_even[entry];
               end

               swap_type <= 1'b1;
            end else begin
               for (entry = 1; entry < MAX_SRPT-2; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_odd[entry+1];
               end

               swap_type <= 1'b0;
            end
         end
      end 
   end 

   assign ap_ready = 1;
   assign ap_idle = 0;
   assign ap_done = 1;

endmodule


/**
 * srpt_grant_pkts_tb() - Test bench for srpt data queue
 */
/* verilator lint_off STMTDLY */
//module srpt_data_pkts_tb();
//
//   reg ap_clk=0;
//   reg ap_rst;
//   reg ap_ce;
//   reg ap_start;
//   reg ap_continue;
//
//   wire	ap_idle;
//   wire	ap_done;
//   wire	ap_ready;
//
//   reg                      sendmsg_in_empty_i;
//   wire                     sendmsg_in_read_en_o;
//   reg [`SRPT_DATA_SIZE-1:0]    sendmsg_in_data_i;
//
//   reg                      grant_in_empty_i;
//   wire                     grant_in_read_en_o;
//   reg [`GRANT_SIZE-1:0]    grant_in_data_i;
//
//   reg                      dbuff_in_empty_i;
//   wire                     dbuff_in_read_en_o;
//   reg [`DBUFF_SIZE-1:0]    dbuff_in_data_i;
//
//   reg                      data_pkt_full_i;
//   wire                     data_pkt_write_en_o;
//   wire [`SRPT_DATA_SIZE-1:0] data_pkt_data_o;
//
//   srpt_data_pkts srpt_queue(.ap_clk(ap_clk), 
//			      .ap_rst(ap_rst), 
//			      .ap_ce(ap_ce), 
//			      .ap_start(ap_start), 
//			      .ap_continue(ap_continue), 
//               .sendmsg_in_empty_i(sendmsg_in_empty_i),
//               .sendmsg_in_read_en_o(sendmsg_in_read_en_o),
//               .sendmsg_in_data_i(sendmsg_in_data_i),
//               .grant_in_empty_i(grant_in_empty_i),
//               .grant_in_read_en_o(grant_in_read_en_o),
//               .grant_in_data_i(grant_in_data_i),
//               .dbuff_in_empty_i(dbuff_in_empty_i),
//               .dbuff_in_read_en_o(dbuff_in_read_en_o),
//               .dbuff_in_data_i(dbuff_in_data_i),
//               .data_pkt_full_i(data_pkt_full_i),
//               .data_pkt_write_en_o(data_pkt_write_en_o),
//               .data_pkt_data_o(data_pkt_data_o),
//			      .ap_idle(ap_idle),
//			      .ap_done(ap_done), 
//			      .ap_ready(ap_ready));
//
//   task new_entry(input [15:0] rpc_id, input [19:0] remaining, granted, dbuffered);
//      begin
//      
//	   sendmsg_in_data_i[`SRPT_DATA_RPC_ID]    = rpc_id;
//	   sendmsg_in_data_i[`SRPT_DATA_REMAINING] = remaining;
//	   sendmsg_in_data_i[`SRPT_DATA_GRANTED]   = granted;
//	   sendmsg_in_data_i[`SRPT_DATA_DBUFFERED] = dbuffered;
//
//	   sendmsg_in_empty_i  = 1;
//
//	   #5;
//	   
//	   sendmsg_in_empty_i  = 0;
//   
//      end
//      
//   endtask
//
//   task new_dbuff(input [15:0] rpc_id, input [19:0] dbuffered);
//      begin
//
//         dbuff_in_data_i[`DBUFF_RPC_ID]  = rpc_id;
//         dbuff_in_data_i[`DBUFF_OFFSET]  = dbuffered;
//         
//	      dbuff_in_empty_i  = 1;
//
//	      #5;
//	      
//	      dbuff_in_empty_i  = 0;
//   
//      end
//   endtask
//
//   task new_grant(input [15:0] rpc_id, input [19:0] granted);
//      begin
//
//         grant_in_data_i[`GRANT_RPC_ID] = rpc_id;
//         grant_in_data_i[`GRANT_OFFSET] = granted;
//         
//	      grant_in_empty_i  = 1;
//
//	      #5;
//	      
//	      grant_in_empty_i  = 0;
//   
//      end
//   endtask
//
//   task get_output();
//      begin
//         data_pkt_full_i = 1;
//	      #5;
//	      
//         data_pkt_full_i = 0;
//   
//      end
//   endtask
//
//   /* verilator lint_off INFINITELOOP */
//   
//   initial begin
//      ap_clk = 0;
//
//      forever begin
//	      #2.5 ap_clk = ~ap_clk;
//      end 
//   end
//   
//   initial begin
//      sendmsg_in_data_i = 0;
//      grant_in_data_i   = 0;
//      dbuff_in_data_i   = 0;
//
//	   sendmsg_in_empty_i  = 0;
//	   grant_in_empty_i    = 0;
//	   dbuff_in_empty_i    = 0;
//	   data_pkt_full_i     = 0;
//
//      // Send reset signal
//      ap_ce = 1; 
//      ap_rst = 0;
//      ap_start = 0;
//      ap_rst = 1;
//
//      #5;
//      ap_rst = 0;
//      ap_start = 1;
//
//      #5;
//      new_entry(1, 5000, 5000, 5000);
//
//      // Entry will block and no outputs will be sent
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      #20;
//      new_dbuff(1, 0);
//
//      // Entry is still blocked and no outputs will be sent
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//
//      #20;
//      new_grant(1, 0);
//
//      // Should give an output
//      get_output();
//
//      #20;
//      new_entry(9, 9000, 9000, 9000);
//      new_entry(8, 8000, 8000, 8000);
//      new_entry(7, 7000, 7000, 7000);
//      new_entry(6, 6000, 6000, 6000);
//      new_entry(5, 5000, 5000, 5000);
//      new_entry(4, 4000, 4000, 4000);
//      new_entry(3, 3000, 3000, 3000);
//      new_entry(2, 2000, 2000, 2000);
//
//      #20;
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//      get_output();
//
//
//
//
//      //#5;
//      //if (sendmsg_in_read_en_o != 0 || grant_in_read_en_o != 0 || dbuff_in_read_en_o != 0) begin
//      //   $display("Reading data when unavailible");
//      //   $stop;
//      //end
//
//      //#5;
//
//      //if (data_pkt_write_en_o != 0) begin
//      //   $display("Writing data when unavailible");
//      //   $stop;
//      //end
//
//      //#5;
//     
//      //// RPC ID, Dbuff ID, Remaining, Granted, Dbuffered, Length
//      //new_entry(1, 1, 100000, 100000, 0, 100000);
//
//      //#50;
//      //if (data_pkt_write_en_o != 0) begin
//      //   $display("Writing data that is unready");
//      //   $stop;
//      //end
//
//      //#5;
//      //new_dbuff(1, 50000, 100000);
//
//      //#20;
//      //data_pkt_full_i = 1;
//
//      //#20;
//      //data_pkt_full_i = 0;
//
//      //#20;
//      //new_dbuff(1, 100000, 100000);
//
//      //#100;
//      //data_pkt_full_i = 1;
//
//      //#600;
//      //data_pkt_full_i = 0;
//
//
//
//
//      //#5;
//      //new_entry(2, 2, 10000, 10000, 0, 10000);
//
//      //#5;
//      //new_entry(3, 3, 10000, 10000, 0, 10000);
//
//      //#5;
//      //new_entry(4, 4, 10000, 10000, 0, 10000);
//
//      //#5;
//      //new_entry(5, 5, 10000, 10000, 0, 10000);
//
//      //#5;
//      //new_entry(6, 6, 10000, 10000, 0, 10000);
//
//      //#5;
//      //new_entry(7, 7, 10000, 10000, 0, 10000);
//
//      //#5;
//      //new_entry(8, 8, 10000, 10000, 0, 10000);
//
//      //#5;
//      //new_entry(9, 9, 10000, 10000, 0, 10000);
//
//      //#5;
//      //new_dbuff(2, 10000, 10000);
//
//      //#5;
//      //new_dbuff(3, 10000, 10000);
//
//      //#5;
//      //new_dbuff(4, 10000, 10000);
//
//      //#5;
//      //new_dbuff(5, 10000, 10000);
//
//      //#5;
//      //new_dbuff(6, 10000, 10000);
//
//      //#5;
//      //new_dbuff(7, 10000, 10000);
//
//      //#5;
//      //new_dbuff(8, 10000, 10000);
//
//      //#5;
//      //new_dbuff(9, 10000, 10000);
//
//      // #5;
//      // new_entry(5, 10000, 5000, 0);
//
//
//      // #5;
//      // new_entry(6, 1, 5000, 1000);
//
//
//      // #5;
//      // new_entry(7, 10000, 10000, 10000);
//
//
//
//      // new_entry(4, 4, 4, 0);
//
//      // #15;
//
//      // new_entry(3, 3, 3, 0);
//
//      // #15;
//
//      // new_entry(1, 1, 1, 0);
//
//      // #15;
//
//      // new_entry(2, 2, 2, 0);
//
//      // #200;
//
//      // new_entry(6, 3, 3, 0);
//
//      // #15;
//
//      // new_entry(7, 4, 4, 0);
//      
//      #1000
//	   $stop;
//   end
//
//endmodule
