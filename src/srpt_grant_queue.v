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
//`define ENTRY_SIZE 95
// `define MSG_LEN 94:63
// log2(1000000/1386)
//`define ENTRY_SIZE 63 
//`define PEER_ID 62:49
//`define RPC_ID 48:35
//`define GRANTABLE 34:3
//`define PRIORITY 2:0
//`define PRIORITY_SIZE 3

//`define ENTRY_SIZE 50
//`define PEER_ID 51:42
//`define RPC_ID 42:33 // TODO wrong
//`define RECV_PKTS 32:23
//`define GRNTBLE_PKTS 22:13
//`define MSG_PKTS 12:3
//`define PRIORITY 2:0
//`define PRIORITY_SIZE 3

`define ENTRY_SIZE 51
`define PEER_ID 50:37
`define RPC_ID 36:23
`define RECV_PKTS 22:13
`define GRNTABLE_PKTS 12:3
`define PRIORITY 2:0
`define PRIORITY_SIZE 3


/* Indicies into header input */
`define HEADER_SIZE 80
`define HDR_PEER_ID 79:66
`define HDR_RPC_ID 65:52
`define HDR_MSG_LEN 51:42
`define HDR_INCOMING 41:32
`define HDR_OFFSET 31:0

/**
 * 
 * 
 * 
 */
module srpt_queue #(parameter MAX_SRPT = 1024)
   (input ap_clk,
    input                    ap_rst,
    input                    ap_ce,
    input                    ap_start,
    input [`ENTRY_SIZE-1:0]  write,
    input                    write_en,
    input [`ENTRY_SIZE-1:0]  forward,
    input                    forward_en,
    output [`ENTRY_SIZE-1:0] read);

   // These will be actual registers
   reg [`ENTRY_SIZE-1:0]     srpt_queue[MAX_SRPT-1:0];

   // This is reg type but should not create a register. Like "logic" in SV.
   reg [`ENTRY_SIZE-1:0]     swap_odd[MAX_SRPT:0]; // swap_even
   reg [`ENTRY_SIZE-1:0]     swap_even[MAX_SRPT:0]; // swap_odd

   reg                       swap_type;

   integer                   entry;

   assign read = srpt_queue[0];


   /* Cases:
    * write_en and forward_en are low so we just SRPT on the existing data in
    * the core, alternating between even and odd swaps.
    *
    * write_en is enabled, so we do an even swap/push and restore the data
    *
    * forward_en is enabled, so we perform an odd swap bridging the boundary
    * between the two cores. We can just set the 0th entry in this queue
    * directly with the forward value, as the active queue will be taking the
    * value coming from read
    */

   /*
    * Active queue -> Main queue
    * SRPT_QUEUE:               6 : 7 : 8 : 0 : 1 : 2 : 3 :
    * write_en/forward_en = 0:  6 : 7 : 8 | 0 : 1 | 2 : 3 |       {(bottom_even), (srpt_even)}
    *                           6 : 7 : 8 : 0 | 1 : 2 | 3 :       {(srpt_queue[0]), (srpt_odd)}
    * write_en = 1:             6 : 7 : 8 | N : 0 | 1 : 2 |       {(srpt_odd)}
    * forward_en = 1:           6 : 7 | 8 : 0 | 1 : 2 | 3 :       {(forward), (srpt_odd)}
    * write_en/forward_en = 1:  6 : 7 | 8 : (N) : 0 | 1 : 2 | 3 : {(write), (forward) (srpt_odd)}
   */

   always @* begin
   
      if (write_en) begin
         if ((write[`PRIORITY] != srpt_queue[0][`PRIORITY]) 
            ? (write[`PRIORITY] < srpt_queue[0][`PRIORITY]) 
            : (write[`GRANTABLE] > srpt_queue[0][`GRANTABLE])) begin

            srpt_odd[0] = srpt_queue[0];
            srpt_odd[1] = write;

         end else begin
            srpt_odd[1] = srpt_queue[0];
            srpt_odd[0] = write;
         end 

      else begin
         srpt_odd[1] = srpt_queue[0];
      end

      // Compute srpt_odd
      for (entry = 2; entry < MAX_SRPT; entry=entry+2) begin

         // If the priority differs or grantable bytes
         if ((srpt_queue[entry-1][`PRIORITY] != srpt_queue[entry][`PRIORITY]) 
             ? (srpt_queue[entry-1][`PRIORITY] < srpt_queue[entry][`PRIORITY]) 
             : (srpt_queue[entry-1][`GRANTABLE] > srpt_queue[entry][`GRANTABLE])) begin

            //srpt_swap_even[entry][`GRANTABLE] = srpt_queue[entry][`GRANTABLE];
            //srpt_swap_even[entry][`PEER_ID] = srpt_queue[entry][`PEER_ID];
            //srpt_swap_even[entry][`RPC_ID] = srpt_queue[entry][`RPC_ID];

            srpt_odd[entry] = srpt_queue[entry];
            srpt_odd[entry+1] = srpt_queue[entry-1];

            //if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
            //    (srpt_queue[entry-1][`PRIORITY] == `SRPT_BLOCK)) begin

            //   srpt_swap_even[entry][`PRIORITY] = `SRPT_BLOCKED;

            //end else if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
            //             (srpt_queue[entry-1][`PRIORITY] == `SRPT_UNBLOCK)) begin

            //   srpt_swap_even[entry][`PRIORITY] = `SRPT_ACTIVE;

            //end 
         end else begin // if ((srpt_queue[entry-1][2:0] != srpt_queue[entry][2:0])...
            swap_odd[entry+1] = srpt_queue[entry];
            swap_odd[entry] = srpt_queue[entry-1];
         end // else: !if((srpt_queue[entry-1][2:0] != srpt_queue[entry][2:0])...
      end


      // Compute srpt_even
      for (entry = 1; entry < MAX_SRPT; entry=entry+2) begin

         // If the priority differs or grantable bytes
         if ((srpt_queue[entry-1][`PRIORITY] != srpt_queue[entry][`PRIORITY]) 
             ? (srpt_queue[entry-1][`PRIORITY] < srpt_queue[entry][`PRIORITY]) 
             : (srpt_queue[entry-1][`GRANTABLE] > srpt_queue[entry][`GRANTABLE])) begin

            //srpt_swap_even[entry][`GRANTABLE] = srpt_queue[entry][`GRANTABLE];
            //srpt_swap_even[entry][`PEER_ID] = srpt_queue[entry][`PEER_ID];
            //srpt_swap_even[entry][`RPC_ID] = srpt_queue[entry][`RPC_ID];

            srpt_even[entry] = srpt_queue[entry];
            srpt_even[entry+1] = srpt_queue[entry-1];

            //if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
            //    (srpt_queue[entry-1][`PRIORITY] == `SRPT_BLOCK)) begin

            //   srpt_swap_even[entry][`PRIORITY] = `SRPT_BLOCKED;

            //end else if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
            //             (srpt_queue[entry-1][`PRIORITY] == `SRPT_UNBLOCK)) begin

            //   srpt_swap_even[entry][`PRIORITY] = `SRPT_ACTIVE;

            //end 
         end else begin // if ((srpt_queue[entry-1][2:0] != srpt_queue[entry][2:0])...
            swap_even[entry+1] = srpt_queue[entry];
            swap_even[entry] = srpt_queue[entry-1];
         end // else: !if((srpt_queue[entry-1][2:0] != srpt_queue[entry][2:0])...
      end


      // TODO need to replace comparator, and need to alternate between even
      // and odd swaps (unless we are pushing an element?)
     

      // Compute the lower elements in the case of insert only
      //if ((write[`PRIORITY] != srpt_queue[0][`PRIORITY]) 
      //   ? (write[`PRIORITY] < srpt_queue[0][`PRIORITY]) 
      //   : (write[`GRANTABLE] > srpt_queue[0][`GRANTABLE])) begin

      //   //srpt_swap_even[0][`PEER_ID] = srpt_queue[0][`PEER_ID];
      //   //srpt_swap_even[0][`RPC_ID] = srpt_queue[0][`RPC_ID];
      //   //srpt_swap_even[0][`GRANTABLE] = srpt_queue[0][`GRANTABLE];
   
      //   bottom_insert[0] = srpt_queue[0];
      //   bottom_insert[1] = write;

      //   //if ((write[`PEER_ID] == srpt_queue[0][`PEER_ID]) && 
      //   //    (write[`PRIORITY] == `SRPT_BLOCK)) begin

      //   //   srpt_swap_even[0][`PRIORITY] = `SRPT_BLOCKED;

      //   //end else if ((write[`PEER_ID] == srpt_queue[0][`PEER_ID]) && 
      //   //             (write[`PRIORITY] == `SRPT_UNBLOCK)) begin

      //   //   srpt_swap_even[0][`PRIORITY] = `SRPT_ACTIVE;

      //   //end 
      //end else begin
      //   bottom_insert[1] = srpt_queue[0];
      //   bottom_insert[0] = write;
      //end 

      //if ((forward[`PRIORITY] != srpt_queue[0][`PRIORITY]) 
      //  ? (forward[`PRIORITY] < srpt_queue[0][`PRIORITY]) 
      //  : (forward[`GRANTABLE] > srpt_queue[0][`GRANTABLE])) begin

      //   //srpt_swap_even[0][`PEER_ID] = srpt_queue[0][`PEER_ID];
      //   //srpt_swap_even[0][`RPC_ID] = srpt_queue[0][`RPC_ID];
      //   //srpt_swap_even[0][`GRANTABLE] = srpt_queue[0][`GRANTABLE];
   
      //   bottom_forward[0] = srpt_queue[0];
      //   bottom_forward[1] = forward;

      //   //if ((write[`PEER_ID] == srpt_queue[0][`PEER_ID]) && 
      //   //    (write[`PRIORITY] == `SRPT_BLOCK)) begin

      //   //   srpt_swap_even[0][`PRIORITY] = `SRPT_BLOCKED;

      //   //end else if ((write[`PEER_ID] == srpt_queue[0][`PEER_ID]) && 
      //   //             (write[`PRIORITY] == `SRPT_UNBLOCK)) begin

      //   //   srpt_swap_even[0][`PRIORITY] = `SRPT_ACTIVE;

      //   //end 
      //end else begin
      //   bottom_forward[1] = srpt_queue[0];
      //   bottom_forward[0] = write;
      //end 



      
      // If the priority differs or grantable bytes
      //if ((write[`PRIORITY] != srpt_queue[0][`PRIORITY]) 
      //  ? (write[`PRIORITY] < srpt_queue[0][`PRIORITY]) 
      //  : (write[`GRANTABLE] > srpt_queue[0][`GRANTABLE])) begin

      //   //srpt_swap_even[0][`PEER_ID] = srpt_queue[0][`PEER_ID];
      //   //srpt_swap_even[0][`RPC_ID] = srpt_queue[0][`RPC_ID];
      //   //srpt_swap_even[0][`GRANTABLE] = srpt_queue[0][`GRANTABLE];
      //   
      //   srpt_insert[0] = srpt_queue[0];
      //   srpt_insert[1] = write;

      //   //if ((write[`PEER_ID] == srpt_queue[0][`PEER_ID]) && 
      //   //    (write[`PRIORITY] == `SRPT_BLOCK)) begin

      //   //   srpt_swap_even[0][`PRIORITY] = `SRPT_BLOCKED;

      //   //end else if ((write[`PEER_ID] == srpt_queue[0][`PEER_ID]) && 
      //   //             (write[`PRIORITY] == `SRPT_UNBLOCK)) begin

      //   //   srpt_swap_even[0][`PRIORITY] = `SRPT_ACTIVE;

      //   //end 
      //end else begin
      //   srpt_insert[1] = srpt_queue[0];
      //   srpt_insert[0] = write;
      //end 

      //srpt_insert[MAX_SRPT] = srpt_queue[MAX_SRPT-1];

      /* 
       * This performs a swap and shift operation for two entries in the 
       * SRPT queue: srpt_queue[entry-1], srpt_queue[entry]. If the priority 
       * of srpt_queue[entry-1] is less then that of srpt_queue[entry] then 
       * we need to perform a swap Ordinarily this would make srpt_queue[entry] 
       * go to srpt_swap[entry-1], BUT WE ARE ALSO PERFORMING A SHIFT, so use 
       * srpt_swap[entry]. If the priority does not differ, then use the number 
       * of grantable bytes instead. We use the same relation but the opposite 
       * result for srpt_swap[entry+1].
       */


      // If the priority differs or grantable bytes
      //if ((forward[`PRIORITY] != srpt_queue[0][`PRIORITY]) 
      //    ? (forward[`PRIORITY] < srpt_queue[0][`PRIORITY]) 
      //    : (forward[`GRANTABLE] > srpt_queue[0][`GRANTABLE])) begin
      //   srpt_bridge[0] = srpt_queue[0];
      //end else begin
      //   srpt_bridge[0] = write;
      //end 

      // srpt_bridge[0] is replaced by forward in the case of forward_en being
      // high

      //for (entry = 2; entry < MAX_SRPT+1; entry=entry+2) begin
      //   // If the priority differs or grantable bytes
      //   if ((srpt_queue[entry-1][`PRIORITY] != srpt_queue[entry][`PRIORITY]) 
      //     ? (srpt_queue[entry-1][`PRIORITY] <  srpt_queue[entry][`PRIORITY]) 
      //     : (srpt_queue[entry-1][`GRANTABLE] > srpt_queue[entry][`GRANTABLE])) begin

      //      srpt_bridge[entry-1] = srpt_queue[entry];
      //      srpt_bridge[entry] = srpt_queue[entry-1];

      //      //srpt_swap_odd[entry-1][`GRANTABLE] = srpt_swap_even[entry][`GRANTABLE];
      //      //srpt_swap_odd[entry-1][`PEER_ID] = srpt_swap_even[entry][`PEER_ID];
      //      //srpt_swap_odd[entry-1][`RPC_ID] = srpt_swap_even[entry][`RPC_ID];

      //      //if ((srpt_swap_even[entry-1][`PEER_ID] == srpt_swap_even[entry][`PEER_ID]) && 
      //      //    (srpt_swap_even[entry-1][`PRIORITY] == `SRPT_BLOCK)) begin

      //      //   srpt_swap_odd[entry-1][`PRIORITY] = `SRPT_BLOCKED;

      //      //end else if ((srpt_swap_even[entry-1][`PEER_ID] == srpt_swap_even[entry][`PEER_ID]) && 
      //      //             (srpt_swap_even[entry-1][`PRIORITY] == `SRPT_UNBLOCK)) begin

      //      //   srpt_swap_odd[entry-1][`PRIORITY] = `SRPT_ACTIVE;

      //      //end 
      //   end else begin // if ((srpt_swap_even[entry-1][`PRIORITY] != srpt_swap_even[entry][`PRIORITY])...
      //      srpt_bridge[entry] = srpt_queue[entry];
      //      srpt_bridge[entry-1] = srpt_queue[entry-1];
      //   end
      //end
   end

   integer                       rst_entry;
   always @(posedge ap_clk) begin
      if (ap_rst) begin
         swap_type = 1'b0;
         for (rst_entry = 0; rst_entry < MAX_SRPT; rst_entry=rst_entry+1) begin
            srpt_queue[rst_entry][`ENTRY_SIZE-1:0] <= {{(`ENTRY_SIZE-3){1'b0}}, `SRPT_EMPTY};
         end
      end else if (ap_ce && ap_start) begin


   /*
    * Active queue -> Main queue
    * SRPT_QUEUE:               6 : 7 : 8 : 0 : 1 : 2 : 3 :
    * write_en/forward_en = 0:  6 : 7 : 8 | 0 : 1 | 2 : 3 |       {(srpt_even)}
    *                           6 : 7 : 8 : 0 | 1 : 2 | 3 :       {(srpt_queue[0]), (srpt_odd)}
    * write_en = 1:             6 : 7 : 8 | N : 0 | 1 : 2 |       {(srpt_odd)}
    * forward_en = 1:           6 : 7 | 8 : 0 | 1 : 2 | 3 :       {(forward), (srpt_odd)}
    * write_en/forward_en = 1:  6 : 7 | 8 : (N) : 0 | 1 : 2 | 3 : {(write), (forward) (srpt_odd)}
   */
   
         if (write_en == 1'b0 && forward_en == 1'b0) begin
            if (swap_type == 1'b0) begin
               // Assumes that write does not keep data around
               for (entry = 0; entry < MAX_SRPT; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_even[entry+1];
               end
              
               swap_type <= 1'b1;
            end else begin
               for (entry = 1; entry < MAX_SRPT; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_odd[entry-1];
               end
               swap_type <= 1'b0;
            end
         end else if (write_en == 1'b1 && forward_en == 1'b0) begin
            for (entry = 0; entry < MAX_SRPT; entry=entry+1) begin
               srpt_queue[entry] <= srpt_odd[entry];
            end
         end else if (write_en == 1'b0 && forward_en == 1'b1) begin
            srpt_queue[0] = forward;
            for (entry = 1; entry < MAX_SRPT; entry=entry+1) begin
               srpt_queue[entry] <= srpt_odd[entry-1];
            end
         end else if (write_en == 1'b1 && forward_en == 1'b1) begin
            srpt_queue[0] = write;
            srpt_queue[1] = forward;
            for (entry = 2; entry < MAX_SRPT; entry=entry+1) begin
               srpt_queue[entry] <= srpt_odd[entry-1];
            end
         end
      end
   end // always @ (posedge ap_clk)

endmodule

/**
 * TODO this needs to handle OOO packets (how does that effect SRPT entry creation?).
 * TODO this needs to consider duplicate packets (can't just increment recv bytes?).
 * TODO need upper bound on number of grantable bytes at any given point
 * The packet map core may need to be involved somehow
 */
module srpt_grant_pkts #(parameter MAX_OVERCOMMIT = 8,
                         parameter MAX_OVERCOMMIT_LOG2 = 3)
   (input ap_clk, ap_rst, ap_ce, ap_start, ap_continue,
    input                        header_in_empty_i,
    output reg                   header_in_read_en_o,
    input [`HEADER_SIZE-1:0]     header_in_data_i,
    input                        grant_pkt_full_o,
    output reg                   grant_pkt_write_en_o,
    output reg [`ENTRY_SIZE-1:0] grant_pkt_data_o, 
    output                       ap_idle, ap_done, ap_ready);

   reg                           main_queue_en;
   reg                           active_queue_en;

   reg                           forward_en;

   wire [`ENTRY_SIZE-1:0]        main_queue_read;
   reg [`ENTRY_SIZE-1:0]         main_queue_write;
   reg [`ENTRY_SIZE-1:0]         active_queue_write; 
   wire [`ENTRY_SIZE-1:0]        active_queue_forward; 

   /* Main queue */
   srpt_queue main_queue(.ap_clk(ap_clk), 
                         .ap_rst(ap_rst), 
                         .ap_ce(ap_ce), 
                         .ap_start(ap_start), 
                         .write(main_queue_write),
                         .write_en(main_queue_we),
                         .forward_en(forward_en),
                         .read(main_queue_read));

   /* Active entries */
   reg [`ENTRY_SIZE-1:0]         active_queue[MAX_OVERCOMMIT-1:0];

   reg                           swap_type;

   assign active_queue_forward = active_queue[MAX_OVERCOMMIT-1];

   // These are reg types and NOT actual registers
   reg [MAX_OVERCOMMIT_LOG2-1:0] peer_match;   // Does the incoming header match on an entry in active set
   reg [MAX_OVERCOMMIT_LOG2-1:0] rpc_match;    // Does the incoming header match on an entry in active set
   // reg [MAX_OVERCOMMIT_LOG2-1:0] next_active;  // Active set entries to be filled
   // reg [MAX_OVERCOMMIT_LOG2-1:0] clear_active; // Active set entries to be cleared

   reg                           peer_match_en;
   reg                           rpc_match_en;
   // reg                           next_active_en;
   //reg                           clear_active_en;

   integer                       i;

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
      // next_active = {MAX_OVERCOMMIT_LOG2{1'b1}};
      // clear_active = {MAX_OVERCOMMIT_LOG2{1'b1}};
      
      peer_match_en  = 1'b0;
      rpc_match_en   = 1'b0;
      // next_active_en = 1'b0;
      // clear_active_en = 1'b0;
      
      // Begin with the assumption we will not write to SRPT queues
      main_queue_we = 1'b0;
      active_queue_we = 1'b0;
      
      // Check every entry in the active set
      for (i = 0; i < MAX_OVERCOMMIT; i=i+1) begin
         // Does the new header have the same peer as the entry in the active set
         if (header_in_data_i[`HDR_PEER_ID] == active_queue[i][`PEER_ID] && peer_match_en == 1'b0) begin
            peer_match = i[MAX_OVERCOMMIT_LOG2-1:0];
            peer_match_en = 1'b1;
         end else begin
            peer_match = peer_match;
         end 
         
         // Does the new header have the same RPC ID as the entry in the active set
         if (header_in_data_i[`HDR_RPC_ID] == active_queue[i][`RPC_ID] && rpc_match_en == 1'b0) begin
            rpc_match = i[MAX_OVERCOMMIT_LOG2-1:0];
            rpc_match_en = 1'b1;
         end else begin
            rpc_match = rpc_match;
         end
         
         // Is the entry in the active set empty, and so, should it be filled and a GRANT packet sent?
         // if (active_set[i][`PRIORITY] == `SRPT_EMPTY && read[i][`PRIORITY] == `SRPT_ACTIVE && next_active_en == 1'b0) begin
         //    next_active = i[MAX_OVERCOMMIT_LOG2-1:0];
         //    next_active_en = 1'b1;
         // end else begin
         //    next_active = next_active;
         // end
         
      end // for (i = 0; i < MAX_OVERCOMMIT; i=i+1)

      // Is there new incoming DATA packets we should process?
      if (!header_in_empty_i) begin
         // If the message length is less than or equal to the incoming bytes, we would have nothing to do here
         if (header_in_data_i[`HDR_MSG_LEN] > header_in_data_i[`HDR_INCOMING]) begin
            
            // It is the first unscheduled packet that creates the entry in the SRPT queue.
            if (header_in_data_i[`HDR_OFFSET] == 0) begin
               
               // Is the new header's peer not one of the active entries
               if (peer_match != {MAX_OVERCOMMIT_LOG2{1'b1}}) begin

                  // Insert the entry into the primary queue
                  main_queue_we = {header_in_data_i[`HDR_PEER_ID], 
                                   header_in_data_i[`HDR_RPC_ID],
                                   1,                                  // recieved packets
                                   header_in_data_i[`HDR_MSG_LEN] - 1, // grantable packets
                                   `SRPT_ACTIVE};

                  main_queue_we = 1;
               // The header's peer is one of the active entries
               end else begin // if (peer_match != {MAX_OVERCOMMIT_LOG2{1'b1}})
                  
                  // Is the grantable packets of the new RPC better than that of the active entry?
                  if ((header_in_data_i[`HDR_MSG_LEN]-1) < srpt_active[peer_match][`GRANTABLE]) begin

                     /* 
                      * We effectively want to implement a swap operation here
                      * between the new (better) entry and the previous entry
                      * that matched on the same peer. To do this, we just
                      * mark the old entry as blocked and submit the new entry
                      * to the head of the active queue
                      */
                     srpt_active_block[peer_match] = `SRPT_BLOCKED;

                     active_queue_write = {header_in_data_i[`HDR_PEER_ID],
                                           header_in_data_i[`HDR_RPC_ID],
                                           1, // received packets
                                           0, // Unused
                                           `SRPT_ACTIVE};

                     active_queue_we = 1'b1;
                    
                  // The grantable packets of the new RPC is worse than that of the active entry
                  end else begin
                     // Insert the new RPC into the big queue
                     main_queue_write = {header_in_data_i[`HDR_PEER_ID],
                                         header_in_data_i[`HDR_RPC_ID],
                                         1,                                  // received packets
                                         header_in_data_i[`HDR_MSG_LEN] - 1, // grantable packets
                                         `SRPT_BLOCKED};

                     main_queue_we = 1'b1;
                  end

               end // else: !if(peer_match != {MAX_OVERCOMMIT{1'b1}})
               
            // A general DATA packet, not the first packet of the unscheduled bytes (which creates the original SRPT entry)
            end else begin // if (header_in_data_i[HDR_OFFSET] == 0)
               /*
                * When an entry is encountered in the queue that matches on
                * peer and rpc, it will update the recv packets value of that
                * entry. 
                */
               active_write = {header_in_data_i[`HDR_PEER_ID],
                               header_in_data_i[`HDR_RPC_ID],
                               1, // received packets
                               0, // Unused
                               `SRPT_UPDATE};

               active_queue_we = 1'b1;
            end
         end // if (header_in_data_i[HDR_MSG_LEN] <= header_in_data_i[HDR_INCOMING])

      // Is there room on our output FIFO for data, and do we have space to send another grant?
      end 
      // TODO this is handled at posedge
      // else if (!grant_pkt_full_o && next_active_en == 1'b1) begin // if (!header_in_empty_i)
         /* 
          * Does this packet have non-zero recv packets and is active
          * If so then send out a grant packet, zero recv bytes, update
          * grantable to be minus the previous recv bytes value
          * If the new grantable value is 0, convert to an UNBLOCK
          */
      // Need to compute swap results, but handled at posedge
      // end else begin
         /*
          * SRPT the queues together, if the bottom entry of the big queue is
          * better than the tail of the small queue, and active then 
          * swap, but also push a block request for the peer that we 
          * just loaded into the active queue.
          */
      //end
   end

   integer rst_entry;

   always @(posedge ap_clk) begin
      if (ap_rst) begin
         grant_pkt_data_o <= {`ENTRY_SIZE{1'b0}};
         grant_pkt_write_en_o <= 1'b0;
         header_in_read_en_o <= 1'b0;

         swap_type = 1'b0;
         for (rst_entry = 0; rst_entry < MAX_SRPT; rst_entry=rst_entry+1) begin
            active_queue[rst_entry][`ENTRY_SIZE-1:0] <= {{(`ENTRY_SIZE-3){1'b0}}, `SRPT_EMPTY};
         end

      end else if (ap_ce && ap_start) begin // if (ap_rst)
      // Is there data on the input stream, and is there room on the output stream
      //if (!header_in_empty_i) begin
      //   // Read the data into the first stage, and acknowledge the data was read
      //   header_in_read_en_o <= 1;

      //   if (clear_active_en) begin
      //      active_set[clear_active][`PRIORITY] = `SRPT_EMPTY;
      //   end

      //   // We did NOT write a grant packet header this cycle
      //   grant_pkt_write_en_o <= 0;
      //   // Is there an open active set entry, does that entry's queue have a value ready
      //end else if (!grant_pkt_full_o && next_active_en == 1'b1) begin // if (!header_in_empty_i)
      //   grant_pkt_data_o <= read[next_active];
      //   active_set[next_active] <= read[next_active];
      //   grant_pkt_write_en_o <= 1;

      //   // We did NOT read a input header this cycle
      //   header_in_read_en_o <= 0;
      //end
      end // if (ap_ce && ap_start)
   end // always @ (posedge ap_clk)

   assign ap_ready = 1;
   assign ap_idle = 0;
   assign ap_done = 1;

endmodule // srpt_grant_queue
