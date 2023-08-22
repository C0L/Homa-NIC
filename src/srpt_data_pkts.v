`timescale 1ns / 1ps

`define SRPT_INVALIDATE   3'b000
`define SRPT_DBUFF_UPDATE 3'b001
`define SRPT_GRANT_UPDATE 3'b010
`define SRPT_EMPTY        3'b011
`define SRPT_BLOCKED      3'b100
`define SRPT_ACTIVE       3'b101

`define SENDMSG_SIZE       86
`define SENDMSG_RPC_ID     15:0
`define SENDMSG_MSG_LEN    35:16
`define SENDMSG_GRANTED    55:36
`define SENDMSG_DBUFF_ID   65:56
`define SENDMSG_DMA_OFFSET 85:66

`define PKTQ_SIZE      99
`define PKTQ_RPC_ID    15:0
`define PKTQ_DBUFF_ID  24:16
`define PKTQ_REMAINING 45:26
`define PKTQ_DBUFFERED 65:46
`define PKTQ_GRANTED   85:66
`define PKTQ_PRIORITY  98:86

`define SENDQ_SIZE      101
`define SENDQ_RPC_ID    15:0
`define SENDQ_DBUFF_ID  25:16
`define SENDQ_OFFSET    57:26
`define SENDQ_DBUFFERED 77:58
`define SENDQ_MSG_LEN   97:78
`define SENDQ_PRIORITY  100:98

`define DBUFF_SIZE   36
`define DBUFF_RPC_ID 15:0
`define DBUFF_OFFSET 35:16

`define GRANT_SIZE   36
`define GRANT_RPC_ID 15:0
`define GRANT_OFFSET 35:16

`define HOMA_PAYLOAD_SIZE 20'h56a
`define CACHE_BLOCK_SIZE 64

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
 * TODO immediate block for short messages is not optimal?
 *  
 * TODO use a LAST bit to handle when notifications are sent?
 */
module srpt_data_pkts #(parameter MAX_RPCS = 64)
   (input ap_clk, ap_rst, ap_ce, ap_start, ap_continue,
    
    input			 sendmsg_in_empty_i,
    output reg			 sendmsg_in_read_en_o,
    input [`SENDMSG_SIZE-1:0]	 sendmsg_in_data_i,
   
    input			 dbuff_in_empty_i,
    output reg			 dbuff_in_read_en_o,
    input [`DBUFF_SIZE-1:0]	 dbuff_in_data_i,
   
    input			 grant_in_empty_i,
    output reg			 grant_in_read_en_o,
    input [`GRANT_SIZE-1:0]	 grant_in_data_i,

    input			 data_pkt_full_i,
    output reg			 data_pkt_write_en_o,
    output reg [`PKTQ_SIZE-1:0]	 data_pkt_data_o,

    input			 cache_req_full_i,
    output reg			 cache_req_write_en_o,
    output reg [`SENDQ_SIZE-1:0] cache_req_data_o,

    output			 ap_idle, ap_done, ap_ready);

   reg [`PKTQ_SIZE-1:0]		 pktq[MAX_RPCS-1:0];
   reg [`PKTQ_SIZE-1:0]		 pktq_swpo[MAX_RPCS-1:0]; 
   reg [`PKTQ_SIZE-1:0]		 pktq_swpe[MAX_RPCS-1:0];

   reg [`SENDQ_SIZE-1:0]	 sendq[MAX_RPCS-1:0];
   reg [`SENDQ_SIZE-1:0]	 sendq_swpo[MAX_RPCS-1:0]; 
   reg [`SENDQ_SIZE-1:0]	 sendq_swpe[MAX_RPCS-1:0];
   
   reg [`PKTQ_SIZE-1:0]		 pktq_ins;
   reg [`SENDQ_SIZE-1:0]	 sendq_ins;

   reg				 pktq_pol;
   reg				 sendq_pol;

   wire [`PKTQ_SIZE-1:0]	 pktq_head;
   wire [`SENDQ_SIZE-1:0]	 sendq_head;

   wire				 pktq_granted;
   wire				 pktq_dbuffered;
   wire				 pktq_ripe;
   wire				 pktq_empty;
   
   task prioritize_pktq;
      output [`PKTQ_SIZE-1:0] low_o, high_o;
      input [`PKTQ_SIZE-1:0]  low_i, high_i;
      begin
	 if ((low_i[`PKTQ_PRIORITY] != high_i[`PKTQ_PRIORITY]) 
	     ? (low_i[`PKTQ_PRIORITY] < high_i[`PKTQ_PRIORITY]) 
	     : (low_i[`PKTQ_REMAINING] > high_i[`PKTQ_REMAINING])) begin
	    high_o = low_i;
	    low_o  = high_i;
	    if (low_i[`PKTQ_RPC_ID] == high_i[`PKTQ_RPC_ID]) begin
	       if (low_i[`PKTQ_PRIORITY] == `SRPT_DBUFF_UPDATE) begin
		  low_o[`PKTQ_DBUFFERED] = low_i[`PKTQ_DBUFFERED];
		  low_o[`PKTQ_PRIORITY]  = `SRPT_ACTIVE;
	       end else if (low_i[`PKTQ_PRIORITY] == `SRPT_GRANT_UPDATE) begin
		  low_o[`PKTQ_GRANTED]   = low_i[`PKTQ_GRANTED];
		  low_o[`PKTQ_PRIORITY]  = `SRPT_ACTIVE;
	       end 
	    end
	 end else begin 
	    high_o = high_i;
	    low_o  = low_i;
	 end 
      end
   endtask 

   task prioritize_sendq;
      output [`SENDQ_SIZE-1:0] low_o, high_o;
      input [`SENDQ_SIZE-1:0]  low_i, high_i;
      begin
	 if ((low_i[`SENDQ_PRIORITY] != high_i[`SENDQ_PRIORITY]) 
	     ? (low_i[`SENDQ_PRIORITY] < high_i[`SENDQ_PRIORITY]) 
	     : (low_i[`SENDQ_DBUFFERED] > high_i[`SENDQ_DBUFFERED])) begin
	    high_o = low_i;
	    low_o  = high_i;
	 end else begin 
	    high_o = high_i;
	    low_o  = low_i;
	 end 
      end
   endtask 

   assign sendq_head     = sendq[0];

   assign pktq_head      = pktq[0];
   assign pktq_granted   = (pktq_head[`PKTQ_GRANTED] + 1 <= pktq_head[`PKTQ_REMAINING])
     | (pktq_head[`PKTQ_GRANTED] == 0);
   assign pktq_dbuffered = (pktq_head[`PKTQ_DBUFFERED] + `HOMA_PAYLOAD_SIZE <= pktq_head[`PKTQ_REMAINING]) 
     | (pktq_head[`PKTQ_DBUFFERED] == 0);
   assign pktq_empty     = pktq_head[`PKTQ_REMAINING] == 0;
   assign pktq_ripe      = pktq_granted & pktq_dbuffered & !pktq_empty;

   integer entry;

   always @* begin
      sendmsg_in_read_en_o = 0;
      dbuff_in_read_en_o   = 0;
      grant_in_read_en_o   = 0;
      data_pkt_write_en_o  = 0;
      cache_req_write_en_o = 0;

      data_pkt_data_o  = pktq_head;
      cache_req_data_o = sendq_head;

      pktq_ins  = 0;
      sendq_ins = 0;

      if (sendmsg_in_empty_i) begin
	 pktq_ins[`PKTQ_RPC_ID]    = sendmsg_in_data_i[`SENDMSG_RPC_ID];
	 pktq_ins[`PKTQ_DBUFF_ID]  = sendmsg_in_data_i[`SENDMSG_DBUFF_ID];
	 pktq_ins[`PKTQ_REMAINING] = sendmsg_in_data_i[`SENDMSG_MSG_LEN];
	 pktq_ins[`PKTQ_DBUFFERED] = sendmsg_in_data_i[`SENDMSG_MSG_LEN];
	 pktq_ins[`PKTQ_GRANTED]   = sendmsg_in_data_i[`SENDMSG_GRANTED];
	 pktq_ins[`PKTQ_PRIORITY]  = `SRPT_ACTIVE;
	 
	 sendq_ins[`SENDQ_RPC_ID]    = sendmsg_in_data_i[`SENDMSG_RPC_ID];
	 sendq_ins[`SENDQ_DBUFF_ID]  = sendmsg_in_data_i[`SENDMSG_DBUFF_ID];
	 sendq_ins[`SENDQ_OFFSET]    = sendmsg_in_data_i[`SENDMSG_DMA_OFFSET]; 
	 sendq_ins[`SENDQ_DBUFFERED] = sendmsg_in_data_i[`SENDMSG_MSG_LEN];
	 sendq_ins[`SENDQ_MSG_LEN]   = sendmsg_in_data_i[`SENDMSG_MSG_LEN];
	 sendq_ins[`SENDQ_PRIORITY]  = `SRPT_ACTIVE;
	 
	 sendmsg_in_read_en_o = 1;
      end else begin

	 if (cache_req_full_i && sendq_head[`SENDQ_PRIORITY] == `SRPT_ACTIVE) begin
	    cache_req_write_en_o = 1;
	 end

	 if (dbuff_in_empty_i) begin
	    pktq_ins[`PKTQ_RPC_ID]    = dbuff_in_data_i[`DBUFF_RPC_ID];
	    pktq_ins[`PKTQ_DBUFFERED] = dbuff_in_data_i[`DBUFF_OFFSET];
	    pktq_ins[`PKTQ_PRIORITY]  = `SRPT_DBUFF_UPDATE;
	    dbuff_in_read_en_o = 1;
	 end else if (grant_in_empty_i) begin
	    pktq_ins[`PKTQ_RPC_ID]   = grant_in_data_i[`GRANT_RPC_ID];
	    pktq_ins[`PKTQ_GRANTED]  = grant_in_data_i[`GRANT_OFFSET];
	    pktq_ins[`PKTQ_PRIORITY] = `SRPT_GRANT_UPDATE;
	    grant_in_read_en_o = 1;
	 end else if (data_pkt_full_i && pktq_head[`PKTQ_PRIORITY] == `SRPT_ACTIVE) begin
	    if (pktq_ripe) begin
	       data_pkt_write_en_o = 1;
	    end
	 end
      end

      prioritize_pktq(pktq_swpo[0], pktq_swpo[1], pktq_ins, pktq[0]);

      // Compute data_swp_odd
      for (entry = 2; entry < MAX_RPCS-1; entry=entry+2) begin
	 prioritize_pktq(pktq_swpo[entry], pktq_swpo[entry+1], pktq[entry-1], pktq[entry]);
      end

      // Compute data_swp_even
      for (entry = 1; entry < MAX_RPCS; entry=entry+2) begin
	 prioritize_pktq(pktq_swpe[entry-1], pktq_swpe[entry], pktq[entry-1], pktq[entry]);
      end

      
      prioritize_sendq(sendq_swpo[0], sendq_swpo[1], sendq_ins, sendq[0]);

      // Compute send_swp_odd
      for (entry = 2; entry < MAX_RPCS-1; entry=entry+2) begin
	 prioritize_sendq(sendq_swpo[entry], sendq_swpo[entry+1], sendq[entry-1], sendq[entry]);
      end

      // Compute send_swp_even
      for (entry = 1; entry < MAX_RPCS; entry=entry+2) begin
	 prioritize_sendq(sendq_swpe[entry-1], sendq_swpe[entry], sendq[entry-1], sendq[entry]);
      end
   end

   integer rst_entry;

   always @(posedge ap_clk) begin
      if (ap_rst) begin
	 pktq_pol <= 0;
	 sendq_pol <= 0;

	 for (rst_entry = 0; rst_entry < MAX_RPCS; rst_entry = rst_entry + 1) begin
	    pktq[rst_entry] <= 0;
	    pktq[rst_entry][`PKTQ_PRIORITY] <= `SRPT_EMPTY;
	    
	    sendq[rst_entry] <= 0;
	    sendq[rst_entry][`SENDQ_PRIORITY] <= `SRPT_EMPTY;
	 end
      end else if (ap_ce && ap_start) begin // if (ap_rst)
	 if (sendmsg_in_empty_i) begin
	    for (entry = 0; entry < MAX_RPCS-1; entry=entry+1) begin
	       sendq[entry] <= sendq_swpo[entry];
	    end 
	 end else if (cache_req_full_i && sendq_head[`SENDQ_PRIORITY] == `SRPT_ACTIVE) begin
	    // Did we just request the last block for this message
	    if (sendq_head[`SENDQ_DBUFFERED] <= `CACHE_BLOCK_SIZE) begin
	       sendq[0]                  <= sendq[1];
	       sendq[1][`SENDQ_PRIORITY] <= `SRPT_INVALIDATE;
	    end else begin
	       sendq[0][`SENDQ_DBUFFERED] <= sendq[0][`SENDQ_DBUFFERED] - `CACHE_BLOCK_SIZE;
	       sendq[0][`SENDQ_OFFSET]    <= sendq[0][`SENDQ_OFFSET] + `CACHE_BLOCK_SIZE;
	    end
	 end else begin
	    if (sendq_pol == 1'b0) begin
	       // Assumes that write does not keep data around
	       for (entry = 0; entry < MAX_RPCS; entry=entry+1) begin
		  sendq[entry] <= sendq_swpe[entry];
	       end

	       sendq_pol <= 1'b1;
	    end else begin
	       for (entry = 1; entry < MAX_RPCS-2; entry=entry+1) begin
		  sendq[entry] <= sendq_swpo[entry+1];
	       end

	       sendq_pol <= 1'b0;
	    end
	 end

	 if (sendmsg_in_empty_i || dbuff_in_empty_i || grant_in_empty_i) begin
	    // Adds either the reactivated message or the update to the queue
	    for (entry = 0; entry < MAX_RPCS-1; entry=entry+1) begin
	       pktq[entry] <= pktq_swpo[entry];
	    end 
	 end else if (data_pkt_full_i && pktq_head[`PKTQ_PRIORITY] == `SRPT_ACTIVE) begin
	    if (pktq_ripe) begin
	       if (pktq_head[`PKTQ_REMAINING] < `HOMA_PAYLOAD_SIZE) begin
		  pktq[0] <= pktq[1];
		  pktq[1][`PKTQ_PRIORITY]  <= `SRPT_INVALIDATE;
	       end else begin
		  pktq[0][`PKTQ_REMAINING] <= pktq_head[`PKTQ_REMAINING] - `HOMA_PAYLOAD_SIZE;
	       end
	    end else if (pktq_empty) begin
	       pktq[0] <= pktq[1];
	       pktq[1][`PKTQ_PRIORITY]  <= `SRPT_INVALIDATE;
	    end else begin
	       pktq[0] <= pktq[1];
	       pktq[1] <= pktq[0];
	       pktq[1][`PKTQ_PRIORITY]  <= `SRPT_BLOCKED;
	    end
	 end else begin
	    if (pktq_pol == 1'b0) begin
	       // Assumes that write does not keep data around
	       for (entry = 0; entry < MAX_RPCS; entry=entry+1) begin
		  pktq[entry] <= pktq_swpe[entry];
	       end

	       pktq_pol <= 1'b1;
	    end else begin
	       for (entry = 1; entry < MAX_RPCS-2; entry=entry+1) begin
		  pktq[entry] <= pktq_swpo[entry+1];
	       end

	       pktq_pol <= 1'b0;
	    end
	 end
      end 
   end 

   assign ap_ready = 1;
   assign ap_idle = 0;
   assign ap_done = 1;

endmodule


/**
 * srpt_data_pkts_tb() - Test bench for srpt data queue
 */
/* verilator lint_off STMTDLY */
module srpt_data_pkts_tb();

   reg ap_clk=0;
   reg ap_rst;
   reg ap_ce;
   reg ap_start;
   reg ap_continue;

   wire	ap_idle;
   wire	ap_done;
   wire	ap_ready;

   reg	sendmsg_in_empty_i;
   wire	sendmsg_in_read_en_o;
   reg [`SENDMSG_SIZE-1:0] sendmsg_in_data_i;

   reg			   grant_in_empty_i;
   wire			   grant_in_read_en_o;
   reg [`GRANT_SIZE-1:0]   grant_in_data_i;

   reg			   dbuff_in_empty_i;
   wire			   dbuff_in_read_en_o;
   reg [`DBUFF_SIZE-1:0]   dbuff_in_data_i;

   reg			   data_pkt_full_i;
   wire			   data_pkt_write_en_o;
   wire [`SENDQ_SIZE-1:0]  data_pkt_data_o;
   
   reg			   cache_req_full_i;
   wire			   cache_req_write_en_o;
   wire [`PKTQ_SIZE-1:0]   cache_req_data_o;

   srpt_data_pkts srpt_queue(.ap_clk(ap_clk), 
			     .ap_rst(ap_rst), 
			     .ap_ce(ap_ce), 
			     .ap_start(ap_start), 
			     .ap_continue(ap_continue), 
			     .sendmsg_in_empty_i(sendmsg_in_empty_i),
			     .sendmsg_in_read_en_o(sendmsg_in_read_en_o),
			     .sendmsg_in_data_i(sendmsg_in_data_i),
			     .grant_in_empty_i(grant_in_empty_i),
			     .grant_in_read_en_o(grant_in_read_en_o),
			     .grant_in_data_i(grant_in_data_i),
			     .dbuff_in_empty_i(dbuff_in_empty_i),
			     .dbuff_in_read_en_o(dbuff_in_read_en_o),
			     .dbuff_in_data_i(dbuff_in_data_i),
			     .data_pkt_full_i(data_pkt_full_i),
			     .data_pkt_write_en_o(data_pkt_write_en_o),
			     .data_pkt_data_o(data_pkt_data_o),
			     .cache_req_full_i(cache_req_full_i),
			     .cache_req_write_en_o(cache_req_write_en_o),
			     .cache_req_data_o(cache_req_data_o),
			     .ap_idle(ap_idle),
			     .ap_done(ap_done), 
			     .ap_ready(ap_ready));

   task sendmsg(input [15:0] rpc_id, input [9:0] dbuff_id, input [19:0] msg_len, dma_offset);
      begin
	 
	 sendmsg_in_data_i[`SENDMSG_RPC_ID]     = rpc_id;
	 sendmsg_in_data_i[`SENDMSG_MSG_LEN]    = msg_len;
	 sendmsg_in_data_i[`SENDMSG_GRANTED]    = msg_len;
	 sendmsg_in_data_i[`SENDMSG_DBUFF_ID]   = dbuff_id;
	 sendmsg_in_data_i[`SENDMSG_DMA_OFFSET] = dma_offset;

	 sendmsg_in_empty_i  = 1;

	 #5;
	 sendmsg_in_empty_i  = 0;
	 wait(sendmsg_in_read_en_o == 1);
      end
      
   endtask

   task dbuff_notif(input [15:0] rpc_id, input [19:0] dbuffered);
      begin
	 dbuff_in_data_i[`DBUFF_RPC_ID]  = rpc_id;
	 dbuff_in_data_i[`DBUFF_OFFSET]  = dbuffered;
	 
	 dbuff_in_empty_i  = 1;
	 
	 wait(dbuff_in_read_en_o == 1);
	 dbuff_in_empty_i  = 0;
      end
   endtask

   task grant_notif(input [15:0] rpc_id, input [19:0] granted);
      begin

	 grant_in_data_i[`GRANT_RPC_ID] = rpc_id;
	 grant_in_data_i[`GRANT_OFFSET] = granted;
	 
	 grant_in_empty_i  = 1;
	 wait(dbuff_in_read_en_o == 1);
	 grant_in_empty_i  = 0;
      end
   endtask

   task get_pkt();
      begin
	 data_pkt_full_i = 1;
	 #5;
	 data_pkt_full_i = 0;
	 wait(data_pkt_write_en_o == 1);
      end
   endtask // get_pkt

   task get_req();
      begin
	 cache_req_full_i = 1;
	 #5;
	 
	 cache_req_full_i = 0;
	 wait(cache_req_write_en_o == 1);
      end
   endtask // get_req

   task test_empty_req();
      begin
	 cache_req_full_i = 1;
	 #5;
	 if(cache_req_write_en_o) begin
	    $display("FAILURE: Data pkts should be empty");
	    $stop;
	 end
	 cache_req_full_i = 0;
      end
   endtask 

   task test_empty_pkt();
      begin
	 data_pkt_full_i = 1;
	 #5;
	 if(data_pkt_write_en_o) begin
	    $display("FAILURE: Data pkts should be empty");
	    $stop;
	 end
	 data_pkt_full_i = 0;
      end
   endtask

   /* verilator lint_off INFINITELOOP */
   
   initial begin
      ap_clk = 0;

      forever begin
	 #2.5 ap_clk = ~ap_clk;
      end 
   end
   
   initial begin
      sendmsg_in_data_i = 0;
      grant_in_data_i   = 0;
      dbuff_in_data_i   = 0;

      sendmsg_in_empty_i  = 0;
      grant_in_empty_i    = 0;
      dbuff_in_empty_i    = 0;
      data_pkt_full_i     = 0;
      cache_req_full_i    = 0;

      // Send reset signal
      ap_ce = 1; 
      ap_rst = 0;
      ap_start = 0;
      ap_rst = 1;

      #5;
      ap_rst = 0;
      ap_start = 1;

      #5;
      /* Add a bunch of user sendmsg requests */
      
      // RPC ID, DBUFF ID, MSG LEN, DMA OFFSET
      sendmsg(1, 1, 5000, 0);
      sendmsg(2, 2, 4000, 4000);
      sendmsg(5, 5, 5000, 5000);
      sendmsg(3, 3, 3000, 3000);

      #30;
      
      // Not dbuff notifs have been provided, so no data pkts out
      test_empty_pkt();
      test_empty_req();

      // Get the req and ensure it comes from the SRPT
      // get_req();

      
      
      
      #1000;
      $stop;
   end

endmodule
