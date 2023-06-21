// Input 
//struct srpt_grant_t {
//  uint32_t message_length;  94:63
//  peer_id_t peer_id;        62:49 
//  rpc_id_t rpc_id;          48:35
//  uint32_t grantable;       34:3
//  ap_uint<3> priority;      2:0
//};

// Output 
//struct ready_grant_pkt_t {
//  peer_id_t peer_id;  59:46
//  rpc_id_t rpc_id;    45:32
//  uint32_t grantable; 31:0
//};


// header_in
// peer_id        124:110
// local_id       109:96
// message_length 95:64
// incoming       63:32
// data_offset    31:0


//struct header_t {
//  // Local Values
//  rpc_id_t local_id;
//  peer_id_t peer_id;
//  dbuff_id_t dbuff_id;
//  uint32_t dma_offset;
//  uint32_t processed_bytes;
//  uint32_t packet_bytes;
//  uint32_t rtt_bytes;
//
//  // IPv6 + Common Header
//  uint16_t payload_length;
//  ap_uint<128> saddr;
//  ap_uint<128> daddr;
//  uint16_t sport;
//  uint16_t dport;
//  homa_packet_type type;
//  uint64_t sender_id;
//
//  // Data Header
//  uint32_t message_length; 
//  uint32_t incoming;
//  uint16_t cutoff_version;
//  uint8_t retransmit;
//
//  // Data Segment
//  uint32_t data_offset;
//  uint32_t segment_length;
//
//  // Ack Header
//  uint64_t client_id;
//  uint16_t client_port;
//  uint16_t server_port; 
//
//  // Grant Header
//  uint32_t grant_offset; 
//  uint8_t priority; 
//
//  ap_uint<1> valid;
//};

`define POP 2'b00
`define PUSH 2'b01
`define ORDER 2'b10

`define SRPT_UPDATE 3'b0
`define SRPT_UPDATE_BLOCK 3'b001
`define SRPT_INVALIDATE 3'b010
`define SRPT_UNBLOCK 3'b011
`define SRPT_EMPTY 3'b100
`define SRPT_BLOCKED 3'b101
`define SRPT_ACTIVE 3'b110

parameter MAX_SRPT = 1024;
parameter MAX_OVERCOMMIT = 8;

/**
 * 
 * @priority - Priorities to be inserted
 * @qs_i - Which queues output do we want to see?
 */
module srpt_grant_queue(input ap_clk, ap_rst, ap_ce, ap_start, ap_continue,
			input	      header_in_empty_i,
			output	      header_in_read_en_o,
			input [124:0] header_in_data_i,
			input	      grant_pkt_full_o,
			output	      grant_pkt_write_en_o,
			output	      grant_pkt_data_o,
			output	      ap_idle, ap_done, ap_ready);
   

   reg [124:0]			      active_set[MAX_OVERCOMMIT];
   // TODO  fifo active_set_fifo
   reg [124:0]			      srpt_queue[MAX_SRPT-1:0];

   // TODO move this information to the RPC STATE????
   //reg [31:0]			      recv_bytes[NUM_RPCS];

   // Certainly get rid of this??? This is used for the new_grant process. Store this info in entries.
   //reg [31:0]			      msg_lengths[NUM_RPCS];
   
//   reg [XXXX:0]			      header_in_data_stg;

   reg [3:0]			      peer_match;
   reg [3:0]			      rpc_match;
   
   initial begin
      count = 14'b0;
      for (integer i = 0; i < MAX_SRPT; ++i) begin
	 for (integer j = 0; j < MAX_OVERCOMMIT; ++j) begin
	    srpt_queue[j][i][2:0] = SRPT_EMPTY;
	 end
      end
   end
   
   //  always @(posedge ap_clk) begin
   //     // Is there data to read and space to write?
   //     if (!header_in_empty_i && !grant_pkt_full_o) begin
   //	header_in_read_en_o <= 1;
   //	header_in_data <= header_in_data_i;
   //     end else if (!grant_pkt_full_o) begin
   //	grant_pkt_write_en_o <= 1;
   //	grant_pkt_data_o <= 1;
   //     end
   //  end

   // This is always checking whether the input header matches on peers/rpcs in active set
   always @(header_data_in) begin
      // All 1s is the default value for a non match
      peer_match = {MAX_OVERCOMMIT{1'b1}};
      rpc_match  = {MAX_OVERCOMMIT{1'b1}};
      for (integer i = 0; i < MAX_OVERCOMMIT; ++i) begin
	 if (header_data_in[124:110] == active_set[i][124:110]) begin
	    peer_match = i;
	 end
	 
	 if (header_data_in[109:96].rpc_id == active_set[i][109:96].rpc_id) begin
	    rpc_match = i;
	 end
      end

      always @(posedge ap_clk) begin
	 if (ap_ce && ap_start) begin
	    // Is there data on the input stream, and is there room on the output stream
	    if (!header_in_empty_i && !grant_pkt_full_o) begin
	       // Read the data into the first stage, and acknoledge the data was read
	       header_in_read_en_o <= 1;
	       //header_in_data_stg <= header_in_data_i; TODO Only needed if pipelining?

	       // Will this RPC ever need grants
	       if (header_in_data_i[95:64] <= header_in_data_i[63:32]) begin
		  // The first unscheduled packet creates the entry in the SRPT queue.
		  if (header_in_data[31:0].data_offset == 0) begin
		     // message_lengths[header_in_data_i[XX:YY].local_id] = header_in_data[XX:YY].message_length;

		     // Is the new header peer already in the active set?
		     if (peer_match != {MAX_OVERCOMMIT{1'b1}}) begin
			// TODO maybe make this a submodule
			// Push the new RPC to the SRPT queues
			for (integer qs = 0; qs < MAX_OVERCOMMIT; ++qs) begin
			   for (integer i = 0; i < MAX_SRPT-2; ++i) begin
			      srpt_queue[qs][0] = {header_data_in[95:64], header_data_in[124:110], header_data_in[109:96], header_data_in[95:64] - header_data_in[63:32], (peer_match == i) ? SRPT_ACTIVE : SRPT_BLOCKED};
			      
			      // If the priority differs or grantable bytes
			      if (((srpt_queue[qs][i][2:0] < srpt_queue[qs][i+1][2:0]) || 
				   (srpt_queue[qs][i][34:3] > srpt_queue[qs][i+1][34:3]))) begin
				 srpt_queue[qs][i+2] <= srpt_queue[qs][i];
				 
				 if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
				     (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
				     (srpt_queue[qs][i][2:0] == SRPT_UPDATE_BLOCK)) begin
				    srpt_queue[qs][i+1][34:3] <= srpt_queue[qs][i][34:3];
				    srpt_queue[qs][i+1][2:0] <= SRPT_BLOCKED;
				 end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
					      (srpt_queue[qs][i][2:0] == SRPT_UPDATE_BLOCK)) begin
				    srpt_queue[qs][i+1][2:0] <= SRPT_BLOCKED;
				 end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
					      (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
					      (srpt_queue[qs][i][2:0] == SRPT_UNBLOCK)) begin
				    srpt_queue[qs][i+1][2:0] <= SRPT_ACTIVE;
				 end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
					      (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
					      (srpt_queue[qs][i][2:0] == SRPT_INVALIDATE)) begin
				    srpt_queue[qs][i+1][2:0] <= SRPT_INVALIDATE;
				 end
			      end else begin
				 srpt_queue[qs][i+2] <= srpt_queue[qs][i+1];
				 srpt_queue[qs][i+1] <= srpt_queue[qs][i];
			      end // else: !if((i < count) &&...
			   end // for (integer i = 0; i < MAX_SRPT-2; ++i)
			end // for (integer i = 0; i < MAX_OVERCOMMIT; ++i)
			
		     // There header concerns new unscheduled bytes not in active set
		     end else begin
			// TODO maybe make this a submodule
			// Push the new RPC to the SRPT queues
			for (integer qs = 0; qs < MAX_OVERCOMMIT; ++qs) begin
			   for (integer i = 0; i < MAX_SRPT-2; ++i) begin
			      srpt_queue[qs][0] = {header_data_in[124:110], header_data_in[109:96], header_data_in[95:64] - header_data_in[63:32], SRPT_ACTIVE};
			      
			      // If the priority differs or grantable bytes
			      if (((srpt_queue[qs][i][2:0] < srpt_queue[qs][i+1][2:0]) || 
				   (srpt_queue[qs][i][34:3] > srpt_queue[qs][i+1][34:3]))) begin
				 srpt_queue[qs][i+2] <= srpt_queue[qs][i];
				 
				 if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
				     (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
				     (srpt_queue[qs][i][2:0] == SRPT_UPDATE_BLOCK)) begin
				    srpt_queue[qs][i+1][34:3] <= srpt_queue[qs][i][34:3];
				    srpt_queue[qs][i+1][2:0] <= SRPT_BLOCKED;
				 end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
					      (srpt_queue[qs][i][2:0] == SRPT_UPDATE_BLOCK)) begin
				    srpt_queue[qs][i+1][2:0] <= SRPT_BLOCKED;
				 end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
					      (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
					      (srpt_queue[qs][i][2:0] == SRPT_UNBLOCK)) begin
				    srpt_queue[qs][i+1][2:0] <= SRPT_ACTIVE;
				 end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
					      (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
					      (srpt_queue[qs][i][2:0] == SRPT_INVALIDATE)) begin
				    srpt_queue[qs][i+1][2:0] <= SRPT_INVALIDATE;
				 end
			      end else begin
				 srpt_queue[qs][i+2] <= srpt_queue[qs][i+1];
				 srpt_queue[qs][i+1] <= srpt_queue[qs][i];
			      end // else: !if(((srpt_queue[qs][i][2:0] < srpt_queue[qs][i+1][2:0]) ||...
			   end // for (integer i = 0; i < MAX_SRPT-2; ++i)
			end // for (integer i = 0; i < MAX_OVERCOMMIT; ++i)
		     end // else: !if(peer_match != {MAX_OVERCOMMIT{1'b1}})
		  // Other Received bytes
		  end else begin // if (header_in_data[XX:YY].data_offset == 0)
		     // TODO move this into a seperate core?
		     // TODO compare the granted value in active set vs this to determine difference
		     //recv_bytes[header_in_data[XX:YY].local_id] <= recv_bytes[header_in_data[XX:YY]] + header_in.segment_length;

		     //if (peer_match != {MAX_OVERCOMMIT{1'b1}} && rpc_match != {MAX_OVERCOMMIT{1'b1}}) begin
			/*
			 * Have we received all the bytes from an active grant?
			 * Have we accumulated enough data bytes to justify sending another grant packet?
			 * This avoids sending 1 GRANT for every DATA packet
			 */
			//if (header_data_in[XX:YY].message_length <= header_data_in[XX:YY].data_offset + header_data_in[XX:YY].segment_length) begin
			   //	    srpt_grant_t invalidate = {header_in.peer_id, header_in.local_id, 0, SRPT_INVALIDATE}; 
			   //	    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
			   //#pragma HLS unroll
			   //	      srpt_queue[i].push(invalidate);
			   //	    }
			   //
			   //	    // Free the current peer ID
			   //	    active_set_fifo.insert(peer_match);
			//end else if (recv_bytes[header_in_data[XX:YY]] > GRANT_REFRESH) begin
			   //	    srpt_grant_t unblock = {header_in.peer_id, header_in.local_id, 0, SRPT_UNBLOCK};
			   //	    
			   //	    for (int i = 0; i < MAX_OVERCOMMIT; ++i) {
			   //#pragma HLS unroll
			   //	      if (peer_match != i) srpt_queue[i].push(unblock);
			   //	    }
			   //	    
			   //	    // Free the current peer ID
			   //	    active_set_fifo.insert(peer_match);

			//end
		     //end
		  end // else: !if(header_in_data[XX:YY].data_offset == 0)
	       end // if (header_in_data_i[XX:YY].message_length <= header_in_data_i[XX:YY].incoming)

	       // TODO ...
	       grant_pkt_write_en_o <= 1;
	       grant_pkt_data_o <= 1;
	    end // if (!header_in_empty_i && !grant_pkt_full_o)
	    
	    //	 for (integer qs = 0; MAX_OVERCOMMIT; ++i) begin 
	    //	    if (control_i == PUSH && ops_i[qs] == 1) begin
	    //	       // TODO This will not work
	    //	       srpt_queue[qs][0] = {peer_id_i, rpc_id_i, grantable_i, priority_i};
	    //	       for (integer i = 0; i < MAX_SRPT-2; ++i) begin
	    //		  // If the priority differs or grantable bytes
	    //		  // TODO can you use count like this?
	    //		  if ((i < count) && 
	    //		      ((srpt_queue[qs][i][2:0] < srpt_queue[qs][i+1][2:0]) || 
	    //		       (srpt_queue[qs][i][34:3] > srpt_queue[qs][i+1][34:3]))) begin
	    //		     srpt_queue[qs][i+2] <= srpt_queue[qs][i];
	    //		     
	    //		     if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
	    //			 (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
	    //			 (srpt_queue[qs][i][2:0] == SRPT_UPDATE_BLOCK)) begin
	    //			srpt_queue[qs][i+1][34:3] <= srpt_queue[qs][i][34:3];
	    //			srpt_queue[qs][i+1][2:0] <= SRPT_BLOCKED;
	    //		     end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
	    //				  (srpt_queue[qs][i][2:0] == SRPT_UPDATE_BLOCK)) begin
	    //			srpt_queue[qs][i+1][2:0] <= SRPT_BLOCKED;
	    //		     end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
	    //				  (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
	    //				  (srpt_queue[qs][i][2:0] == SRPT_UNBLOCK)) begin
	    //			srpt_queue[qs][i+1][2:0] <= SRPT_ACTIVE;
	    //		     end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
	    //				  (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
	    //				  (srpt_queue[qs][i][2:0] == SRPT_INVALIDATE)) begin
	    //			srpt_queue[qs][i+1][2:0] <= SRPT_INVALIDATE;
	    //		     end
	    //		  end else begin
	    //		     srpt_queue[qs][i+2] <= srpt_queue[qs][i+1];
	    //		     srpt_queue[qs][i+1] <= srpt_queue[qs][i];
	    //		  end // else: !if((i < count) &&...
	    //	       end // for (integer i = 0; i < MAX_SRPT-2; ++i)
	    //	       count <= count + 1;
	    //	    end // if (control_i == PUSH && ops_i[qs] == 1)
	    //	 end // for (integer qs = 0; MAX_OVERCOMMIT; ++i)
	    //      end // if (ap_ce)
	 end // always @ (posedge ap_clk)
	 
	 assign ap_ready = 1;
	 assign ap_idle = 0;
	 assign ap_done = 1;
	 
	 endmodule // srpt_grant_queue

//else if () begin // if (control_i == PUSH && ops_i[qs] == 1)
//end else if (control_i == POP) begin 
//   for (integer i = 1; i < MAX_SRPT-1; ++i) begin
//      if ((i < count) && 
//	   ((srpt_queue[qs][i][2:0] < srpt_queue[qs][i+1][2:0]) || 
//	    (srpt_queue[qs][i][34:3] > srpt_queue[qs][i+1][34:3]))) begin
//	  srpt_queue[qs][i-1] <= srpt_queue[qs][i+1];
//	  
//	  if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
//	      (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
//	      (srpt_queue[qs][i][2:0] == SRPT_UPDATE_BLOCK)) begin
//	     srpt_queue[qs][i][34:3] <= srpt_queue[qs][i-1][34:3];
//	     srpt_queue[qs][i][2:0] <= SRPT_BLOCKED;
//	  end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
//		       (srpt_queue[qs][i][2:0] == SRPT_UPDATE_BLOCK)) begin
//	     srpt_queue[qs][i][2:0] <= SRPT_BLOCKED;
//	  end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
//		       (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
//		       (srpt_queue[qs][i][2:0] == SRPT_UNBLOCK)) begin
//	     srpt_queue[qs][i][2:0] <= SRPT_ACTIVE;
//	  end else if ((srpt_queue[qs][i][62:49] == srpt_queue[qs][i+1][62:49]) && 
//		       (srpt_queue[qs][i][48:35] == srpt_queue[qs][i+1][48:35]) &&
//		       (srpt_queue[qs][i][2:0] == SRPT_INVALIDATE)) begin
//	     srpt_queue[qs][i][2:0] <= SRPT_INVALIDATE;
//	  end
//      end else begin // if ((i < count) &&...
//	  srpt_queue[qs][i-1] <= srpt_queue[qs][i];
//	  srpt_queue[qs][i]   <= srpt_queue[qs][i+1];
//      end // else: !if((i < count) &&...
//   end // for (integer i = 1; i < MAX_SRPT-1; ++i)
//   count <= count - 1;
//end // if (control_i == POP)

