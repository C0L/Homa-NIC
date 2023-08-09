`timescale 1ns / 1ps

`define SRPT_UPDATE 3'b000
`define SRPT_BLOCK 3'b001
`define SRPT_INVALIDATE 3'b010
`define SRPT_UNBLOCK 3'b011
`define SRPT_EMPTY 3'b100
`define SRPT_BLOCKED 3'b101
`define SRPT_ACTIVE 3'b110

`define ENTRY_SIZE 97
`define PEER_ID 13:0
`define RPC_ID 29:14
`define RECV_BYTES 61:30
`define GRANTABLE_BYTES 93:62
`define PRIORITY 96:94
`define PRIORITY_SIZE 3

`define HEADER_SIZE 126
`define HDR_PEER_ID 13:0
`define HDR_RPC_ID 29:14
`define HDR_OFFSET 61:30
`define HDR_MSG_LEN 93:62
`define HDR_INCOMING 125:94

// 60K byte (RTT_Bytes) * 8 (MAX_OVERCOMMIT)
`define RTT_BYTES 60000
`define OVERCOMMIT_BYTES 480000
`define HOMA_PAYLOAD_SIZE 32'h56a

/**
 * srpt_grant_pkts() - Determines the next RPC that should receive a
 * grant packet. Selections are based on the number of grantable bytes,
 * where less bytes is more desirable.
 *
 * #MAX_OVERCOMMIT - The number of simultanous peers that can be actively granted to
 * 
 * #MAX_OVERCOMMIT_LOG2 - Number of bits needed to index an array of size MAX_OVERCOMMIT
 * 
 * #MAX_SRPT - The maximum number of elements that could possibly receive grants
 * 
 * @header_in_empty_i - 0 value indicates that there are no new
 * headers we need to incorporate into the queue
 * @header_in_read_en_o - Acknowledgement that the input header data has been read
 * @header_in_data_i    - The data content of the header
 *
 * @data_pkt_full_o     - 1 value indicates that the outgoing stream has room
 * for another data packet that should be sent while a 0 value indicates that
 * the stream is full
 * @data_pkt_write_en_o - Acknowledgement that the data in data_pkt_data_o
 * should be read by the outgoing stream
 * @data_pkt_data_o     - The actual outgoing pkts that should be sent onto
 * the link.
 *
 * There are 3 operations within the grant SRPT queue:
 *     1) There is header data available on the input stream. The new
 *     header is checked to determine if it is the first header in an RPC
 *     that we have received. If it is, we construct a new entry in the
 *     queue which will be inserted at slot MAX_OVERCOMMIT. This entry
 *     takes the PEER_ID, RPC_ID from the read header. The initial
 *     RECV_BYTES is set to HOMA_PAYLOAD_SIZE, because this is the first
 *     header for an RPC that we have received. The initial
 *     GRANTABLE_BYTES is set to the size of the message minus
 *     HOMA_PAYLOAD_SIZE (indicating we have a whole message of grantable
 *     bytes besides this first packet that we have received. We then
 *     determine if the entry should begin as active or inactive. We check
 *     to see if peer_match_en is high, which indicates that the PEER_ID
 *     of the new header matches on some PEER_ID in the first
 *     MAX_OVERCOMMIT elements of the srpt_queue. If peer_match_en is high,
 *     we compare the number of grantable bytes of the new header with
 *     that of the entry that matched on peer. If the new header is a more
 *     desirable option in that it has less grantable bytes, then we will
 *     disable the matched entry in the queue, and set the new header
 *     entry to be ACTIVE. If the new header was worse in grantable bytes
 *     than the current entry, then we insert the header in a blocked
 *     state. If peer_match_en was not set, then we can just insert
 *     the new header as active.
 * 
 *     If this header was not the first packet that we have received for 
 *     an RPC then we need to determine where to send an update on the
 *     number of RECV_BYTES and the GRANTABLE_BYTES. This coherency is
 *     required so that the most desirable RPC in terms of grantable
 *     bytes changes in response to the arrival of new packets. There
 *     are two cases: 1) the RPC corresponding to the incoming packet
 *     is in the MAX_OVERCOMMIT elements, as indicated by the
 *     rpc_match_en flag, 2) the RPC corresponding to the incoming
 *     packet is not in MAX_OVERCOMMIT elements. If (1) is true then
 *     we will just update the entry in place. If (2) is true then we
 *     will submit an update message at the MAX_OVERCOMMIT entry.
 * 
 *     2) If there is not a new header then we check if we can send
 *     out a new grant packet. The eligibility to send a packet is
 *     determined by the grant_pkt_full_i signal being high which
 *     indicates that there is room on the output stream and
 *     ready_match_en being high which indicates there is an entry in
 *     the first MAX_OVERCOMMIT elements that is ready to be sent (is
 *     ACTIVE and has non-zero RECV_BYTES). We are limited in the
 *     number of grants we can send by the number MAX_OVERCOMMIT
 *     bytes. We track the number of "free bytes" in the avail_pkts
 *     variable. If we go to send a grant that has more RECV_BYTES than
 *     avail_pkts, then we will only send a grant for avail_pkts number
 *     of bytes. If not, we can send up to RECV_BYTES. If we are
 *     limited by the avail packets, then we are going to decrement
 *     RECV_BYTES and GRANTABLE_BYTES by the avail_pkts, set avail_pkts
 *     to 0, and construct an output on the output stream with the
 *     PEER_ID, RPC_ID, from the entry and set RECV_BYTES to 0. We use
 *     the GRANTABLE_BYTES on the output to communicate how many bytes
 *     this grant is for which depends on if we are limited by
 *     avail_pkts or not. If we are not limited by avail_bytes, then we
 *     also must check to see if the GRANTABLE_BYTES - RECV_BYTES == 0
 *     which indicates the message has been fully granted and we can
 *     set it to EMPTY and forget it. If it is not empty then we update
 *     the entry and avail_pkts.
 *      
 *     3) If there is not a new header, and we are not eligible to
 *     send a grant, we will check to see if a new entry needs to be moved
 *     from the upper part of the queue into the lower MAX_OVERCOMMIT set,
 *     and thus block on the PEER_ID of the moved entry. If new_active is
 *     set, then we will swap in the new entry, and submit a BLOCK message
 *     to the upper queue so that entries with the same PEER_ID become
 *     blocked.
 * 
 * TODO this needs to handle OOO packets (packetmap?)
 * TODO this needs to consider duplicate packets (packetmap?)
 * TODO rtt bytes is hardcoded into this design
 */
module srpt_grant_pkts #(parameter MAX_OVERCOMMIT = 8,
			 parameter MAX_OVERCOMMIT_LOG2 = 3,
			 parameter MAX_SRPT = 1024)
   (input ap_clk, ap_rst, ap_ce, ap_start, ap_continue,
    input			 header_in_empty_i,
    output reg			 header_in_read_en_o,
    input [`HEADER_SIZE-1:0]	 header_in_data_i,
    input			 grant_pkt_full_i,
    output reg			 grant_pkt_write_en_o,
    output reg [`ENTRY_SIZE-1:0] grant_pkt_data_o, 
    output			 ap_idle, ap_done, ap_ready);

   reg [`ENTRY_SIZE-1:0]	 srpt_queue[MAX_SRPT-1:0];
   wire [`ENTRY_SIZE-1:0]	 srpt_active[MAX_OVERCOMMIT-1:0];

   reg [`ENTRY_SIZE-1:0]	 srpt_odd[MAX_SRPT-1:0]; 
   reg [`ENTRY_SIZE-1:0]	 srpt_even[MAX_SRPT-1:0]; 

   reg				 swap_type;

   reg [MAX_OVERCOMMIT_LOG2-1:0] peer_match; 
   reg [MAX_OVERCOMMIT_LOG2-1:0] rpc_match;   
   reg [MAX_OVERCOMMIT_LOG2-1:0] ready_match;  

   reg				 peer_match_en;
   reg				 rpc_match_en;
   reg				 ready_match_en;

   reg [31:0]			 avail_pkts;

   reg [1:0]			 state;
   
   reg				 new_header;
   reg				 new_grant;
   reg				 new_active;
   

   assign ap_ready = 1;
   assign ap_idle = 0;
   assign ap_done = 1;
   
   task prioritize;
      output [`ENTRY_SIZE-1:0] low_o, high_o;
      input [`ENTRY_SIZE-1:0]  low_i, high_i;
      begin
	 // If the priority differs or grantable bytes
	 if ((low_i[`PRIORITY] != high_i[`PRIORITY]) 
	     ? (low_i[`PRIORITY] < high_i[`PRIORITY]) 
	     : (low_i[`GRANTABLE_BYTES] > high_i[`GRANTABLE_BYTES])) begin
	    high_o = low_i;
	    low_o  = high_i;
	    if (low_i[`PEER_ID] == high_i[`PEER_ID]) begin
	       if (low_i[`PRIORITY] == `SRPT_BLOCK) begin
		  low_o[`PRIORITY]  = `SRPT_BLOCKED;
	       end else if (low_i[`PRIORITY] == `SRPT_UNBLOCK) begin
		  low_o[`PRIORITY]  = `SRPT_ACTIVE;
	       end else if (low_i[`RPC_ID] == high_i[`RPC_ID]
			    && low_i[`PRIORITY] == `SRPT_UPDATE) begin
		  low_o[`GRANTABLE_BYTES] = low_i[`GRANTABLE_BYTES] - `HOMA_PAYLOAD_SIZE;
		  low_o[`RECV_BYTES]      = low_i[`RECV_BYTES] + `HOMA_PAYLOAD_SIZE;
	       end 
	    end
	 end else begin 
	    high_o = high_i;
	    low_o  = low_i;
	 end 
      end
   endtask 

   integer entry;

   always @* begin

      grant_pkt_data_o     = 0;
      grant_pkt_write_en_o = 0;
      header_in_read_en_o  = 0;

      // Init all values so that no latches are inferred. All control paths assign a value.
      peer_match     = {MAX_OVERCOMMIT_LOG2{1'b1}};
      rpc_match      = {MAX_OVERCOMMIT_LOG2{1'b1}};
      ready_match    = {MAX_OVERCOMMIT_LOG2{1'b1}};
      
      peer_match_en  = 1'b0;
      rpc_match_en   = 1'b0;
      ready_match_en = 1'b0;

      new_header = header_in_empty_i;

      new_active = ((srpt_queue[MAX_OVERCOMMIT-1][`PRIORITY] != srpt_queue[MAX_OVERCOMMIT][`PRIORITY]) ?
		    (srpt_queue[MAX_OVERCOMMIT-1][`PRIORITY] < srpt_queue[MAX_OVERCOMMIT][`PRIORITY]) :
		    (srpt_queue[MAX_OVERCOMMIT-1][`GRANTABLE_BYTES] > srpt_queue[MAX_OVERCOMMIT][`GRANTABLE_BYTES])) && 
		   (srpt_queue[MAX_OVERCOMMIT][`PRIORITY] == `SRPT_ACTIVE);
      
     
      // Check every entry in the active set
      for (entry = 0; entry < MAX_OVERCOMMIT; entry=entry+1) begin
	 // Does the new header have the same peer as the entry in the active set
	 if (header_in_data_i[`HDR_PEER_ID] == srpt_queue[entry][`PEER_ID] && peer_match_en == 1'b0) begin
	    peer_match = entry[MAX_OVERCOMMIT_LOG2-1:0];
	    peer_match_en = 1'b1;
	 end else begin
	    peer_match = peer_match;
	 end 

	 // Does the new header have the same RPC ID as the entry in the active set
	 if (header_in_data_i[`HDR_RPC_ID] == srpt_queue[entry][`RPC_ID] && rpc_match_en == 1'b0) begin
	    rpc_match = entry[MAX_OVERCOMMIT_LOG2-1:0];
	    rpc_match_en = 1'b1;
	 end else begin
	    rpc_match = rpc_match;
	 end

	 // Is the entry in the active set empty, and so, should it be filled and a GRANT packet sent?
	 if (srpt_queue[entry][`PRIORITY] == `SRPT_ACTIVE 
	     && srpt_queue[entry][`RECV_BYTES] != 0 
	     && ready_match_en == 1'b0) begin
// && avail_pkts != 0) begin
	    ready_match = entry[MAX_OVERCOMMIT_LOG2-1:0];
	    ready_match_en = 1'b1;
	 end else begin
	    ready_match = ready_match;
	 end
      end // for (entry = 0; entry < MAX_OVERCOMMIT; entry=entry+1)

      new_grant = grant_pkt_full_i && ready_match_en && avail_pkts != 0;
      
      /* verilator lint_off WIDTH */
      if (new_header) begin
	 header_in_read_en_o  = 1;
	 grant_pkt_write_en_o = 0;
      end else if (new_grant) begin // if (new_header)
	 grant_pkt_data_o[`PEER_ID]    = srpt_queue[ready_match][`PEER_ID];
	 grant_pkt_data_o[`RPC_ID]     = srpt_queue[ready_match][`RPC_ID];
	 grant_pkt_data_o[`RECV_BYTES] = 32'b0;
	 
	 // Do we want to send more packets than 8 * MAX_OVERCOMMIT PKTS?
	 if (srpt_queue[ready_match][`RECV_BYTES] > avail_pkts) begin
	    // Just send avail_pkts of data
	    grant_pkt_data_o[`GRANTABLE_BYTES] = srpt_queue[ready_match][`GRANTABLE_BYTES] - avail_pkts;
	 end else begin 
	    grant_pkt_data_o[`GRANTABLE_BYTES] = srpt_queue[ready_match][`GRANTABLE_BYTES] - srpt_queue[ready_match][`RECV_BYTES];
	 end 
	 
	 header_in_read_en_o = 0;
	 grant_pkt_write_en_o = 1;
      end // if (new_grant)
      /* verilator lint_on WIDTH */

      srpt_odd[0] = srpt_queue[0];
      srpt_odd[MAX_SRPT-1] = srpt_queue[MAX_SRPT-1];
      
      for (entry = 2; entry < MAX_SRPT-1; entry=entry+2) begin
	 prioritize(srpt_odd[entry-1], srpt_odd[entry], srpt_queue[entry-1], srpt_queue[entry]);
      end
      
      for (entry = 1; entry < MAX_SRPT; entry=entry+2) begin
	 prioritize(srpt_even[entry-1], srpt_even[entry], srpt_queue[entry-1], srpt_queue[entry]);
      end
   end

   integer rst_entry;

   always @(posedge ap_clk) begin

      if (ap_rst) begin
	 swap_type <= 0;

	 avail_pkts <= `OVERCOMMIT_BYTES;

	 for (rst_entry = 0; rst_entry < MAX_SRPT; rst_entry=rst_entry+1) begin
	    srpt_queue[rst_entry][`PEER_ID]         <= 0;
	    srpt_queue[rst_entry][`RPC_ID]          <= 0;
	    srpt_queue[rst_entry][`RECV_BYTES]      <= 0;
	    srpt_queue[rst_entry][`GRANTABLE_BYTES] <= 0;
	    srpt_queue[rst_entry][`PRIORITY]        <= `SRPT_EMPTY;
	 end
      end else if (ap_ce && ap_start) begin
	 if (new_header) begin

	    // Make room for that new entry 
	    for (entry = MAX_OVERCOMMIT+1; entry < MAX_SRPT-1; entry=entry+1) begin
	       srpt_queue[entry] <= srpt_queue[entry-1];
	    end

	    // TODO cannot base off of the first unscheduled packet
	    // It is the first unscheduled packet that creates the entry in the SRPT queue.
	    if (header_in_data_i[`HDR_OFFSET] == 0) begin
	       
	       /* verilator lint_on WIDTH */
	       srpt_queue[MAX_OVERCOMMIT][`PEER_ID]         <= header_in_data_i[`HDR_PEER_ID];
	       srpt_queue[MAX_OVERCOMMIT][`RPC_ID]          <= header_in_data_i[`HDR_RPC_ID];
	       srpt_queue[MAX_OVERCOMMIT][`RECV_BYTES]      <= `HOMA_PAYLOAD_SIZE;
	       srpt_queue[MAX_OVERCOMMIT][`GRANTABLE_BYTES] <= header_in_data_i[`HDR_MSG_LEN] - `HOMA_PAYLOAD_SIZE;
	       
	       // Is the new header's peer one of the active entries?
	       if (peer_match_en) begin
		  /* verilator lint_off WIDTH */
		  if ((header_in_data_i[`HDR_MSG_LEN] -`HOMA_PAYLOAD_SIZE) < srpt_queue[peer_match][`GRANTABLE_BYTES]) begin
		     srpt_queue[peer_match][`PRIORITY]     <= `SRPT_BLOCKED;
		     srpt_queue[MAX_OVERCOMMIT][`PRIORITY] <= `SRPT_ACTIVE;

		  end else begin
		     // Insert the new RPC into the big queue
		     srpt_queue[MAX_OVERCOMMIT][`PRIORITY] <= `SRPT_BLOCKED;
		  end
		  
		  /* verilator lint_on WIDTH */
	       end else begin 
		  avail_pkts <= avail_pkts + `HOMA_PAYLOAD_SIZE;
		  // Insert the new RPC into the big queue
		  srpt_queue[MAX_OVERCOMMIT][`PRIORITY] <= `SRPT_ACTIVE;
	       end
	    end else begin // if (header_in_data_i[`HDR_OFFSET] == 0)
	       /* verilator lint_off WIDTH */
	       if (rpc_match_en) begin
		  srpt_queue[rpc_match][`RECV_BYTES]      <= srpt_queue[rpc_match][`RECV_BYTES] + `HOMA_PAYLOAD_SIZE;
		  srpt_queue[rpc_match][`GRANTABLE_BYTES] <= srpt_queue[rpc_match][`GRANTABLE_BYTES] - `HOMA_PAYLOAD_SIZE;
	       end else begin
		  srpt_queue[MAX_OVERCOMMIT][`PEER_ID]         <= header_in_data_i[`HDR_PEER_ID];
		  srpt_queue[MAX_OVERCOMMIT][`RPC_ID]          <= header_in_data_i[`HDR_RPC_ID];
		  srpt_queue[MAX_OVERCOMMIT][`RECV_BYTES]      <= 0;
		  srpt_queue[MAX_OVERCOMMIT][`GRANTABLE_BYTES] <= 0;
		  srpt_queue[MAX_OVERCOMMIT][`PRIORITY]        <= `SRPT_UPDATE;
	       end
	       /* verilator lint_on WIDTH */
	    end
	 end else if (new_grant) begin // if (header_in_empty_i)
	    /* verilator lint_off WIDTH */
	    // Do we want to send more packets than 8 * MAX_OVERCOMMIT PKTS?
	    if (srpt_queue[ready_match][`RECV_BYTES] > avail_pkts) begin
	       srpt_queue[ready_match][`RECV_BYTES]      <= srpt_queue[ready_match][`RECV_BYTES] - avail_pkts;
	       srpt_queue[ready_match][`GRANTABLE_BYTES] <= srpt_queue[ready_match][`GRANTABLE_BYTES] - avail_pkts;

	       avail_pkts <= 0;  
	    end else begin 
	       // Is this going to result in a fully granted message?
	       if ((srpt_queue[ready_match][`GRANTABLE_BYTES] - srpt_queue[ready_match][`RECV_BYTES]) == 0) begin
		  srpt_queue[ready_match][`PRIORITY] <= `SRPT_EMPTY;
	       end

	       srpt_queue[ready_match][`RECV_BYTES]      <= 0;
	       srpt_queue[ready_match][`GRANTABLE_BYTES] <= srpt_queue[ready_match][`GRANTABLE_BYTES] - srpt_queue[ready_match][`RECV_BYTES];

	       avail_pkts <= avail_pkts - srpt_queue[ready_match][`RECV_BYTES];
	    end // else: !if(srpt_queue[ready_match][`RECV_BYTES] > avail_pkts)
	    /* verilator lint_on WIDTH */
	 end else if (new_active) begin // if (new_grant)
	    // Make room for that new entry 
	    for (entry = MAX_OVERCOMMIT+2; entry < MAX_SRPT-1; entry=entry+1) begin
	       srpt_queue[entry] <= srpt_queue[entry-1];
	    end

	    srpt_queue[MAX_OVERCOMMIT][`PEER_ID]         <= srpt_queue[MAX_OVERCOMMIT][`PEER_ID];
	    srpt_queue[MAX_OVERCOMMIT][`RPC_ID]          <= srpt_queue[MAX_OVERCOMMIT][`RPC_ID];
	    srpt_queue[MAX_OVERCOMMIT][`RECV_BYTES]      <= 32'b0;
	    srpt_queue[MAX_OVERCOMMIT][`GRANTABLE_BYTES] <= 32'b0;
	    srpt_queue[MAX_OVERCOMMIT][`PRIORITY]        <= `SRPT_BLOCK;

	    srpt_queue[MAX_OVERCOMMIT-1] <= srpt_queue[MAX_OVERCOMMIT];
	    srpt_queue[MAX_OVERCOMMIT+1] <= srpt_queue[MAX_OVERCOMMIT-1];

	 end else begin
	    if (swap_type == 1'b0) begin
	       for (entry = 0; entry < MAX_SRPT; entry=entry+1) begin
		  srpt_queue[entry] <= srpt_even[entry];
	       end

	       swap_type <= 1'b1;
	    end else begin
	       for (entry = 0; entry < MAX_OVERCOMMIT-1; entry=entry+1) begin
		  srpt_queue[entry] <= srpt_odd[entry];
	       end

	       for (entry = MAX_OVERCOMMIT+1; entry < MAX_SRPT; entry=entry+1) begin
		  srpt_queue[entry] <= srpt_odd[entry];
	       end

	       swap_type <= 1'b0;
	    end
	 end
      end
   end 
endmodule // srpt_grant_queue

/**
 * srpt_data_pkts_tb() - Test bench for srpt data queue
 */
/* verilator lint_off STMTDLY */
module srpt_grant_pkts_tb();
   reg ap_clk=0;
   reg ap_rst;
   reg ap_ce;
   reg ap_start;
   reg ap_continue;

   wire	ap_idle;
   wire	ap_done;
   wire	ap_ready;

   reg	header_in_empty_i;
   wire	header_in_read_en_o;
   reg [`HEADER_SIZE-1:0] header_in_data_i;
   
   reg			  grant_pkt_full_i;
   wire [`ENTRY_SIZE-1:0] grant_pkt_data_o; 
   wire			  grant_pkt_write_en_o;

   srpt_grant_pkts srpt_queue(.ap_clk(ap_clk), 
			      .ap_rst(ap_rst), 
			      .ap_ce(ap_ce), 
			      .ap_start(ap_start), 
			      .ap_continue(ap_continue), 
			      .header_in_empty_i(header_in_empty_i), 
			      .header_in_read_en_o(header_in_read_en_o), 
			      .header_in_data_i(header_in_data_i), 
			      .grant_pkt_full_i(grant_pkt_full_i), 
			      .grant_pkt_write_en_o(grant_pkt_write_en_o), 
			      .grant_pkt_data_o(grant_pkt_data_o),
			      .ap_idle(ap_idle), 
			      .ap_done(ap_done), 
			      .ap_ready(ap_ready));

   task new_entry(input [15:0] rpc_id, input [13:0] peer_id, input [31:0] msg_len, offset);
      begin
	 
	 header_in_data_i[`HDR_PEER_ID]  = peer_id;
	 header_in_data_i[`HDR_RPC_ID]   = rpc_id;
	 header_in_data_i[`HDR_MSG_LEN]  = msg_len;
	 header_in_data_i[`HDR_OFFSET]   = offset;
	 header_in_data_i[`HDR_INCOMING] = 0;
	 
	 header_in_empty_i = 1;

	 #5;
	 
	 header_in_empty_i = 0;
	 
      end
      
   endtask // new_entry
   
   task get_output();
      begin
	 grant_pkt_full_i = 1;
	 #5;
	 
	 grant_pkt_full_i = 0;
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
      header_in_data_i = {`HEADER_SIZE{1'b0}};
      header_in_empty_i = 0;
      grant_pkt_full_i = 0;
      
      // Send reset signal
      ap_ce = 1; 
      ap_rst = 0;
      ap_start = 0;
      ap_rst = 1;

      #5;
      ap_rst = 0;
      ap_start = 1;

      #5;
      
      new_entry(5, 5, 5, 0);
      new_entry(4, 4, 4, 0);
      new_entry(3, 3, 3, 0);
      new_entry(1, 1, 1, 0);
      new_entry(2, 2, 2, 0);
      new_entry(6, 3, 3, 0);

      #200;
      new_entry(7, 4, 4, 0);
      
      #1000;
      $stop;
   end

endmodule
