`timescale 1ns / 1ps

`define SRPT_UPDATE 3'b0
`define SRPT_BLOCK 3'b001
`define SRPT_INVALIDATE 3'b010
`define SRPT_UNBLOCK 3'b011
`define SRPT_EMPTY 3'b100
`define SRPT_BLOCKED 3'b101
`define SRPT_ACTIVE 3'b110

`define ENTRY_SIZE 51
`define PEER_ID 50:37
`define RPC_ID 36:23
`define RECV_PKTS 22:13
`define GRNTBLE_PKTS 12:3
`define PRIORITY 2:0
`define PRIORITY_SIZE 3

`define HEADER_SIZE 58
`define HDR_PEER_ID 57:44
`define HDR_RPC_ID 43:30
`define HDR_MSG_LEN 29:20
`define HDR_INCOMING 19:10
`define HDR_OFFSET 9:0

`define CORE_IDLE 0
`define CORE_NEW_HDR 1
`define CORE_QUEUE_SWAP 2

/*
 * TODO this needs to handle OOO packets (how does that effect SRPT entry creation?).
 * TODO this needs to consider duplicate packets (can't just increment recv bytes?).
 * TODO need upper bound on number of grantable bytes at any given point
 * The packet map core may need to be involved somehow
 */
module srpt_grant_pkts #(parameter MAX_OVERCOMMIT = 8,
                         parameter MAX_OVERCOMMIT_LOG2 = 3,
                         parameter MAX_SRPT = 1024)
     (input ap_clk, ap_rst, ap_ce, ap_start, ap_continue,
      input                        header_in_empty_i,
      output reg                   header_in_read_en_o,
      input [`HEADER_SIZE-1:0]     header_in_data_i,
      input                        grant_pkt_full_o,
      output reg                   grant_pkt_write_en_o,
      output reg [`ENTRY_SIZE-1:0] grant_pkt_data_o, 
      output                       ap_idle, ap_done, ap_ready);

   reg [`ENTRY_SIZE-1:0]         srpt_queue[MAX_SRPT-1:0];

   reg [`ENTRY_SIZE-1:0]         srpt_odd[MAX_SRPT-1:0]; 
   reg [`ENTRY_SIZE-1:0]         srpt_even[MAX_SRPT-1:0]; 

   reg                           swap_type;

   // These are reg types and NOT actual registers
   reg [MAX_OVERCOMMIT_LOG2-1:0] peer_match;   // Does the incoming header match on an entry in active set
   reg [MAX_OVERCOMMIT_LOG2-1:0] rpc_match;    // Does the incoming header match on an entry in active set
   reg [MAX_OVERCOMMIT_LOG2-1:0] ready_match;    // Does the incoming header match on an entry in active set

   reg                           peer_match_en;
   reg                           rpc_match_en;
   reg                           ready_match_en;

   reg [1:0]                     state; 

   integer                       entry;

   always @* begin

      // Init all values so that no latches are inferred. All control paths assign a value.
      peer_match  = {MAX_OVERCOMMIT_LOG2{1'b1}};
      rpc_match   = {MAX_OVERCOMMIT_LOG2{1'b1}};
      ready_match   = {MAX_OVERCOMMIT_LOG2{1'b1}};

      peer_match_en  = 1'b0;
      rpc_match_en   = 1'b0;
      ready_match_en = 1'b0;

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
         if (srpt_queue[entry][`PRIORITY] == `SRPT_ACTIVE && srpt_queue[entry][`RECV_PKTS] != 0 && ready_match_en == 1'b0) begin
            ready_match = entry[MAX_OVERCOMMIT_LOG2-1:0];
            ready_match_en = 1'b1;
         end else begin
            ready_match = ready_match;
         end
      end

      srpt_odd[0] = srpt_queue[0];
      srpt_odd[MAX_SRPT-1] = srpt_queue[MAX_SRPT-1];

      // Compute srpt_odd
      for (entry = 2; entry < MAX_SRPT-1; entry=entry+2) begin
         // If the priority differs or grantable bytes
         if ((srpt_queue[entry-1][`PRIORITY] != srpt_queue[entry][`PRIORITY]) 
            ? (srpt_queue[entry-1][`PRIORITY] < srpt_queue[entry][`PRIORITY]) 
            : (srpt_queue[entry-1][`GRNTBLE_PKTS] > srpt_queue[entry][`GRNTBLE_PKTS])) begin
            srpt_odd[entry-1] = srpt_queue[entry];
            srpt_odd[entry] = srpt_queue[entry-1];
         end else begin 
            srpt_odd[entry] = srpt_queue[entry];
            srpt_odd[entry-1] = srpt_queue[entry-1];
         end 
      end

      // Compute srpt_even
      for (entry = 1; entry < MAX_SRPT; entry=entry+2) begin
         // If the priority differs or grantable bytes
         if ((srpt_queue[entry-1][`PRIORITY] != srpt_queue[entry][`PRIORITY]) 
            ? (srpt_queue[entry-1][`PRIORITY] < srpt_queue[entry][`PRIORITY]) 
            : (srpt_queue[entry-1][`GRNTBLE_PKTS] > srpt_queue[entry][`GRNTBLE_PKTS])) begin
            srpt_even[entry]   = srpt_queue[entry-1];
            srpt_even[entry-1] = srpt_queue[entry];
         end else begin 
            srpt_even[entry]   = srpt_queue[entry];
            srpt_even[entry-1] = srpt_queue[entry-1];
         end 
      end
   end

   integer rst_entry;

   always @(posedge ap_clk) begin
      if (ap_rst) begin
         grant_pkt_data_o <= {`ENTRY_SIZE{1'b0}};
         grant_pkt_write_en_o <= 1'b0;
         header_in_read_en_o <= 1'b0;
         state <= `CORE_IDLE;
         swap_type <= 1'b0;

         for (rst_entry = 0; rst_entry < MAX_SRPT; rst_entry=rst_entry+1) begin
            srpt_queue[rst_entry][`ENTRY_SIZE-1:0] <= {{(`ENTRY_SIZE-3){1'b0}}, `SRPT_EMPTY};
         end
      end else if (ap_ce && ap_start) begin
         case (state) 
            `CORE_IDLE: begin
               if (!header_in_empty_i) begin

                  // If the message length is less than or equal to the incoming bytes, we would have nothing to do here
                  if (header_in_data_i[`HDR_MSG_LEN] > header_in_data_i[`HDR_INCOMING]) begin

                     // It is the first unscheduled packet that creates the entry in the SRPT queue.
                     if (header_in_data_i[`HDR_OFFSET] == 0) begin

                        // Is the new header's peer one of the active entries
                        if (peer_match != {MAX_OVERCOMMIT_LOG2{1'b1}}) begin
                           $display("NEW BLOCKED");
                           // Insert the entry into the primary queue
                           srpt_queue[MAX_OVERCOMMIT] <= {header_in_data_i[`HDR_PEER_ID], 
                              header_in_data_i[`HDR_RPC_ID],
                              {9'b0, 1'b1},                          // recieved packets
                              header_in_data_i[`HDR_MSG_LEN] - 1'b1, // grantable packets
                              `SRPT_BLOCKED};

                        // The header's peer is not one of the active entries
                        end else begin 
                           // Is the grantable packets of the new RPC better than that of the active entry?
                           /* verilator lint_off WIDTH */
                           if (peer_match_en && (header_in_data_i[`HDR_MSG_LEN]-1) < srpt_queue[peer_match][`GRNTBLE_PKTS]) begin
                           /* verilator lint_on WIDTH */

                              $display("BETTER ENTRY");

                              $display(header_in_data_i[`HDR_PEER_ID]);
                             //srpt_active_block[peer_match] = `SRPT_BLOCKED;

                             //main_queue_write = {header_in_data_i[`HDR_PEER_ID], 
                             //                  header_in_data_i[`HDR_RPC_ID],
                             //                  {9'b0, 1'b1},                          // recieved packets
                             //                  header_in_data_i[`HDR_MSG_LEN] - 1'b1, // grantable packets
                             //                  `SRPT_ACTIVE};

                             //main_queue_en = 1'b1;

                             // The grantable packets of the new RPC is worse than that of the active entry
                           end else begin
                              $display("NEW ACTIVE");
                              $display(header_in_data_i[`HDR_PEER_ID]);
                              // Insert the new RPC into the big queue
                              srpt_queue[MAX_OVERCOMMIT] <= {header_in_data_i[`HDR_PEER_ID], 
                                 header_in_data_i[`HDR_RPC_ID],
                                 {9'b0, 1'b1},                          // recieved packets
                                 header_in_data_i[`HDR_MSG_LEN] - 1'b1, // grantable packets
                                 `SRPT_ACTIVE};
                           end
                        end
                     // A general DATA packet, not the first packet of the unscheduled bytes (which creates the original SRPT entry)
                     end else begin
                        /*
                         * When an entry is encountered in the queue that matches on
                         * peer and rpc, it will update the recv packets value of that
                         * entry. 
                         */
                        $display("OTHER DATA PACKET");
                        // TODO maybe best to do this manually on the lower 8 entries and submit to the big queue
                        //active_write = {header_in_data_i[`HDR_PEER_ID],
                        //                header_in_data_i[`HDR_RPC_ID],
                        //                1, // received packets
                        //                0, // Unused
                        //                `SRPT_UPDATE};

                        //active_queue_we = 1'b1;
                     end

                     // Make room for that new entry 
                     for (entry = MAX_OVERCOMMIT+1; entry < MAX_SRPT-1; entry=entry+1) begin
                        srpt_queue[entry] <= srpt_queue[entry-1];
                     end

                     // We need one more cycle to cmp the new entry
                     state <= `CORE_NEW_HDR;

                     // Acknoledge that we read the data 
                     header_in_read_en_o <= 1;

                     // We did not write a grant packet
                     grant_pkt_write_en_o <= 0;


                  end

               // Are there new grants we should send
               end else if (!grant_pkt_full_o && ready_match_en == 1'b1) begin 

                  state <= `CORE_IDLE;

                  // We did not read the data 
                  header_in_read_en_o <= 0;

                  // We did not write a grant packet
                  grant_pkt_write_en_o <= 0;

                  // Send and new grant packet
                  /* verilator lint_off WIDTH */
                  grant_pkt_data_o <= srpt_queue[ready_match];
                  /* verilator lint_on WIDTH */

                  // TODO Can still SRPT on the top and bottom

               // Do we need to send any block operations?
               end else if ((srpt_queue[MAX_OVERCOMMIT-1][`PRIORITY] != srpt_queue[MAX_OVERCOMMIT][`PRIORITY]) 
                              ? (srpt_queue[MAX_OVERCOMMIT-1][`PRIORITY] < srpt_queue[MAX_OVERCOMMIT][`PRIORITY]) 
                              : (srpt_queue[MAX_OVERCOMMIT-1][`GRNTBLE_PKTS] > srpt_queue[MAX_OVERCOMMIT][`GRNTBLE_PKTS])) begin

                  // We need one more cycle to send out the block request
                  state <= `CORE_QUEUE_SWAP;

                  // We did not read the data 
                  header_in_read_en_o <= 0;

                  // We did not write a grant packet
                  grant_pkt_write_en_o <= 0;

                  // Swap in the new best element
                  for (entry = 0; entry < MAX_SRPT; entry=entry+1) begin
                     srpt_queue[entry] <= srpt_odd[entry];
                  end

               // Otherwise just sort the two queues
               end else begin
                  state <= `CORE_IDLE;

                  // We did not read the data 
                  header_in_read_en_o <= 0;

                  // We did not write a grant packet
                  grant_pkt_write_en_o <= 0;

                  if (swap_type == 1'b0) begin
                     // Assumes that write does not keep data around
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

            `CORE_QUEUE_SWAP: begin
               state <= `CORE_IDLE;

               // We did not read the data 
               header_in_read_en_o <= 0;

               // We did not write a grant packet
               grant_pkt_write_en_o <= 0;

               // Make room for that new entry 
               for (entry = MAX_OVERCOMMIT+1; entry < MAX_SRPT-1; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_queue[entry-1];
               end

               srpt_queue[MAX_OVERCOMMIT] <= {srpt_queue[MAX_OVERCOMMIT-1][`PEER_ID], 
                  srpt_queue[MAX_OVERCOMMIT-1][`RPC_ID],
                  10'b0,// recieved packets
                  10'b0, // grantable packets
                  `SRPT_BLOCK};

            end

            `CORE_NEW_HDR: begin
               state <= `CORE_IDLE;

               // We did not read the data 
               header_in_read_en_o <= 0;

               // We did not write a grant packet
               grant_pkt_write_en_o <= 0;

               // SRPT in the new element we just added
               for (entry = 0; entry < MAX_OVERCOMMIT-1; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_odd[entry];
               end

               for (entry = MAX_OVERCOMMIT+1; entry < MAX_SRPT; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_odd[entry];
               end

               // TODO can maybe do extra work here and send a grant packet
            end
         endcase
      end
   end 

   assign ap_ready = 1;
   assign ap_idle = 0;
   assign ap_done = 1;

endmodule // srpt_grant_queue
