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

`define CORE_IDLE 0
`define CORE_NEW_HDR 1
`define CORE_QUEUE_SWAP 2
`define CORE_GRANT 3

// 60K byte (RTT_Bytes) * 8 (MAX_OVERCOMMIT)
`define RTT_BYTES 60000
`define OVERCOMMIT_BYTES 480000
// `define OVERCOMMIT_BITS 19
`define HOMA_PAYLOAD_SIZE 32'h56a

/*
 * TODO this needs to handle OOO packets (how does that effect SRPT entry creation?).
 * TODO this needs to consider duplicate packets (can't just increment recv bytes?).
 * TODO need upper bound on number of grantable bytes at any given point
 * TODO rtt bytes is hardcoded into this design
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

   reg [MAX_OVERCOMMIT_LOG2-1:0] peer_match_latch;   // Does the incoming header match on an entry in active set
   reg [MAX_OVERCOMMIT_LOG2-1:0] rpc_match_latch;    // Does the incoming header match on an entry in active set
   reg [MAX_OVERCOMMIT_LOG2-1:0] ready_match_latch;    // Does the incoming header match on an entry in active set

   reg                           peer_match_en_latch;
   reg                           rpc_match_en_latch;
   reg                           ready_match_en_latch;

   reg [`HEADER_SIZE-1:0]        header_in_data_latch;

   reg [31:0]    avail_pkts;

   reg [1:0]                     state; 

   integer                       entry;

   always @* begin

      // Init all values so that no latches are inferred. All control paths assign a value.
      peer_match    = {MAX_OVERCOMMIT_LOG2{1'b1}};
      rpc_match     = {MAX_OVERCOMMIT_LOG2{1'b1}};
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
         if (srpt_queue[entry][`PRIORITY] == `SRPT_ACTIVE 
            && srpt_queue[entry][`RECV_BYTES] != 0 
            && ready_match_en == 1'b0
            && avail_pkts != 0) begin
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
            : (srpt_queue[entry-1][`GRANTABLE_BYTES] > srpt_queue[entry][`GRANTABLE_BYTES])) begin
            srpt_odd[entry-1] = srpt_queue[entry];
            srpt_odd[entry] = srpt_queue[entry-1];
            
            if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
               (srpt_queue[entry-1][`PRIORITY] == `SRPT_BLOCK)) begin

               srpt_odd[entry-1][`PRIORITY] = `SRPT_BLOCKED;

            end else if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
                        (srpt_queue[entry-1][`PRIORITY] == `SRPT_UNBLOCK)) begin

               srpt_odd[entry-1][`PRIORITY] = `SRPT_ACTIVE;
            end else if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
                        (srpt_queue[entry-1][`RPC_ID] == srpt_queue[entry][`RPC_ID]) && 
                        (srpt_queue[entry-1][`PRIORITY] == `SRPT_UPDATE)) begin

               srpt_odd[entry-1][`GRANTABLE_BYTES] = srpt_queue[entry-1][`GRANTABLE_BYTES] - `HOMA_PAYLOAD_SIZE;
               srpt_odd[entry-1][`RECV_BYTES] = srpt_queue[entry-1][`RECV_BYTES] + `HOMA_PAYLOAD_SIZE;

            end 

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
            : (srpt_queue[entry-1][`GRANTABLE_BYTES] > srpt_queue[entry][`GRANTABLE_BYTES])) begin
            srpt_even[entry]   = srpt_queue[entry-1];
            srpt_even[entry-1] = srpt_queue[entry];

            if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
               (srpt_queue[entry-1][`PRIORITY] == `SRPT_BLOCK)) begin

               srpt_even[entry-1][`PRIORITY] = `SRPT_BLOCKED;

            end else if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
                        (srpt_queue[entry-1][`PRIORITY] == `SRPT_UNBLOCK)) begin

               srpt_even[entry-1][`PRIORITY] = `SRPT_ACTIVE;
            end else if ((srpt_queue[entry-1][`PEER_ID] == srpt_queue[entry][`PEER_ID]) && 
                        (srpt_queue[entry-1][`RPC_ID] == srpt_queue[entry][`RPC_ID]) && 
                        (srpt_queue[entry-1][`PRIORITY] == `SRPT_UPDATE)) begin

               srpt_even[entry-1][`GRANTABLE_BYTES] = srpt_queue[entry-1][`GRANTABLE_BYTES] - `HOMA_PAYLOAD_SIZE;
               srpt_even[entry-1][`RECV_BYTES] = srpt_queue[entry-1][`RECV_BYTES] + `HOMA_PAYLOAD_SIZE;
            end 

         end else begin 
            srpt_even[entry]   = srpt_queue[entry];
            srpt_even[entry-1] = srpt_queue[entry-1];
         end 
      end
   end

   integer rst_entry;

   always @(posedge ap_clk) begin

      if (ap_rst) begin
         grant_pkt_data_o     <= {`ENTRY_SIZE{1'b0}};
         grant_pkt_write_en_o <= 1'b0;
         header_in_read_en_o  <= 1'b0;
         state                <= `CORE_IDLE;
         swap_type            <= 1'b0;

         peer_match_latch     <= 0;
         rpc_match_latch      <= 0;
         ready_match_latch    <= 0;

         peer_match_en_latch  <= 0;
         rpc_match_en_latch   <= 0;
         ready_match_en_latch <= 0;

         header_in_data_latch <= 0;

         avail_pkts           <= `OVERCOMMIT_BYTES;

         for (rst_entry = 0; rst_entry < MAX_SRPT; rst_entry=rst_entry+1) begin
            srpt_queue[rst_entry][`ENTRY_SIZE-1:0] <= {{(`ENTRY_SIZE-3){1'b0}}, `SRPT_EMPTY};
         end
      end else if (ap_ce && ap_start) begin

         // Latch the last match values
         peer_match_latch <= peer_match;
         rpc_match_latch <= rpc_match;
         ready_match_latch <= ready_match;

         peer_match_en_latch <= peer_match_en;
         rpc_match_en_latch <= rpc_match_en;
         ready_match_en_latch <= ready_match_en;

         // Latch the last header data input
         header_in_data_latch <= header_in_data_i;

         case (state) 
            `CORE_IDLE: begin
               if (header_in_empty_i) begin

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

                  // TODO extra time to SRPT on the lower entries

               // Are there new grants we should send
               end else if (grant_pkt_full_o && ready_match_en == 1'b1) begin 

                  state <= `CORE_GRANT;

                  // We did not read the data 
                  header_in_read_en_o <= 0;

                  // We did not write a grant packet
                  grant_pkt_write_en_o <= 0;

                  // TODO Can still SRPT on the top

               // Do we need to send any block messages 
               end else if (((srpt_queue[MAX_OVERCOMMIT-1][`PRIORITY] != srpt_queue[MAX_OVERCOMMIT][`PRIORITY]) 
                            ? (srpt_queue[MAX_OVERCOMMIT-1][`PRIORITY] < srpt_queue[MAX_OVERCOMMIT][`PRIORITY]) 
                            : (srpt_queue[MAX_OVERCOMMIT-1][`GRANTABLE_BYTES] > srpt_queue[MAX_OVERCOMMIT][`GRANTABLE_BYTES]))
                           && (srpt_queue[MAX_OVERCOMMIT][`PRIORITY] == `SRPT_ACTIVE)) begin

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

            `CORE_GRANT: begin
               state <= `CORE_IDLE;

               // We did not read the data 
               header_in_read_en_o <= 0;

               // We did write a grant packet
               grant_pkt_write_en_o <= 1;

               /*
                * If we have reached this state:
                *   1) There are some recv pkts 
                *   2) There are some avail pkts
                */

               /* verilator lint_off WIDTH */

               // Do we want to send more packets than 8 * MAX_OVERCOMMIT PKTS?
               if (srpt_queue[ready_match_latch][`RECV_BYTES] > avail_pkts) begin
                  // Just send avail_pkts of data
                  // grant_pkt_data_o <= {srpt_queue[ready_match_latch][`PEER_ID],
                  //                      srpt_queue[ready_match_latch][`RPC_ID],
                  //                      avail_pkts,
                  //                      32'b0,
                  //                      3'b0};
            
                  grant_pkt_data_o[`PEER_ID]         <= srpt_queue[ready_match_latch][`PEER_ID];
                  grant_pkt_data_o[`RPC_ID]          <= srpt_queue[ready_match_latch][`RPC_ID];
                  grant_pkt_data_o[`RECV_BYTES]      <= 32'b0;
                  grant_pkt_data_o[`GRANTABLE_BYTES] <= avail_pkts;
                  grant_pkt_data_o[`PRIORITY]        <= 3'b0;

                  srpt_queue[ready_match_latch][`RECV_BYTES]      <= srpt_queue[ready_match_latch][`RECV_BYTES] - avail_pkts;
                  srpt_queue[ready_match_latch][`GRANTABLE_BYTES] <= srpt_queue[ready_match_latch][`GRANTABLE_BYTES] - avail_pkts;

                  avail_pkts <= 0;  
               end else begin 
                  // Is this going to result in a fully granted message?
                  if ((srpt_queue[ready_match_latch][`GRANTABLE_BYTES] - srpt_queue[ready_match_latch][`RECV_BYTES]) == 0) begin
                     srpt_queue[ready_match_latch][`PRIORITY] <= `SRPT_EMPTY;
                  end

                  srpt_queue[ready_match_latch][`RECV_BYTES]      <= 0;
                  srpt_queue[ready_match_latch][`GRANTABLE_BYTES] <= srpt_queue[ready_match_latch][`GRANTABLE_BYTES] - srpt_queue[ready_match_latch][`RECV_BYTES];

                  //grant_pkt_data_o <= {srpt_queue[ready_match_latch][`PEER_ID],
                  //                     srpt_queue[ready_match_latch][`RPC_ID],
                  //                     srpt_queue[ready_match_latch][`RECV_BYTES],
                  //                     32'b0,
                  //                     3'b0};
                  
                  grant_pkt_data_o[`PEER_ID]         <= srpt_queue[ready_match_latch][`PEER_ID];
                  grant_pkt_data_o[`RPC_ID]          <= srpt_queue[ready_match_latch][`RPC_ID];
                  grant_pkt_data_o[`RECV_BYTES]      <= 32'b0;
                  grant_pkt_data_o[`GRANTABLE_BYTES] <= srpt_queue[ready_match_latch][`RECV_BYTES];
                  grant_pkt_data_o[`PRIORITY]       <= 3'b0;

               end
               /* verilator lint_on WIDTH */
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

               srpt_queue[MAX_OVERCOMMIT][`PEER_ID]         <= srpt_queue[MAX_OVERCOMMIT-1][`PEER_ID];
               srpt_queue[MAX_OVERCOMMIT][`RPC_ID]          <= srpt_queue[MAX_OVERCOMMIT-1][`RPC_ID];
               srpt_queue[MAX_OVERCOMMIT][`RECV_BYTES]      <= 32'b0;
               srpt_queue[MAX_OVERCOMMIT][`GRANTABLE_BYTES] <= 32'b0;
               srpt_queue[MAX_OVERCOMMIT][`PRIORITY]        <= `SRPT_BLOCK;
               //srpt_queue[MAX_OVERCOMMIT] <= {srpt_queue[MAX_OVERCOMMIT-1][`PEER_ID], 
               //   srpt_queue[MAX_OVERCOMMIT-1][`RPC_ID],
               //   32'b0,// recieved packets
               //   32'b0, // grantable packets
               //   `SRPT_BLOCK};

            end

            `CORE_NEW_HDR: begin

               // It is the first unscheduled packet that creates the entry in the SRPT queue.
               if (header_in_data_latch[`HDR_OFFSET] == 0) begin
                  // Is the new header's peer one of the active entries?
                  if (peer_match_en_latch) begin

                     // Is the grantable packets of the new RPC better than that of the active entry?
                     
                     /* verilator lint_off WIDTH */
                     if ((header_in_data_latch[`HDR_MSG_LEN]-`HOMA_PAYLOAD_SIZE) < srpt_queue[peer_match_latch][`GRANTABLE_BYTES]) begin
                        srpt_queue[peer_match_latch][`PRIORITY] <= `SRPT_BLOCKED;

                        /* verilator lint_on WIDTH */
                        srpt_queue[MAX_OVERCOMMIT][`PEER_ID]         <= header_in_data_latch[`HDR_PEER_ID];
                        srpt_queue[MAX_OVERCOMMIT][`RPC_ID]          <= header_in_data_latch[`HDR_RPC_ID];
                        srpt_queue[MAX_OVERCOMMIT][`RECV_BYTES]      <= `HOMA_PAYLOAD_SIZE;
                        srpt_queue[MAX_OVERCOMMIT][`GRANTABLE_BYTES] <= header_in_data_latch[`HDR_MSG_LEN] - `HOMA_PAYLOAD_SIZE;
                        srpt_queue[MAX_OVERCOMMIT][`PRIORITY]        <= `SRPT_ACTIVE;

                        // srpt_queue[MAX_OVERCOMMIT] <= {header_in_data_latch[`HDR_PEER_ID], 
                        //                                header_in_data_latch[`HDR_RPC_ID],
                        //                                `HOMA_PAYLOAD_SIZE,
                        //                                header_in_data_latch[`HDR_MSG_LEN] - `HOMA_PAYLOAD_SIZE,
                        //                                `SRPT_ACTIVE};

                     // The grantable packets of the new RPC is worse than that of the active entry
                     end else begin
                        // Insert the new RPC into the big queue
                        srpt_queue[MAX_OVERCOMMIT][`PEER_ID]         <= header_in_data_latch[`HDR_PEER_ID];
                        srpt_queue[MAX_OVERCOMMIT][`RPC_ID]          <= header_in_data_latch[`HDR_RPC_ID];
                        srpt_queue[MAX_OVERCOMMIT][`RECV_BYTES]      <= `HOMA_PAYLOAD_SIZE;
                        srpt_queue[MAX_OVERCOMMIT][`GRANTABLE_BYTES] <= header_in_data_latch[`HDR_MSG_LEN] - `HOMA_PAYLOAD_SIZE;
                        srpt_queue[MAX_OVERCOMMIT][`PRIORITY]        <= `SRPT_BLOCKED;

                        //srpt_queue[MAX_OVERCOMMIT] <= {header_in_data_latch[`HDR_PEER_ID], 
                        //   header_in_data_latch[`HDR_RPC_ID],
                        //   `HOMA_PAYLOAD_SIZE,                         
                        //   header_in_data_latch[`HDR_MSG_LEN] - `HOMA_PAYLOAD_SIZE,
                        //   `SRPT_BLOCKED};
                     end

                  // The header's peer is not one of the active entries
                  end else begin 
                     // Insert the new RPC into the big queue
                     srpt_queue[MAX_OVERCOMMIT][`PEER_ID]         <= header_in_data_latch[`HDR_PEER_ID];
                     srpt_queue[MAX_OVERCOMMIT][`RPC_ID]          <= header_in_data_latch[`HDR_RPC_ID];
                     srpt_queue[MAX_OVERCOMMIT][`RECV_BYTES]      <= `HOMA_PAYLOAD_SIZE;
                     srpt_queue[MAX_OVERCOMMIT][`GRANTABLE_BYTES] <= header_in_data_latch[`HDR_MSG_LEN] - `HOMA_PAYLOAD_SIZE;
                     srpt_queue[MAX_OVERCOMMIT][`PRIORITY]        <= `SRPT_ACTIVE;

                     //srpt_queue[MAX_OVERCOMMIT] <= {header_in_data_latch[`HDR_PEER_ID], 
                     //   header_in_data_latch[`HDR_RPC_ID],
                     //   `HOMA_PAYLOAD_SIZE,                        
                     //   header_in_data_latch[`HDR_MSG_LEN] - `HOMA_PAYLOAD_SIZE,
                     //   `SRPT_ACTIVE};
                  end

               // A general DATA packet, not the first packet of the unscheduled bytes (which creates the original SRPT entry)
               end else begin
                  /*
                   * When an entry is encountered in the queue that matches on
                   * peer and rpc, it will update the recv packets value of that
                   * entry. 
                   */
                  if (rpc_match_en_latch == 1'b1) begin

                     /* verilator lint_off WIDTH */
                     srpt_queue[rpc_match_latch][`RECV_BYTES] <= srpt_queue[rpc_match_latch][`RECV_BYTES] + `HOMA_PAYLOAD_SIZE;
                     srpt_queue[rpc_match_latch][`GRANTABLE_BYTES] <= srpt_queue[rpc_match_latch][`GRANTABLE_BYTES] - `HOMA_PAYLOAD_SIZE;
                     /* verilator lint_on WIDTH */

                  end else begin
      
                     // srpt_queue[MAX_OVERCOMMIT] <= {header_in_data_latch[`HDR_PEER_ID], 
                     // header_in_data_latch[`HDR_RPC_ID],
                     // 32'b0,
                     // 32'b0,
                     // `SRPT_UPDATE};

                     srpt_queue[MAX_OVERCOMMIT][`PEER_ID]         <= header_in_data_latch[`HDR_PEER_ID];
                     srpt_queue[MAX_OVERCOMMIT][`RPC_ID]          <= header_in_data_latch[`HDR_RPC_ID];
                     srpt_queue[MAX_OVERCOMMIT][`RECV_BYTES]      <= 32'b0;
                     srpt_queue[MAX_OVERCOMMIT][`GRANTABLE_BYTES] <= 32'b0;
                     srpt_queue[MAX_OVERCOMMIT][`PRIORITY]        <= `SRPT_UPDATE;
                  end
               end
               


               state <= `CORE_IDLE;

               // We did not read the data 
               header_in_read_en_o <= 0;

               // We did not write a grant packet
               grant_pkt_write_en_o <= 0;

               // TODO extra time here to send a grant packet
            end
         endcase
      end
   end 

   assign ap_ready = 1;
   assign ap_idle = 0;
   assign ap_done = 1;

endmodule // srpt_grant_queue
