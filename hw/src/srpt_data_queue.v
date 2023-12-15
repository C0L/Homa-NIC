`timescale 1ns / 1ps

`define SRPT_INVALIDATE   3'b000
`define SRPT_DBUFF_UPDATE 3'b001
`define SRPT_GRANT_UPDATE 3'b010
`define SRPT_EMPTY        3'b011
`define SRPT_BLOCKED      3'b100
`define SRPT_ACTIVE       3'b101

`define QUEUE_ENTRY_SIZE      99
`define QUEUE_ENTRY_RPC_ID    15:0  // ID of this transaction
`define QUEUE_ENTRY_DBUFF_ID  24:16 // Corresponding on chip cache
`define QUEUE_ENTRY_REMAINING 45:26 // Remaining to be sent or cached
`define QUEUE_ENTRY_DBUFFERED 65:46 // Number of bytes cached
`define QUEUE_ENTRY_GRANTED   85:66 // Number of bytes granted
`define QUEUE_ENTRY_PRIORITY  88:86 // Deprioritize inactive messages

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
 * TODO immediate block for short messages is not optimal? This can be buffered from another core?
 *  
 * TODO use a LAST bit to handle when notifications are sent?
 */
module srpt_data_queue #(parameter MAX_RPCS = 64)
   (input ap_clk, ap_rst, 
    
    input				S_AXIS_TVALID,
    output reg				S_AXIS_TREADY,
    input [`QUEUE_ENTRY_SIZE-1:0]	S_AXIS_TDATA,

    output reg				M_AXIS_TVALID,
    input wire				M_AXIS_TREADY,
    output reg [C_S_AXI_DATA_WIDTH-1:0]	M_AXIS_TDATA,

    // input			       data_pkt_full_i,
    // output reg			       data_pkt_write_en_o,
    // output reg [`QUEUE_ENTRY_SIZE-1:0] data_pkt_data_o,

    output				ap_idle, ap_done, ap_ready);

   // Data out priority queue
   reg [`QUEUE_ENTRY_SIZE-1:0]	       sendq[MAX_RPCS-1:0];
   reg [`QUEUE_ENTRY_SIZE-1:0]	       sendq_swpo[MAX_RPCS-1:0]; 
   reg [`QUEUE_ENTRY_SIZE-1:0]	       sendq_swpe[MAX_RPCS-1:0];
   reg [`QUEUE_ENTRY_SIZE-1:0]	       sendq_insert;

   wire [`QUEUE_ENTRY_SIZE-1:0]	       sendq_head;
   
   
   wire				       sendq_granted;
   wire				       sendq_dbuffered;
   wire				       sendq_ripe;
   wire				       sendq_empty;

   reg				       sendq_polarity;
   
   task prioritize_sendq;
      output [`QUEUE_ENTRY_SIZE-1:0] low_o, high_o;
      input [`QUEUE_ENTRY_SIZE-1:0]  low_i, high_i;
      begin
	 if ((low_i[`QUEUE_ENTRY_PRIORITY] != high_i[`QUEUE_ENTRY_PRIORITY]) 
	     ? (low_i[`QUEUE_ENTRY_PRIORITY] < high_i[`QUEUE_ENTRY_PRIORITY]) 
	     : (low_i[`QUEUE_ENTRY_REMAINING] > high_i[`QUEUE_ENTRY_REMAINING])) begin
	    high_o = low_i;
	    low_o  = high_i;
	    if (low_i[`QUEUE_ENTRY_RPC_ID] == high_i[`QUEUE_ENTRY_RPC_ID]) begin
	       if (low_i[`QUEUE_ENTRY_PRIORITY] == `SRPT_DBUFF_UPDATE) begin
		  low_o[`QUEUE_ENTRY_DBUFFERED] = low_i[`QUEUE_ENTRY_DBUFFERED];
		  low_o[`QUEUE_ENTRY_PRIORITY]  = `SRPT_ACTIVE;
	       end else if (low_i[`QUEUE_ENTRY_PRIORITY] == `SRPT_GRANT_UPDATE) begin
		  low_o[`QUEUE_ENTRY_GRANTED]   = low_i[`QUEUE_ENTRY_GRANTED];
		  low_o[`QUEUE_ENTRY_PRIORITY]  = `SRPT_ACTIVE;
	       end 
	    end
	 end else begin 
	    high_o = high_i;
	    low_o  = low_i;
	 end 
      end
   endtask // prioritize_sendq
   
   assign sendq_head      = sendq[0];
   assign sendq_granted   = (sendq_head[`QUEUE_ENTRY_GRANTED] + 1 <= sendq_head[`QUEUE_ENTRY_REMAINING])
     | (sendq_head[`QUEUE_ENTRY_GRANTED] == 0);
   
   assign sendq_dbuffered = (sendq_head[`QUEUE_ENTRY_DBUFFERED] + `HOMA_PAYLOAD_SIZE <= sendq_head[`QUEUE_ENTRY_REMAINING]) 
     | (sendq_head[`QUEUE_ENTRY_DBUFFERED] == 0);
   assign sendq_empty     = sendq_head[`QUEUE_ENTRY_REMAINING] == 0;
   assign sendq_ripe      = sendq_granted & sendq_dbuffered & !sendq_empty;

   integer entry;

   always @* begin
      S_AXIS_TREADY_SENDMSG = 0;
      sendq_insert         = 0;
      M_AXIS_TVALID        = 0;
      M_AXIS_TDATA         = 0;

      if (S_AXIS_TVALID) begin
	 sendq_insert  = S_AXIS_TDATA;
	 S_AXIS_TREADY = 1;
      end if (M_AXIS_TREADY && sendq_head[`QUEUE_ENTRY_PRIORITY] == `SRPT_ACTIVE) begin // else: !if(S_AXIS_TVALID_SENDMSG)
	 if (sendq_ripe) begin
	    M_AXIS_TDATA  = sendq_head;
	    M_AXIS_TVALID = 1; 
	 end
      end

      prioritize_sendq(sendq_swpo[0], sendq_swpo[1], sendq_insert, sendq[0]);

      // Compute data_swp_odd
      for (entry = 2; entry < MAX_RPCS-1; entry=entry+2) begin
	 prioritize_sendq(sendq_swpo[entry], sendq_swpo[entry+1], sendq[entry-1], sendq[entry]);
      end

      // Compute data_swp_even
      for (entry = 1; entry < MAX_RPCS; entry=entry+2) begin
	 prioritize_sendq(sendq_swpe[entry-1], sendq_swpe[entry], sendq[entry-1], sendq[entry]);
      end
   end

   integer rst_entry;

   always @(posedge ap_clk) begin
      if (ap_rst) begin
	 sendq_polarity      <= 0;
	 for (rst_entry = 0; rst_entry < MAX_RPCS; rst_entry = rst_entry + 1) begin
	    sendq[rst_entry] <= 0;
	    sendq[rst_entry][`QUEUE_ENTRY_PRIORITY] <= `SRPT_EMPTY;
	 end
      end else begin // if (ap_rst)
	 if (S_AXIS_TVALID) begin
	    // Adds either the reactivated message or the update to the queue
	    for (entry = 0; entry < MAX_RPCS-1; entry=entry+1) begin
	       sendq[entry] <= sendq_swpo[entry];
	    end 
	 end else if (data_pkt_full_i && sendq_head[`QUEUE_ENTRY_PRIORITY] == `SRPT_ACTIVE) begin
	    $display("sendq_granted: %d", sendq_granted);
	    $display("sendq_dbuffered: %d", sendq_dbuffered);
	    $display("sendq_empty: %d", sendq_empty);
	    $display("sendq_ripe: %d", sendq_ripe);

	    if (sendq_ripe) begin
	       $display("RIPE ENTRY %d", sendq_head[`QUEUE_ENTRY_REMAINING]);
	       if (sendq_head[`QUEUE_ENTRY_REMAINING] <= `HOMA_PAYLOAD_SIZE) begin
		  $display("INVALIDATING HEAD\n");
		  sendq[0] <= sendq[1];
		  sendq[1][`QUEUE_ENTRY_PRIORITY]  <= `SRPT_INVALIDATE;
	       end else begin
		  $display("DECREMENETING HEAD\n");
		  sendq[0][`QUEUE_ENTRY_REMAINING] <= sendq_head[`QUEUE_ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE;
	       end
	    end else if (sendq_empty) begin
	       sendq[0] <= sendq[1];
	       sendq[1][`QUEUE_ENTRY_PRIORITY]  <= `SRPT_INVALIDATE;
	    end else begin
	       sendq[0] <= sendq[1];
	       sendq[1] <= sendq[0];
	       sendq[1][`QUEUE_ENTRY_PRIORITY]  <= `SRPT_BLOCKED;
	    end
	 end else begin
	    if (sendq_polarity == 1'b0) begin
	       // Assumes that write does not keep data around
	       for (entry = 0; entry < MAX_RPCS; entry=entry+1) begin
		  sendq[entry] <= sendq_swpe[entry];
	       end

	       sendq_polarity <= 1'b1;
	    end else begin
	       for (entry = 1; entry < MAX_RPCS-2; entry=entry+1) begin
		  sendq[entry] <= sendq_swpo[entry+1];
	       end

	       sendq_polarity <= 1'b0;
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
module srpt_data_queue_tb();

   reg ap_clk=0;
   reg ap_rst;

   wire	ap_idle;
   wire	ap_done;
   wire	ap_ready;

   reg	S_AXIS_TVALID;
   wire	S_AXIS_TREADY;
   reg [`QUEUE_ENTRY_SIZE-1:0] S_AXIS_TDATA;

   reg			       M_AXIS_TREADY;
   wire			       M_AXIS_TVALID;
   wire [`QUEUE_ENTRY_SIZE-1:0]	M_AXIS_TDATA;
   
   srpt_data_queue srpt_data_queue_tb(.ap_clk(ap_clk), 
				      .ap_rst(ap_rst), 
				      .S_AXIS_TVALID(S_AXIS_TVALID),
				      .S_AXIS_TREADY(S_AXIS_TREADY),
				      .S_AXIS_TDATA(S_AXIS_TDATA),
				      .M_AXIS_TVALID(M_AXIS_TVALID),
				      .M_AXIS_TREADY(M_AXIS_TREADY),
				      .M_AXIS_TDATA(M_AXIS_TDATA));
   

   task sendmsg(input [15:0] rpc_id, 
		input [9:0]  dbuff_id, 
		input [19:0] initial_grant, 
		input [19:0] initial_dbuff, 
		input [19:0] msg_len);
      
      begin
	 $display("Adding to queue: RPC ID: %d MSG LEN: %d DBUFF ID: %d", rpc_id, msg_len, dbuff_id);
	 
	 S_AXIS_TDATA[`QUEUE_ENTRY_RPC_ID]    = rpc_id;
	 S_AXIS_TDATA[`QUEUE_ENTRY_DBUFF_ID]  = dbuff_id;
	 S_AXIS_TDATA[`QUEUE_ENTRY_REMAINING] = msg_len;
	 S_AXIS_TDATA[`QUEUE_ENTRY_DBUFFERED] = initial_dbuff;
	 S_AXIS_TDATA[`QUEUE_ENTRY_GRANTED]   = initial_grant;
	 S_AXIS_TDATA[`QUEUE_ENTRY_PRIORITY]  = `SRPT_ACTIVE;

	 S_AXIS_TVALID = 1;
	 // TODO should also make sure the reader is ready

	 #5;

	 S_AXIS_TVALID = 0;
	 wait(S_AXIS_TREADY == 1);
      end
      
   endtask

   task dbuff_notif(input [15:0] rpc_id, input [19:0] dbuffered);
      begin
	 
	 $display("dbuff notif: RPC ID: %d DBUFFERED: %d", rpc_id, dbuffered);
	 M_AXIS_TDATA[`QUEUE_ENTRY_RPC_ID]    = rpc_id;
	 M_AXIS_TDATA[`QUEUE_ENTRY_DBUFFERED] = dbuffered;
	 M_AXIS_TDATA[`QUEUE_ENTRY_PRIORITY]  = `SRPT_DBUFF_UPDATE;
	 
	 M_AXIS_TREADY = 1;

	 #5;
	 
	 M_AXIS_TREADY = 0;
	 wait(M_AXIS_TVALID == 1);
      end
   endtask

   task grant_notif(input [15:0] rpc_id, input [19:0] granted);
      begin

	 $display("grant notif: RPC ID: %d GRANTED: %d", rpc_id, granted);
	 M_AXIS_TDATA[`QUEUE_ENTRY_RPC_ID]   = rpc_id;
	 M_AXIS_TDATA[`QUEUE_ENTRY_GRANTED]  = granted;
	 M_AXIS_TDATA[`QUEUE_ENTRY_PRIORITY] = `SRPT_GRANT_UPDATE;
	 
	 M_AXIS_TREADY = 1;
	 
	 #5;
	    
	 M_AXIS_TREADY = 0;
	 wait(M_AXIS_TVALID == 1);
      end
   endtask

   task get_output();
      begin
	 $display("get_output()");
	 // TODO should also make sure the writer is ready
	 S_AXIS_TVALID = 1;
	 
	 #5;
	 S_AXIS_TVALID = 0;
	 
	 $display("queue output: RPC ID: %d, DBUFF ID: %d, REMAINING: %d, DBUFFERED: %d, GRANTED: %d, PRIORITY: %d", S_AXIS_TDATA[`QUEUE_ENTRY_RPC_ID], S_AXIS_TDATA[`QUEUE_ENTRY_DBUFF_ID], S_AXIS_TDATA[`QUEUE_ENTRY_REMAINING], S_AXIS_TDATA[`QUEUE_ENTRY_DBUFFERED], S_AXIS_TDATA[`QUEUE_ENTRY_GRANTED], S_AXIS_TDATA[`QUEUE_ENTRY_PRIORITY]);
	 wait(S_AXIS_TREADY == 1);
      end
   endtask // get_pkt
/*
   task test_empty_output();
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
*/
   /* verilator lint_off INFINITELOOP */
   
   initial begin
      ap_clk = 0;

      forever begin
	 #2.5 ap_clk = ~ap_clk;
      end 
   end
   
   initial begin
      S_AXIS_TDATA  = 0;
      M_AXIS_TVALID = 0;
      S_AXIS_TDATA  = 0;

      // Send reset signal
      ap_rst = 1;

      #5;
      ap_rst = 0;

      #5;
      
      // RPC ID, DBUFF ID, INIT GRANT, INIT DBUFF, MSG LEN, INITIAL STATE
      sendmsg(1, 1, 1000, 1000, 1000);
      sendmsg(2, 2, 4000, 4000, 4000);
      sendmsg(5, 5, 5000, 5000, 5000);
      sendmsg(3, 3, 3000, 3000, 3000);
      
      sendmsg(7, 7, 0, 512, 512);

      #100;
      
      // data_pkt_full_i = 1;
      #200;
      // data_pkt_full_i = 0;

      #30;

      sendmsg(6, 6, 0, 0, 6000);

      get_output();
      get_output();
      get_output();
      get_output();
      get_output();
      
      #30;
      grant_notif(3, 0);
      //dbuff_notif(3, 2700);
      //dbuff_notif(3, 2500);
      //dbuff_notif(3, 2200);
      //dbuff_notif(3, 1800);
      //dbuff_notif(3, 1500);
      //dbuff_notif(3, 1300);
      //dbuff_notif(3, 500);
      //dbuff_notif(3, 0);

      dbuff_notif(3, 2700);
      dbuff_notif(3, 2500);
      dbuff_notif(3, 2200);
      dbuff_notif(3, 1800);
      dbuff_notif(3, 1500);
      dbuff_notif(3, 1300);
      dbuff_notif(3, 500);
      dbuff_notif(3, 0);



      #30;

            
      get_output();
      get_output();
      get_output();
       
      get_output();
      get_output();
      get_output();
       
      get_output();
      get_output();
      get_output();
       
      get_output();
      get_output();
      get_output();
 
      
      get_output();
      get_output();
      get_output();
      
      #1000;
      $stop;
   end

endmodule
