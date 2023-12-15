`timescale 1ns / 1ps

`define SRPT_INVALIDATE   3'b000
`define SRPT_DBUFF_UPDATE 3'b001
`define SRPT_GRANT_UPDATE 3'b010
`define SRPT_EMPTY        3'b011
`define SRPT_BLOCKED      3'b100
`define SRPT_ACTIVE       3'b101

`define QUEUE_ENTRY_SIZE      104
`define QUEUE_ENTRY_RPC_ID    15:0  // ID of this transaction
`define QUEUE_ENTRY_DBUFF_ID  24:16 // Corresponding on chip cache
`define QUEUE_ENTRY_REMAINING 45:26 // Remaining to be sent or cached
`define QUEUE_ENTRY_DBUFFERED 65:46 // Number of bytes cached
`define QUEUE_ENTRY_GRANTED   85:66 // Number of bytes granted
`define QUEUE_ENTRY_PRIORITY  88:86 // Deprioritize inactive messages

`define HOMA_PAYLOAD_SIZE 20'h56a
`define CACHE_BLOCK_SIZE 64

`define CACHE_SIZE 16384

/**
 * srpt_data_pkts() - Determines which data packet should be transmit
 * next based on the number of remaining bytes to send. Packets
 * however must remain eligible in that they 1) have at least 1 byte
 * of granted data, and 2) the data is availible on chip for their
 * transmission.
 *
 * #MAX_RPCS - The size of the SRPT queue, or the maximum number of
 * RPCs that we could be trying to send to at a single point in time
 *
 * @sendmsg_in_empty_i - 0 value indicates there are no new sendmsg
 * requsts to be read from this input stream while 1 indictes there
 * are sendmsgs on the input stream.  @sendmsg_in_read_en_o -
 * Acknowledgement that the input data has been read
 * @sendmsg_in_data_i  - The actual data content of the incoming stream
 *
 * @grant_in_empty_i - 0 value indicates there are no new grants to be
 * read from this stream while a 1 value indicates there are grants
 * which should be read @grant_in_read_en_o - Acknowledgement that the
 * input grant has been read and can be discarded by the FIFO
 * @grant_in_data_i  - The actual grant content that needs to be added
 * to this queue.
 * 
 * @dbuff_in_empty_i - 0 value indicates there are no data buff
 * notifications that are availible on the input stream while 1
 * indicates there are databuffer notifications we should integrate
 * @dbuff_in_read_en_o - Acknowledgement that the data buffer
 * notification has been read from the input stream
 * @dbuff_in_data_i - The actual notification content of the incoming
 * stream
 * 
 * @data_pkt_full_i - 1 value indicates that the outgoing stream has
 * room for another element while 0 indicates the output stream is
 * full 
 * @data_pkt_write_en_o - Acknowledgement that the data in
 * data_pkt_data_o should be read by the outgoing stream
 * @data_pkt_data_o - The actual outgoing pkts that should be sent
 * onto the link.
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
 * 
 * TODO Need feed back from pktq to sendq
 * 
 * TODO immediate block for short messages is not optimal? This can be buffered from another core?
 *  
 * TODO use a LAST bit to handle when notifications are sent?
 */
module srpt_queue #(parameter MAX_RPCS = 64,
		    parameter TYPE = "sendmsg")
   (input ap_clk, ap_rst, 
    
    input			       S_AXIS_TVALID,
    output reg			       S_AXIS_TREADY,
    input [`QUEUE_ENTRY_SIZE-1:0]      S_AXIS_TDATA,

    output reg			       M_AXIS_TVALID,
    input wire			       M_AXIS_TREADY,
    output reg [`QUEUE_ENTRY_SIZE-1:0] M_AXIS_TDATA);

   // Data out priority queue
   reg [`QUEUE_ENTRY_SIZE-1:0]	       queue[MAX_RPCS-1:0];
   reg [`QUEUE_ENTRY_SIZE-1:0]	       queue_swap_odd[MAX_RPCS-1:0]; 
   reg [`QUEUE_ENTRY_SIZE-1:0]	       queue_swap_even[MAX_RPCS-1:0];
   reg [`QUEUE_ENTRY_SIZE-1:0]	       queue_insert;
   wire [`QUEUE_ENTRY_SIZE-1:0]	       queue_head;
   reg				       queue_swap_polarity;

   wire				       sendq_granted;
   wire				       sendq_dbuffered;
   wire				       sendq_empty;

   reg				       ripe;
   
   
   task prioritize;
      output [`QUEUE_ENTRY_SIZE-1:0] low_o, high_o;
      input [`QUEUE_ENTRY_SIZE-1:0]  low_i, high_i;
      begin
	 if ((low_i[`QUEUE_ENTRY_PRIORITY] != high_i[`QUEUE_ENTRY_PRIORITY]) 
	     ? (low_i[`QUEUE_ENTRY_PRIORITY] < high_i[`QUEUE_ENTRY_PRIORITY]) 
	     : (low_i[`QUEUE_ENTRY_REMAINING] > high_i[`QUEUE_ENTRY_REMAINING])) begin
	    high_o = low_i;
	    low_o  = high_i;

	    if (TYPE == "sendmsg") begin
	       if (low_i[`QUEUE_ENTRY_RPC_ID] == high_i[`QUEUE_ENTRY_RPC_ID]) begin
		  if (low_i[`QUEUE_ENTRY_PRIORITY] == `SRPT_DBUFF_UPDATE) begin
		     low_o[`QUEUE_ENTRY_DBUFFERED] = low_i[`QUEUE_ENTRY_DBUFFERED];
		     low_o[`QUEUE_ENTRY_PRIORITY]  = `SRPT_ACTIVE;
		  end else if (low_i[`QUEUE_ENTRY_PRIORITY] == `SRPT_GRANT_UPDATE) begin
		     low_o[`QUEUE_ENTRY_GRANTED]   = low_i[`QUEUE_ENTRY_GRANTED];
		     low_o[`QUEUE_ENTRY_PRIORITY]  = `SRPT_ACTIVE;
		  end 
	       end
	    end else if (TYPE == "fetch") begin // if ((low_i[`QUEUE_ENTRY_PRIORITY] != high_i[`QUEUE_ENTRY_PRIORITY])...
	       if (low_i[`QUEUE_ENTRY_PRIORITY] == `SRPT_DBUFF_UPDATE 
		   && low_i[`QUEUE_ENTRY_DBUFF_ID] == high_i[`QUEUE_ENTRY_DBUFF_ID]) begin
		  low_o[`QUEUE_ENTRY_DBUFFERED] = low_o[`QUEUE_ENTRY_DBUFFERED] - 1386;
		  low_o[`QUEUE_ENTRY_PRIORITY]  = `SRPT_ACTIVE;
	       end
	    end
	 end else begin 
	    high_o = high_i;
	    low_o  = low_i;
	 end 
      end
   endtask // prioritize_sendq
   
   assign queue_head      = queue[0];
   assign sendq_granted   = (queue_head[`QUEUE_ENTRY_GRANTED] + 1 <= queue_head[`QUEUE_ENTRY_REMAINING])
     | (queue_head[`QUEUE_ENTRY_GRANTED] == 0);
   
   assign sendq_dbuffered = (queue_head[`QUEUE_ENTRY_DBUFFERED] + `HOMA_PAYLOAD_SIZE <= queue_head[`QUEUE_ENTRY_REMAINING]) 
     | (queue_head[`QUEUE_ENTRY_DBUFFERED] == 0);
   assign sendq_empty     = queue_head[`QUEUE_ENTRY_REMAINING] == 0;

   integer entry;

   always @* begin
      S_AXIS_TREADY = 0;
      queue_insert  = 0;
      M_AXIS_TVALID = 0;
      M_AXIS_TDATA  = 0;

      if (TYPE == "sendmsg") begin
	 ripe = sendq_granted & sendq_dbuffered & !sendq_empty;
      end else if (TYPE == "fetch") begin
	 ripe = 1;
      end

      if (S_AXIS_TVALID) begin
	 queue_insert  = S_AXIS_TDATA;
	 S_AXIS_TREADY = 1;
      end else if (ripe && queue_head[`QUEUE_ENTRY_PRIORITY] == `SRPT_ACTIVE) begin // else: !if(S_AXIS_TVALID_SENDMSG)
	 M_AXIS_TDATA  = queue_head;
	 M_AXIS_TVALID = 1; 
      end

      prioritize(queue_swap_odd[0], queue_swap_odd[1], queue_insert, queue[0]);

      // Compute data_swp_odd
      for (entry = 2; entry < MAX_RPCS-1; entry=entry+2) begin
	 prioritize(queue_swap_odd[entry], queue_swap_odd[entry+1], queue[entry-1], queue[entry]);
      end

      // Compute data_swp_even
      for (entry = 1; entry < MAX_RPCS; entry=entry+2) begin
	 prioritize(queue_swap_even[entry-1], queue_swap_even[entry], queue[entry-1], queue[entry]);
      end
   end

   integer rst_entry;

   always @(posedge ap_clk) begin
      if (ap_rst) begin
	 queue_swap_polarity <= 0;
	 for (rst_entry = 0; rst_entry < MAX_RPCS; rst_entry = rst_entry + 1) begin
	    queue[rst_entry] <= 0;
	    queue[rst_entry][`QUEUE_ENTRY_PRIORITY] <= `SRPT_EMPTY;
	 end
      end else begin // if (ap_rst)
	 if (S_AXIS_TVALID) begin
	    // Adds either the reactivated message or the update to the queue
	    for (entry = 0; entry < MAX_RPCS-1; entry=entry+1) begin
	       queue[entry] <= queue_swap_odd[entry];
	    end 
	 end else if (queue_head[`QUEUE_ENTRY_PRIORITY] == `SRPT_ACTIVE) begin
	    // Did the consumer accept the value? 
	    if (M_AXIS_TREADY) begin 
	       if (TYPE == "sendmsg") begin
		  if (ripe) begin
		     if (queue_head[`QUEUE_ENTRY_REMAINING] <= `HOMA_PAYLOAD_SIZE) begin
			queue[0] <= queue[1];
			queue[1][`QUEUE_ENTRY_PRIORITY]  <= `SRPT_INVALIDATE;
		     end else begin
			queue[0][`QUEUE_ENTRY_REMAINING] <= queue_head[`QUEUE_ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE;
		     end
		  end else if (sendq_empty) begin
		     queue[0] <= queue[1];
		     queue[1][`QUEUE_ENTRY_PRIORITY]  <= `SRPT_INVALIDATE;
		  end else begin
		     queue[0] <= queue[1];
		     queue[1] <= queue[0];
		     queue[1][`QUEUE_ENTRY_PRIORITY]  <= `SRPT_BLOCKED;
		  end
	       end else if (TYPE == "fetch") begin // if (TYPE == "sendmsg")
		  // Did we just request the last block for this message
		  if (queue_head[`QUEUE_ENTRY_REMAINING] <= `CACHE_BLOCK_SIZE) begin
		     queue[0]                        <= queue[1];
		     queue[1][`QUEUE_ENTRY_PRIORITY] <= `SRPT_INVALIDATE;
		  end else begin
		     queue[0][`QUEUE_ENTRY_GRANTED]   <= queue[0][`QUEUE_ENTRY_GRANTED] + `CACHE_BLOCK_SIZE;
		     queue[0][`QUEUE_ENTRY_REMAINING] <= queue[0][`QUEUE_ENTRY_REMAINING] - `CACHE_BLOCK_SIZE;
		     queue[0][`QUEUE_ENTRY_DBUFFERED] <= queue[0][`QUEUE_ENTRY_DBUFFERED] + `CACHE_BLOCK_SIZE;
		     // TODO could also swap here?
		     if (queue[0][`QUEUE_ENTRY_DBUFFERED] >= `CACHE_SIZE - `CACHE_BLOCK_SIZE) begin
			queue[0][`QUEUE_ENTRY_PRIORITY] <= `SRPT_BLOCKED;
		     end
		  end
	       end // if (TYPE == "fetch")
	    end
	 end else begin
	    if (queue_swap_polarity == 1'b0) begin
	       // Assumes that write does not keep data around
	       for (entry = 0; entry < MAX_RPCS; entry=entry+1) begin
		  queue[entry] <= queue_swap_even[entry];
	       end

	       queue_swap_polarity <= 1'b1;
	    end else begin
	       for (entry = 1; entry < MAX_RPCS-2; entry=entry+1) begin
		  queue[entry] <= queue_swap_odd[entry+1];
	       end

	       queue_swap_polarity <= 1'b0;
	    end
	 end
      end 
   end 

endmodule // srpt_queue

//task fetch_test_single_entry();
//   begin
//      enqueue(1, 1, 0, 0, 10000);
//      for (i = 0; i < 10000 / `HOMA_PAYLOAD_SIZE + 1; i=i+1) begin
//	 dequeue();
//      end
//      
//   end
//endtask
