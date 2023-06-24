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

/* Indicies into grant entry (elements stored in SRPT) */
`define ENTRY_SIZE 95
`define MSG_LEN 94:63
`define PEER_ID 62:49
`define RPC_ID 48:35
`define GRANTABLE 34:3
`define PRIORITY 2:0
`define PRIORITY_SIZE 3

/* Indicies into header input */
`define HEADER_SIZE 124
`define HDR_PEER_ID 123:110
`define HDR_RPC_ID 109:96
`define HDR_MSG_LEN 95:64
`define HDR_INCOMING 63:32
`define HDR_OFFSET 31:0

// TODO maybe allow stealing of entries so long as the peers of those stolen entries are not in the active set?
// TODO need some general ordering operations that can be done when idle (or does not have room to write)

/**
 * 
 * 
 * 
 */
module srpt_queue #(parameter MAX_SRPT = 1024)
   (input ap_clk,
    input		     ap_rst,
    input		     ap_ce,
    input		     ap_start,
    input [`ENTRY_SIZE-1:0]  write,
    input		     we,
    output [`ENTRY_SIZE-1:0] read);

   // These will be actual registers
   reg [`ENTRY_SIZE-1:0]		   srpt_queue[MAX_SRPT-1:0];

   // This is reg type but should not create a register. Like "logic" in SV.
   reg [`ENTRY_SIZE-1:0]		   srpt_swap_even[MAX_SRPT-1:0];
   reg [`ENTRY_SIZE-1:0]		   srpt_swap_odd[MAX_SRPT-1:0];

   integer				   entry;

   assign read = srpt_queue[0];
   
   /* 
    * This will create a bus of wires, srpt_swap, which is what the SRPT queue will look like if we were to perform a swap with the even entries. 
    * The even entries means we compare and swap 0 <-> 1, 2<->3, 4<->5. These values will not actually modify the state of the queue. They are 
    * just used for the next swap operation, which compares the odd entries. We have to do this as a two step process because it is possible there 
    * is an entry which has a lower priority entry below it, and a higher priority entry above it. If swaps were happened at the same time between 
    * every value at once, we could destroy data.
    */
   always @* begin
      if ((write[`PRIORITY] != srpt_queue[0][`PRIORITY]) ? 
	  (write[`PRIORITY] < srpt_queue[0][`PRIORITY]) : 
	  (write[`GRANTABLE] > srpt_queue[0][`GRANTABLE])) begin
	 srpt_swap_even[0] = srpt_queue[0];
	 srpt_swap_even[1] = write;
      end else begin
	 srpt_swap_even[0] = write;
	 srpt_swap_even[1] = srpt_queue[0];
      end

      /* 
       * This performs a swap and shift operation for two entries in the SRPT queue: srpt_queue[entry-1], srpt_queue[entry]
       * if the priority of srpt_queue[entry-1] is less then that of srpt_queue[entry] then we need to perform a swap
       * Ordinarily this would make srpt_queue[entry] go to srpt_swap[entry-1], BUT WE ARE ALSO PERFORMING A SHIFT, so use srpt_swap[entry]
       * if the priority does not differ, then use the number of grantable bytes instead
       * We use the same relation but the opposite result for srpt_swap[entry+1]
       */
      for (entry = 2; entry < MAX_SRPT-2; entry=entry+2) begin
	 // If the priority differs or grantable bytes

	 if ((srpt_queue[entry-1][`PRIORITY] != srpt_queue[entry][`PRIORITY]) 
	   ? (srpt_queue[entry-1][`PRIORITY] < srpt_queue[entry][`PRIORITY]) 
	   : (srpt_queue[entry-1][`GRANTABLE] > srpt_queue[entry][`GRANTABLE])) begin
	    
	    srpt_swap_even[entry][`MSG_LEN] = srpt_queue[entry][`MSG_LEN];
	    srpt_swap_even[entry][`PEER_ID] = srpt_queue[entry][`PEER_ID];
	    srpt_swap_even[entry][`RPC_ID] = srpt_queue[entry][`RPC_ID];

	    srpt_swap_even[entry+1] = srpt_queue[entry-1];

	    if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
	    	(srpt_queue[entry-1][`RPC_ID] == srpt_queue[entry][`RPC_ID]) &&
	    	(srpt_queue[entry-1][`PRIORITY] == `SRPT_UPDATE_BLOCK)) begin
	        
	       srpt_swap_even[entry][`GRANTABLE] = srpt_queue[entry-1][`GRANTABLE];
	       srpt_swap_even[entry][`PRIORITY] = `SRPT_BLOCKED;
	       
	    end else if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
	    		 (srpt_queue[entry-1][`PRIORITY] == `SRPT_UPDATE_BLOCK)) begin
	       
	       srpt_swap_even[entry][`GRANTABLE] = srpt_queue[entry][`GRANTABLE];
	       srpt_swap_even[entry][`PRIORITY] = `SRPT_BLOCKED;
	       
	    end else if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
	    		 (srpt_queue[entry-1][`PRIORITY] == `SRPT_UNBLOCK)) begin
	        
	       srpt_swap_even[entry][`GRANTABLE] = srpt_queue[entry][`GRANTABLE];
	       srpt_swap_even[entry][`PRIORITY] = `SRPT_ACTIVE;
	       
	    end else if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
	    		 (srpt_queue[entry-1][`RPC_ID] == srpt_queue[entry][`RPC_ID]) &&
	    		 (srpt_queue[entry-1][`PRIORITY] == `SRPT_INVALIDATE)) begin
	       
	       srpt_swap_even[entry][`GRANTABLE] = srpt_queue[entry][`GRANTABLE];
	       srpt_swap_even[entry][`PRIORITY] = `SRPT_EMPTY;
	    end

	 end else begin // if ((srpt_queue[entry-1][2:0] != srpt_queue[entry][2:0])...
	    srpt_swap_even[entry+1] = srpt_queue[entry];
	    srpt_swap_even[entry] = srpt_queue[entry-1];
	 end // else: !if((srpt_queue[entry-1][2:0] != srpt_queue[entry][2:0])...
      end

      srpt_swap_odd[0] = srpt_swap_even[0];
      
      for (entry = 2; entry < MAX_SRPT-2; entry=entry+2) begin
	 // If the priority differs or grantable bytes
	 if ((srpt_swap_even[entry-1][`PRIORITY] != srpt_swap_even[entry][`PRIORITY]) 
	   ? (srpt_swap_even[entry-1][`PRIORITY] < srpt_swap_even[entry][`PRIORITY]) 
	   : (srpt_swap_even[entry-1][`GRANTABLE] > srpt_swap_even[entry][`GRANTABLE])) begin

	    srpt_swap_odd[entry] = srpt_swap_even[entry-1];
	    
	    srpt_swap_odd[entry-1][`MSG_LEN] = srpt_swap_even[entry][`MSG_LEN];
	    srpt_swap_odd[entry-1][`PEER_ID] = srpt_swap_even[entry][`PEER_ID];
	    srpt_swap_odd[entry-1][`RPC_ID] = srpt_swap_even[entry][`RPC_ID];

	    if ((srpt_swap_even[entry-1][`PEER_ID] == srpt_swap_even[entry][`PEER_ID]) && 
	    	(srpt_swap_even[entry-1][`RPC_ID] == srpt_swap_even[entry][`RPC_ID]) &&
	    	(srpt_swap_even[entry-1][`PRIORITY] == `SRPT_UPDATE_BLOCK)) begin
	        
	       srpt_swap_odd[entry-1][`GRANTABLE] = srpt_swap_even[entry][`GRANTABLE];
	       srpt_swap_odd[entry-1][`PRIORITY] = `SRPT_BLOCKED;
	       
	    end else if ((srpt_swap_even[entry-1][`PEER_ID] == srpt_swap_even[entry][`PEER_ID]) && 
	    		 (srpt_swap_even[entry-1][`PRIORITY] == `SRPT_UPDATE_BLOCK)) begin
	       
	       srpt_swap_odd[entry-1][`GRANTABLE] = srpt_swap_even[entry][`GRANTABLE];
	       srpt_swap_odd[entry-1][`PRIORITY] = `SRPT_BLOCKED;
	       
	    end else if ((srpt_swap_even[entry-1][`PEER_ID] == srpt_swap_even[entry][`PEER_ID]) && 
	    		 (srpt_swap_even[entry-1][`PRIORITY] == `SRPT_UNBLOCK)) begin
	        
	       srpt_swap_odd[entry-1][`GRANTABLE] = srpt_swap_even[entry][`GRANTABLE];
	       srpt_swap_odd[entry-1][`PRIORITY] = `SRPT_ACTIVE;
	       
	    end else if ((srpt_swap_even[entry-1][`PEER_ID] == srpt_swap_even[entry][`PEER_ID]) && 
	    		 (srpt_swap_even[entry-1][`RPC_ID] == srpt_swap_even[entry][`RPC_ID]) &&
	    		 (srpt_swap_even[entry-1][`PRIORITY] == `SRPT_INVALIDATE)) begin
	       
	       srpt_swap_odd[entry-1][`GRANTABLE] = srpt_swap_even[entry][`GRANTABLE];
	       srpt_swap_odd[entry-1][`PRIORITY] = `SRPT_EMPTY;
	    end
	 end else begin // if ((srpt_swap_even[entry-1][2:0] != srpt_swap_even[entry][2:0])...
	    srpt_swap_odd[entry] = srpt_swap_even[entry];
	    srpt_swap_odd[entry-1] = srpt_swap_even[entry-1];
	 end
      end // for (entry = 2; entry < MAX_SRPT-2; entry=entry+2)
   end // always @ *
   
   integer			 rst_entry;
   always @(posedge ap_clk) begin
      if (ap_rst) begin
	 for (rst_entry = 0; rst_entry < MAX_SRPT; rst_entry=rst_entry+1) begin
	    srpt_queue[rst_entry][`ENTRY_SIZE-1:0] <= {{(`ENTRY_SIZE-3){1'b0}}, `SRPT_EMPTY};
	 end
      end else if (ap_ce && ap_start) begin
	 if (we) begin
	    for (entry = 0; entry < MAX_SRPT-2; entry=entry+1) begin
	       // If the priority differs or grantable bytes
	       srpt_queue[entry] <= srpt_swap_odd[entry];
	    end
	 end
      end
   end // always @ (posedge ap_clk)

endmodule

/**
 * 
 * 
 * TODO this needs to handle OOO packets (how does that effect SRPT entry creation?).
 * TODO this needs to consider duplicate packets (can't just increment recv bytes?).
 * The packet map core may need to be involved somehow
 */
module srpt_grant_pkts(input ap_clk, ap_rst, ap_ce, ap_start, ap_continue,
		       input			    header_in_empty_i,
		       output reg		    header_in_read_en_o,
		       input [`HEADER_SIZE-1:0]	    header_in_data_i,
		       input			    grant_pkt_full_o,
		       output reg		    grant_pkt_write_en_o,
		       output reg [`ENTRY_SIZE-1:0] grant_pkt_data_o,
		       output			    ap_idle, ap_done, ap_ready);
   
   parameter			 MAX_OVERCOMMIT = 8;
   parameter			 MAX_OVERCOMMIT_LOG2 = 3;

   reg [`ENTRY_SIZE-1:0]			    active_set[MAX_OVERCOMMIT-1:0];

   reg						    we;
   reg [`PRIORITY_SIZE-1:0]			    priorities[MAX_OVERCOMMIT-1:0];
   
   wire [`ENTRY_SIZE-1:0]			    read[MAX_OVERCOMMIT-1:0];
   reg [`ENTRY_SIZE-1:0]			    write[MAX_OVERCOMMIT-1:0];
   
   genvar					    queue;
  
   generate
      for (queue = 0; queue < MAX_OVERCOMMIT; queue = queue+1) begin
	 srpt_queue srpt(.ap_clk(ap_clk), 
			 .ap_rst(ap_rst), 
			 .ap_ce(ap_ce), 
			 .ap_start(ap_start), 
			 .write(write[queue]),
			 .we(we),
			 .read(read[queue]));
	 end
   endgenerate
  
   // TODO move this information to the RPC STATE????
   //reg [31:0]			      recv_bytes[NUM_RPCS];

   // These are reg types and NOT actual registers
   reg [MAX_OVERCOMMIT_LOG2-1:0]			    peer_match;   // Does the incoming header match on an entry in active set
   reg [MAX_OVERCOMMIT_LOG2-1:0]			    rpc_match;    // Does the incoming header match on an entry in active set
   reg [MAX_OVERCOMMIT_LOG2-1:0]			    next_active;  // Active set entries to be filled
   reg [MAX_OVERCOMMIT_LOG2-1:0]			    clear_active; // Active set entries to be cleared

   reg							    peer_match_en ;
   reg							    rpc_match_en;
   reg							    next_active_en;
   reg							    clear_active_en;

   integer					    i;
   integer					    j;

   /*
    * Whenever the header data is updating
    *   1) Determine if the header's peer is in the active set, and if so, what index
    *   2) Determine if the header's rpc is in the active set, and if so, what index
    *   3) Determine the next open entry in the active set, if any
    */
   always @* begin

      // Init all values so that no latches are inferred. All control paths assign a value.
      peer_match  = {MAX_OVERCOMMIT_LOG2{1'b1}};
      rpc_match   = {MAX_OVERCOMMIT_LOG2{1'b1}};
      next_active = {MAX_OVERCOMMIT_LOG2{1'b1}};
      clear_active = {MAX_OVERCOMMIT_LOG2{1'b1}};

      peer_match_en  = 1'b0;
      rpc_match_en   = 1'b0;
      next_active_en = 1'b0;
      clear_active_en = 1'b0;

      for (i = 0; i < MAX_OVERCOMMIT; i=i+1) begin
	 priorities[i] = `SRPT_BLOCKED;
      end
      
      for (i = 0; i < MAX_OVERCOMMIT; i = i + 1) begin
	 write[i] = {`ENTRY_SIZE{1'b1}};
      end

      // Check every entry in the active set
      for (i = 0; i < MAX_OVERCOMMIT; i=i+1) begin
	 // Does the new header have the same peer as the entry in the active set
	 if (header_in_data_i[`HDR_PEER_ID] == active_set[i][`PEER_ID] && peer_match_en == 1'b0) begin
	    peer_match = i[MAX_OVERCOMMIT_LOG2-1:0];
	    peer_match_en = 1'b1;
	 end else begin
	    peer_match = peer_match;
	 end 
	
	 // Does the new header have the same RPC ID as the entry in the active set
	 if (header_in_data_i[`HDR_RPC_ID] == active_set[i][`RPC_ID] && rpc_match_en == 1'b0) begin
	    rpc_match = i[MAX_OVERCOMMIT_LOG2-1:0];
	    rpc_match_en = 1'b1;
	 end else begin
	    rpc_match = rpc_match;
	 end
	
	 // Is the entry in the active set empty, and so, should it be filled and a GRANT packet sent?
	 if (active_set[j][`PRIORITY] == `SRPT_EMPTY && next_active_en == 1'b0) begin
	    next_active = j[MAX_OVERCOMMIT_LOG2-1:0];
	    next_active_en = 1'b1;
	 end else begin
	    next_active = next_active;
	 end
      end // for (i = 0; i < MAX_OVERCOMMIT; i=i+1)

      
      // Begin with the assumption we will not write to SRPT queues
      we = 1'b0;

      // Is there new incoming DATA packets we should process?
      if (!header_in_empty_i) begin
	 
	 // If the message length is less than or equal to the incoming bytes, we would have nothing to do here
	 if (header_in_data_i[`HDR_MSG_LEN] <= header_in_data_i[`HDR_INCOMING]) begin
	    
	    // It is the first unscheduled packet that creates the entry in the SRPT queue.
	    if (header_in_data_i[`HDR_OFFSET] == 0) begin
	       
	       // Is the new header's peer already in the active set?
	       if (peer_match != {MAX_OVERCOMMIT_LOG2{1'b1}}) begin

		  /*
		   * If the new header's peer is in the active set:
		   *   1) Insert the entry in a blocked state into all queues associated with active set entries that did not match
		   *   2) Insert the entry in an unblocked state into the queue associated with the active set entry that did match
		   */  
		  for (i = 0; i < MAX_OVERCOMMIT; i=i+1) begin
		     // TODO is there a type issue here?
		     priorities[i] = (peer_match == i[MAX_OVERCOMMIT_LOG2-1:0]) ? `SRPT_ACTIVE : `SRPT_BLOCKED;
		  end
	       end else begin // if (peer_match != {MAX_OVERCOMMIT{1'b1}})
		  // If the peer is not actively granted to that means the incoming header immediately eligible on all slots
		  for (i = 0; i < MAX_OVERCOMMIT; i=i+1) begin
		     // TODO is there a type issue here?
		     priorities[i] = `SRPT_ACTIVE;
		  end
	       end // else: !if(peer_match != {MAX_OVERCOMMIT{1'b1}})

	       // TODO Set the number of recv bytes here
	       
	    // A general DATA packet, not the first packet of the unscheduled bytes (which creates the original SRPT entry)
	    end else begin // if (header_in_data_i[HDR_OFFSET] == 0)
	       // TODO increment the recv bytes by segment length (or really set a flag so it can be done at posedge)

	       // Did we just receive data from an RPC within our active set? If so, bump it to the queues.
	       if (peer_match != -1 && rpc_match != -1) begin
		  // If the message has no more bytes to grant, then remove it from this core entirely
		  if (header_in_data_i[`HDR_MSG_LEN] == header_in_data_i[`HDR_INCOMING]) begin
		     for (i = 0; i < MAX_OVERCOMMIT; i=i+1) begin
			// TODO is there a type issue here?
			priorities[i] = `SRPT_INVALIDATE;
		     end
		  // If the message has more bytes to grant, then just unblock the RPCs for this peer in all the other cores
		  // This will evict an entry for a single received packet (in contrast to waiting for some refresh size like 10000bytes)
		  end else begin
		     for (i = 0; i < MAX_OVERCOMMIT; i=i+1) begin
			// TODO is there a type issue here?
			priorities[i] = `SRPT_UNBLOCK;
		     end
		  end

		  clear_active = peer_match;
		  clear_active_en = 1'b1;
	       end
	    end
	 end // if (header_in_data_i[HDR_MSG_LEN] <= header_in_data_i[HDR_INCOMING])

	 // This configures pure messages as well as actual entries
	 for (i = 0; i < MAX_OVERCOMMIT; i = i + 1) begin
	    write[i] = {header_in_data_i[`HDR_MSG_LEN], 
			header_in_data_i[`HDR_PEER_ID], 
			header_in_data_i[`HDR_RPC_ID], 
			header_in_data_i[`HDR_MSG_LEN] - header_in_data_i[`HDR_INCOMING], 
			priorities[i]};
	 end
	 
	 // We want to write this new data to the SRPT queue
	 we = 1'b1;
      // Is there room on our output FIFO for data, and do we have space to send another grant?
      end else if (!grant_pkt_full_o && next_active_en == 1'b1) begin // if (!header_in_empty_i)
	 // Is the queue empty/stable
	 if (read[next_active][`PRIORITY] != `SRPT_ACTIVE) begin
	    priorities[next_active] = `SRPT_UPDATE;

	    for (i = 0; i < MAX_OVERCOMMIT; i=i+1) begin
	       if (next_active == i[MAX_OVERCOMMIT_LOG2-1:0]) begin 
		  priorities[i] = `SRPT_UPDATE_BLOCK;
	       end
	    end
	 end
	 
	 // Configure messages for every queue 
	 for (i = 0; i < MAX_OVERCOMMIT; i = i + 1) begin
	    // Send updates or update block requests with new grantable value
	    write[i] = {read[next_active][`MSG_LEN],
			read[next_active][`PEER_ID],
			read[next_active][`RPC_ID],
			read[next_active][`GRANTABLE] - 1000,// TODO minus RECV BYTES, which should be stored alongside? Ideally not in a BRAM..
			priorities[i]};
	 end

	 // We want to write this new data to the SRPT queue
	 we = 1'b1;
      end // if (!grant_pkt_full_o && empty_active != {MAX_OVERCOMMIT{1'b}})
   end // always @ *
   
   always @(posedge ap_clk) begin
      if (ap_rst) begin
	 grant_pkt_data_o <= {`ENTRY_SIZE{1'b0}};
      grant_pkt_write_en_o <= 1'b0;
      header_in_read_en_o <= 1'b0;

	 for (i = 0; i < MAX_OVERCOMMIT; i=i+1) begin
	    active_set[i] = {{(`ENTRY_SIZE - 3){1'b0}}, `SRPT_EMPTY};
	 end
      end else if (ap_ce && ap_start) begin // if (ap_rst)
	 // Is there data on the input stream, and is there room on the output stream
	 if (!header_in_empty_i) begin
	    // Read the data into the first stage, and acknowledge the data was read
	    header_in_read_en_o <= 1;

	    if (clear_active_en) begin
	       active_set[clear_active][`PRIORITY] = `SRPT_EMPTY;
	    end

	    // We did NOT write a grant packet header this cycle
	    grant_pkt_write_en_o <= 0;
	 // Is there an open active set entry, does that entry's queue have a value ready
	 end else if (!grant_pkt_full_o && next_active_en == 1'b1) begin // if (!header_in_empty_i)
	    grant_pkt_data_o <= read[next_active];
	    active_set[next_active] <= read[next_active];
	    grant_pkt_write_en_o <= 1;

	    // We did NOT read a input header this cycle
	    header_in_read_en_o <= 0;
	 end
      end // if (ap_ce && ap_start)
   end // always @ (posedge ap_clk)
   
   assign ap_ready = 1;
   assign ap_idle = 0;
   assign ap_done = 1;
   
endmodule // srpt_grant_queue
