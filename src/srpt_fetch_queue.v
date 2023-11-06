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
 *
 */
module srpt_fetch_queue #(parameter MAX_RPCS = 64)
   (input ap_clk, ap_rst, ap_ce, ap_start, ap_continue,
    
    input			       fetch_in_empty_i,
    output reg			       fetch_in_read_en_o,
    input [`QUEUE_ENTRY_SIZE-1:0]      fetch_in_data_i,
   
    input			       fetch_out_full_i,
    output reg			       fetch_out_write_en_o,
    output reg [`QUEUE_ENTRY_SIZE-1:0] fetch_out_data_o,

    output			       ap_idle, ap_done, ap_ready);

   // Data out priority queue
   reg [`QUEUE_ENTRY_SIZE-1:0]	       fetchq[MAX_RPCS-1:0];
   reg [`QUEUE_ENTRY_SIZE-1:0]	       fetchq_swpo[MAX_RPCS-1:0]; 
   reg [`QUEUE_ENTRY_SIZE-1:0]	       fetchq_swpe[MAX_RPCS-1:0];
   reg [`QUEUE_ENTRY_SIZE-1:0]	       fetchq_insert;

   wire [`QUEUE_ENTRY_SIZE-1:0]	       fetchq_head;
   
   reg				       fetchq_polarity;
   
   task prioritize_fetchq;
      output [`QUEUE_ENTRY_SIZE-1:0] low_o, high_o;
      input [`QUEUE_ENTRY_SIZE-1:0]  low_i, high_i;
      begin
	 if ((low_i[`QUEUE_ENTRY_PRIORITY] != high_i[`QUEUE_ENTRY_PRIORITY]) 
	     ? (low_i[`QUEUE_ENTRY_PRIORITY] < high_i[`QUEUE_ENTRY_PRIORITY]) 
	     : (low_i[`QUEUE_ENTRY_REMAINING] > high_i[`QUEUE_ENTRY_REMAINING])) begin
	    high_o = low_i;
	    low_o  = high_i;
	 end else begin 
	    high_o = high_i;
	    low_o  = low_i;
	 end 
      end
   endtask 
   
   assign fetchq_head = fetchq[0];

   integer entry;

   always @* begin
      fetchq_insert        = 0;
      fetch_in_read_en_o   = 0;
      fetch_out_write_en_o = 0;
      fetch_out_data_o     = 0;
      
      if (fetch_in_empty_i) begin
	 fetch_in_read_en_o   = 1;
	 fetchq_insert        = fetch_in_data_i;
      end else if (fetch_out_full_i && fetchq_head[`QUEUE_ENTRY_PRIORITY] == `SRPT_ACTIVE) begin
	 fetch_out_data_o     = fetchq_head;
	 fetch_out_write_en_o = 1;
      end

      prioritize_fetchq(fetchq_swpo[0], fetchq_swpo[1], fetchq_insert, fetchq[0]);

      // Compute data_swp_odd
      for (entry = 2; entry < MAX_RPCS-1; entry=entry+2) begin
	 prioritize_fetchq(fetchq_swpo[entry], fetchq_swpo[entry+1], fetchq[entry-1], fetchq[entry]);
      end

      // Compute data_swp_even
      for (entry = 1; entry < MAX_RPCS; entry=entry+2) begin
	 prioritize_fetchq(fetchq_swpe[entry-1], fetchq_swpe[entry], fetchq[entry-1], fetchq[entry]);
      end

    end

   integer rst_entry;

   always @(posedge ap_clk) begin
      if (ap_rst) begin
	 fetchq_polarity      <= 0;

	 for (rst_entry = 0; rst_entry < MAX_RPCS; rst_entry = rst_entry + 1) begin
	    fetchq[rst_entry] <= 0;
	    fetchq[rst_entry][`QUEUE_ENTRY_PRIORITY] <= `SRPT_EMPTY;
	 end
      end else if (ap_ce && ap_start) begin // if (ap_rst)
	 if (fetch_in_empty_i) begin
	    for (entry = 0; entry < MAX_RPCS-1; entry=entry+1) begin
	       fetchq[entry] <= fetchq_swpo[entry];
	    end 
	 end else if (fetch_out_full_i && fetchq_head[`QUEUE_ENTRY_PRIORITY] == `SRPT_ACTIVE) begin
	    // Did we just request the last block for this message
	    if (fetchq_head[`QUEUE_ENTRY_REMAINING] <= `CACHE_BLOCK_SIZE) begin
	       $display("Active entry out");
	       fetchq[0]                        <= fetchq[1];
	       fetchq[1][`QUEUE_ENTRY_PRIORITY] <= `SRPT_INVALIDATE;
	    end else begin
	       $display("Dbuffered more");
	       fetchq[0][`QUEUE_ENTRY_REMAINING] <= fetchq[0][`QUEUE_ENTRY_REMAINING] - `CACHE_BLOCK_SIZE;
	       fetchq[0][`QUEUE_ENTRY_DBUFFERED] <= fetchq[0][`QUEUE_ENTRY_DBUFFERED] + `CACHE_BLOCK_SIZE;
	    end
	 end else begin
	    if (fetchq_polarity == 1'b0) begin
	       // Assumes that write does not keep data around
	       for (entry = 0; entry < MAX_RPCS; entry=entry+1) begin
		  fetchq[entry] <= fetchq_swpe[entry];
	       end

	       fetchq_polarity <= 1'b1;
	    end else begin
	       for (entry = 1; entry < MAX_RPCS-2; entry=entry+1) begin
		  fetchq[entry] <= fetchq_swpo[entry+1];
	       end

	       fetchq_polarity <= 1'b0;
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
module srpt_fetch_queue_tb();

   reg ap_clk=0;
   reg ap_rst;
   reg ap_ce;
   reg ap_start;
   reg ap_continue;

   wire	ap_idle;
   wire	ap_done;
   wire	ap_ready;

   reg	fetch_in_empty_i;
   wire	fetch_in_read_en_o;
   reg [`QUEUE_ENTRY_SIZE-1:0] fetch_in_data_i;

   reg			        fetch_out_full_i;
   wire 			fetch_out_write_en_o;
   wire [`QUEUE_ENTRY_SIZE-1:0]	fetch_out_data_o;
   
   srpt_fetch_queue srpt_fetch_queue_tb(.ap_clk(ap_clk), 
					.ap_rst(ap_rst), 
					.ap_ce(ap_ce), 
					.ap_start(ap_start), 
					.ap_continue(ap_continue), 
					.fetch_in_empty_i(fetch_in_empty_i),
					.fetch_in_read_en_o(fetch_in_read_en_o),
					.fetch_in_data_i(fetch_in_data_i),
					.fetch_out_full_i(fetch_out_full_i),
					.fetch_out_write_en_o(fetch_out_write_en_o),
					.fetch_out_data_o(fetch_out_data_o),
					.ap_idle(ap_idle),
					.ap_done(ap_done), 
					.ap_ready(ap_ready));

   task sendmsg(input [15:0] rpc_id, 
		input [9:0]  dbuff_id, 
		input [19:0] initial_grant, 
		input [19:0] initial_dbuff, 
		input [19:0] msg_len);
      
      begin
	 $display("Adding to queue: RPC ID: %d MSG LEN: %d DBUFF ID: %d", rpc_id, msg_len, dbuff_id);
	 
	 fetch_in_data_i[`QUEUE_ENTRY_RPC_ID]    = rpc_id;
	 fetch_in_data_i[`QUEUE_ENTRY_DBUFF_ID]  = dbuff_id;
	 fetch_in_data_i[`QUEUE_ENTRY_REMAINING] = msg_len;
	 fetch_in_data_i[`QUEUE_ENTRY_DBUFFERED] = initial_dbuff;
	 fetch_in_data_i[`QUEUE_ENTRY_GRANTED]   = initial_grant;
	 fetch_in_data_i[`QUEUE_ENTRY_PRIORITY]  = `SRPT_ACTIVE;

	 fetch_in_empty_i  = 1;
	 // TODO should also make sure the reader is ready

	 #5;

	 fetch_in_empty_i  = 0;
	 wait(fetch_in_read_en_o == 1);
      end
      
   endtask // sendmsg
   
   task get_output();
      begin
	 $display("get_output()");
	 // TODO should also make sure the writer is ready
	 fetch_out_full_i = 1;

	 wait(fetch_out_write_en_o == 1);
	 
	 $display("queue output: RPC ID: %d, DBUFF ID: %d, REMAINING: %d, DBUFFERED: %d, GRANTED: %d, PRIORITY: %d", fetch_out_data_o[`QUEUE_ENTRY_RPC_ID], fetch_out_data_o[`QUEUE_ENTRY_DBUFF_ID], fetch_out_data_o[`QUEUE_ENTRY_REMAINING], fetch_out_data_o[`QUEUE_ENTRY_DBUFFERED], fetch_out_data_o[`QUEUE_ENTRY_GRANTED], fetch_out_data_o[`QUEUE_ENTRY_PRIORITY]);
	 
	 #5;

	 fetch_out_full_i = 0;

      end
   endtask // get_pkt

   task test_empty_output();
      begin
	 fetch_out_full_i = 1;
	 #5;
	 if(fetch_out_write_en_o) begin
	    $display("FAILURE: Data pkts should be empty");
	    $stop;
	 end
	 fetch_out_full_i = 0;
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
      fetch_in_data_i  = 0;
      fetch_in_empty_i = 0;
      fetch_out_full_i = 0;

      // Send reset signal
      ap_ce = 1; 
      ap_rst = 0;
      ap_start = 0;
      ap_rst = 1;

      #5;
      ap_rst = 0;
      ap_start = 1;

      #5;
      /* Add a bunch of inactive sendmsg requests */
      
      // RPC ID, DBUFF ID, INIT GRANT, INIT DBUFF, MSG LEN
      // sendmsg(1, 1, 0, 0, 1000);
      // sendmsg(2, 2, 0, 0, 2000);
      // sendmsg(5, 5, 0, 0, 3000);
      // sendmsg(3, 3, 0, 0, 4000);

      sendmsg(7, 7, 0, 0, 512);

      #30;

      // TODO this seems to fail on the first output?
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
