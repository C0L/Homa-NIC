`timescale 1ns / 1ps

`define SRPT_INVALIDATE 3'b000
`define SRPT_UPDATE     3'b001
`define SRPT_EMPTY      3'b010
`define SRPT_BLOCKED    3'b011
`define SRPT_ACTIVE     3'b100

`define SRPT_DATA_SIZE      157
`define SRPT_DATA_RPC_ID    15:0
`define SRPT_DATA_DBUFF_ID  25:16
`define SRPT_DATA_REMAINING 45:26
`define SRPT_DATA_GRANTED   77:58
`define SRPT_DATA_DBUFFERED 109:90
`define SRPT_DATA_MSG_LEN   141:122
// `define SRPT_DATA_REMAINING 57:26
// `define SRPT_DATA_GRANTED   89:58
// `define SRPT_DATA_DBUFFERED 121:90
// `define SRPT_DATA_MSG_LEN   153:122
`define SRPT_DATA_PRIORITY  156:154

`define REMAINING_SIZE 20

`define ENTRY_SIZE      69
`define ENTRY_RPC_ID    15:0
`define ENTRY_DBUFF_ID  25:16
`define ENTRY_REMAINING 45:26
`define ENTRY_LIMIT     65:46
`define ENTRY_PRIORITY  68:66

`define DBUFF_SIZE     74
`define DBUFF_DBUFF_ID 9:0
`define DBUFF_MSG_LEN  41:10
`define DBUFF_OFFSET   61:42

`define DBUFF_ID_SIZE  10

`define GRANT_SIZE   42
`define GRANT_DBUFF_ID 9:0
`define GRANT_OFFSET   29:10



`define HOMA_PAYLOAD_SIZE 20'h56a

//  Xilinx Simple Dual Port Single Clock RAM
//  This code implements a parameterizable SDP single clock memory.
//  If a reset or enable is not necessary, it may be tied off or removed from the code.
module xilinx_simple_dual_port_1_clock_ram #(
  parameter RAM_WIDTH = 64,                       // Specify RAM data width
  parameter RAM_DEPTH = 512,                      // Specify RAM depth (number of entries)
  parameter RAM_PERFORMANCE = "HIGH_PERFORMANCE", // Select "HIGH_PERFORMANCE" or "LOW_LATENCY" 
  parameter INIT_FILE = ""                        // Specify name/location of RAM initialization file if using one (leave blank if not)
) (
  input [clogb2(RAM_DEPTH-1)-1:0] addra, // Write address bus, width determined from RAM_DEPTH
  input [clogb2(RAM_DEPTH-1)-1:0] addrb, // Read address bus, width determined from RAM_DEPTH
  input [RAM_WIDTH-1:0] dina,          // RAM input data
  input clka,                          // Clock
  input wea,                           // Write enable
  input enb,                           // Read Enable, for additional power savings, disable when not in use
  input rstb,                          // Output reset (does not affect memory contents)
  input regceb,                        // Output register enable
  output [RAM_WIDTH-1:0] doutb         // RAM output data
);

  reg [RAM_WIDTH-1:0] BRAM [RAM_DEPTH-1:0];
  reg [RAM_WIDTH-1:0] ram_data = {RAM_WIDTH{1'b0}};

  // The following code either initializes the memory values to a specified file or to all zeros to match hardware
  generate
    if (INIT_FILE != "") begin: use_init_file
      initial
        $readmemh(INIT_FILE, BRAM, 0, RAM_DEPTH-1);
    end else begin: init_bram_to_zero
      integer ram_index;
      initial
        for (ram_index = 0; ram_index < RAM_DEPTH; ram_index = ram_index + 1)
          BRAM[ram_index] = {RAM_WIDTH{1'b0}};
    end
  endgenerate

  always @(posedge clka) begin
    if (wea)
      BRAM[addra] <= dina;
    if (enb)
      ram_data <= BRAM[addrb];
  end

  //  The following code generates HIGH_PERFORMANCE (use output register) or LOW_LATENCY (no output register)
  generate
    if (RAM_PERFORMANCE == "LOW_LATENCY") begin: no_output_register

      // The following is a 1 clock cycle read latency at the cost of a longer clock-to-out timing
       assign doutb = ram_data;

    end else begin: output_register

      // The following is a 2 clock cycle read latency with improve clock-to-out timing

      reg [RAM_WIDTH-1:0] doutb_reg = {RAM_WIDTH{1'b0}};

      always @(posedge clka)
        if (rstb)
          doutb_reg <= {RAM_WIDTH{1'b0}};
        else if (regceb)
          doutb_reg <= ram_data;

      assign doutb = doutb_reg;

    end
  endgenerate

  //  The following function calculates the address width based on specified RAM depth
  function integer clogb2;
    input integer depth;
      for (clogb2=0; depth>0; clogb2=clogb2+1)
        depth = depth >> 1;
  endfunction

endmodule


/**
 * srpt_grant_pkts() - Determines which data packet should be transmit next
 * based on the number of remaining bytes to send. Packets however must
 * remain eligible in that they 1) have at least 1 byte of granted data, and
 * 2) the data is availible on chip for their transmission 
 *
 * #MAX_SRPT - The size of the SRPT queue, or the maximum number of RPCs that
 * we could be trying to send to at a single point in time
 *
 * @sendmsg_in_empty_i   - 0 value indicates there are no new sendmsg requsts
 * to be read from this input stream while 1 indictes there are sendmsgs on the
 * input stream.
 * @sendmsg_in_read_en_o - Acknowledgement that the input data has been read
 * @sendmsg_in_data_i    - The actual data content of the incoming stream
 *
 * @grant_in_empty_i   - 0 value indicates there are no new grants to be read
 * from this stream while a 1 value indicates there are grants which should be
 * read
 * @grant_in_read_en_o - Acknowledgement that the input grant has been read
 * and can be discarded by the FIFO
 * @grant_in_data_i    - The actual grant content that needs to be added to
 * this queue.
 *
 * @dbuff_in_empty_i   - 0 value indicates there are no data buff
 * notifications that are availible on the input stream while 1 indicates
 * there are databuffer notifications we should integrate
 * @dbuff_in_read_en_o - Acknowledgement that the data buffer notification has
 * been read from the input stream
 * @dbuff_in_data_i    - The actual notification content of the incoming
 * stream
 *
 * @data_pkt_full_o     - 0 value indicates that the outgoing stream has room
 * for another data packet that should be sent while a 1 value indicates that
 * the stream is full
 * @data_pkt_write_en_o - Acknowledgement that the data in data_pkt_data_o
 * should be added to the outgoing stream
 * @data_pkt_data_o     - The actual outgoing pkts that should be sent onto
 * the link.
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
 */
module srpt_data_pkts #(parameter MAX_SRPT = 1024)
     (input ap_clk, ap_rst, ap_ce, ap_start, ap_continue,
      input                            sendmsg_in_empty_i,
      output reg                       sendmsg_in_read_en_o,
      input [`SRPT_DATA_SIZE-1:0]      sendmsg_in_data_i,
      input                            grant_in_empty_i,
      output reg                       grant_in_read_en_o,
      input [`GRANT_SIZE-1:0]          grant_in_data_i,
      input                            dbuff_in_empty_i,
      output reg                       dbuff_in_read_en_o,
      input [`DBUFF_SIZE-1:0]          dbuff_in_data_i,
      input                            data_pkt_full_i,
      output reg                       data_pkt_write_en_o,
      output reg [`SRPT_DATA_SIZE-1:0] data_pkt_data_o, 
      output                         ap_idle, ap_done, ap_ready);

   reg [`ENTRY_SIZE-1:0]        srpt_queue[MAX_SRPT-1:0];
   reg [`ENTRY_SIZE-1:0]        srpt_odd[MAX_SRPT-1:0]; 
   reg [`ENTRY_SIZE-1:0]        srpt_even[MAX_SRPT-1:0]; 
   reg [`ENTRY_SIZE-1:0]        new_entry;

   reg                          sendmsg_in_empty_i_latch_stage_0, sendmsg_in_empty_i_latch_stage_1;
   reg [`SRPT_DATA_SIZE-1:0]    sendmsg_in_data_i_latch_stage_0, sendmsg_in_data_i_latch_stage_1;

   reg                          grant_in_empty_i_latch_stage_0, grant_in_empty_i_latch_stage_1;
   reg [`GRANT_SIZE-1:0]        grant_in_data_i_latch_stage_0, grant_in_data_i_latch_stage_1;

   reg                          dbuff_in_empty_i_latch_stage_0, dbuff_in_empty_i_latch_stage_1;
   reg [`DBUFF_SIZE-1:0]        dbuff_in_data_i_latch_stage_0, dbuff_in_data_i_latch_stage_1;

   reg                          swap_type;

   wire                         sendmsg_ripe;
   wire                         dbuff_ripe;
   wire                         grant_ripe;
   wire                         bram_ripe;

   reg [`DBUFF_ID_SIZE-1:0]     bram_addra, bram_addrb;
   reg [`SRPT_DATA_SIZE-1:0]    bram_dina;
   wire [`SRPT_DATA_SIZE-1:0]   bram_doutb;
   reg                          bram_wea;
   reg                          bram_enb;

   reg [`DBUFF_ID_SIZE-1:0]     rem_addrb, rem_addra;
   reg [`REMAINING_SIZE-1:0]    rem_dina;
   wire [`REMAINING_SIZE-1:0]   rem_doutb;
   reg                          rem_enb;
   reg                          rem_wea;

   wire [`ENTRY_SIZE-1:0]       head;

   //  Xilinx Simple Dual Port Single Clock RAM
   xilinx_simple_dual_port_1_clock_ram #(
     .RAM_WIDTH(`SRPT_DATA_SIZE),          // Specify RAM data width
     .RAM_DEPTH(1024),                     // Specify RAM depth (number of entries)
     .RAM_PERFORMANCE("HIGH_PERFORMANCE"), // Select "HIGH_PERFORMANCE" or "LOW_LATENCY" 
     .INIT_FILE("")                        // Specify name/location of RAM initialization file if using one (leave blank if not)
   ) entries (
     .addra(bram_addra),   // Write address bus, width determined from RAM_DEPTH
     .addrb(bram_addrb),   // Read address bus, width determined from RAM_DEPTH
     .dina(bram_dina),     // RAM input data, width determined from RAM_WIDTH
     .clka(ap_clk),        // Clock
     .wea(bram_wea),       // Write enable
     .enb(1'b1),	         // Read Enable, for additional power savings, disable when not in use
     .rstb(ap_rst),        // Output reset (does not affect memory contents)
     .regceb(1'b1),        // Output register enable
     .doutb(bram_doutb)    // RAM output data, width determined from RAM_WIDTH
   );

   //  Xilinx Simple Dual Port Single Clock RAM
   xilinx_simple_dual_port_1_clock_ram #(
     .RAM_WIDTH(`REMAINING_SIZE),         // Specify RAM data width
     .RAM_DEPTH(1024),                     // Specify RAM depth (number of entries)
     .RAM_PERFORMANCE("HIGH_PERFORMANCE"), // Select "HIGH_PERFORMANCE" or "LOW_LATENCY" 
     .INIT_FILE("")                        // Specify name/location of RAM initialization file if using one (leave blank if not)
   ) remaining (
     .addra(rem_addra),   // Write address bus, width determined from RAM_DEPTH
     .addrb(rem_addrb),   // Read address bus, width determined from RAM_DEPTH
     .dina(rem_dina),     // RAM input data, width determined from RAM_WIDTH
     .clka(ap_clk),        // Clock
     .wea(rem_wea),       // Write enable
     .enb(1'b1),	         // Read Enable, for additional power savings, disable when not in use
     .rstb(ap_rst),        // Output reset (does not affect memory contents)
     .regceb(1'b1),        // Output register enable
     .doutb(rem_doutb)    // RAM output data, width determined from RAM_WIDTH
   );

   function [0:0] ripeness (input [19:0] granted, buffered, current, length);
      begin
      ripeness = !(((current + 1 > granted) & (length > granted)) 
                  | ((current + `HOMA_PAYLOAD_SIZE > buffered) & (length > buffered)) 
                  | ((current >= length)));
      end
   endfunction

   function [19:0] limit (input [19:0] granted, buffered, length);
      begin
         limit = length - ((granted <= (buffered - `HOMA_PAYLOAD_SIZE)) ? MIN(granted,length) : MIN(buffered,length));
      end
   endfunction

   function [19:0] MIN (input [19:0] p, q);
      begin
         MIN = (p)>(q)?(q):(p);
      end
   endfunction

   assign head = srpt_queue[0];

   assign sendmsg_ripe = ripeness(bram_doutb[`SRPT_DATA_GRANTED],
         sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_DBUFFERED],
         0,
         sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_MSG_LEN]);

   assign bram_ripe = ripeness(bram_doutb[`SRPT_DATA_GRANTED],
         bram_doutb[`SRPT_DATA_DBUFFERED],
         bram_doutb[`SRPT_DATA_MSG_LEN] - rem_doutb,
         bram_doutb[`SRPT_DATA_MSG_LEN]);

   assign dbuff_ripe = ripeness(bram_doutb[`SRPT_DATA_GRANTED],
         dbuff_in_data_i_latch_stage_1[`DBUFF_OFFSET],
         bram_doutb[`SRPT_DATA_MSG_LEN] - rem_doutb,
         bram_doutb[`SRPT_DATA_MSG_LEN]);

   assign grant_ripe = ripeness(grant_in_data_i_latch_stage_1[`GRANT_OFFSET],
         bram_doutb[`SRPT_DATA_DBUFFERED],
         bram_doutb[`SRPT_DATA_MSG_LEN] - rem_doutb,
         bram_doutb[`SRPT_DATA_MSG_LEN]);

   integer entry;

   always @* begin
      sendmsg_in_read_en_o = 0;
      dbuff_in_read_en_o   = 0;
      grant_in_read_en_o   = 0;
      data_pkt_write_en_o  = 0;

      new_entry  = 0;

      bram_addra = 0;
      bram_addrb = 0;
      bram_dina  = 0;
      bram_wea   = 0;

      rem_addra  = 0;
      rem_addrb  = 0;
      rem_dina   = 0;
      rem_wea    = 0;

      data_pkt_data_o[`SRPT_DATA_RPC_ID]    = head[`ENTRY_RPC_ID];
      data_pkt_data_o[`SRPT_DATA_DBUFF_ID]  = head[`ENTRY_DBUFF_ID];
      data_pkt_data_o[`SRPT_DATA_REMAINING] = head[`ENTRY_REMAINING]; 

      if (sendmsg_in_empty_i) begin
         sendmsg_in_read_en_o = 1;
         bram_addrb = sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID];
         rem_addrb  = sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID];
      end else if (dbuff_in_empty_i) begin
         dbuff_in_read_en_o = 1;
         bram_addrb = dbuff_in_data_i[`DBUFF_DBUFF_ID];
         rem_addrb  = dbuff_in_data_i[`DBUFF_DBUFF_ID];
      end else if (grant_in_empty_i) begin
         grant_in_read_en_o = 1;
         bram_addrb = grant_in_data_i[`GRANT_DBUFF_ID];
         rem_addrb  = grant_in_data_i[`GRANT_DBUFF_ID];
      end

           
      if (sendmsg_in_empty_i_latch_stage_1) begin
         new_entry[`ENTRY_LIMIT]    = limit(sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_GRANTED], 
               bram_doutb[`SRPT_DATA_DBUFFERED], 
               sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_MSG_LEN]);
         new_entry[`ENTRY_DBUFF_ID]  = sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_DBUFF_ID];
         new_entry[`ENTRY_RPC_ID]    = sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_RPC_ID];
         new_entry[`ENTRY_REMAINING] = sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_MSG_LEN];
         new_entry[`ENTRY_PRIORITY]  = `SRPT_ACTIVE;

         bram_dina[`SRPT_DATA_RPC_ID]    = bram_doutb[`SRPT_DATA_RPC_ID];
         bram_dina[`SRPT_DATA_DBUFF_ID]  = bram_doutb[`SRPT_DATA_DBUFF_ID];
         bram_dina[`SRPT_DATA_DBUFFERED] = bram_doutb[`SRPT_DATA_DBUFFERED];
         bram_dina[`SRPT_DATA_GRANTED]   = grant_in_data_i_latch_stage_1[`GRANT_OFFSET];
         bram_dina[`SRPT_DATA_MSG_LEN]   = bram_doutb[`SRPT_DATA_MSG_LEN];

         rem_wea    = 1;
         rem_dina   = sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_MSG_LEN];
         rem_addra  = sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_DBUFF_ID];
         bram_addra = sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_DBUFF_ID];
      end else if (dbuff_in_empty_i_latch_stage_1) begin
         new_entry[`ENTRY_LIMIT]     = limit(bram_doutb[`SRPT_DATA_GRANTED], 
               dbuff_in_data_i_latch_stage_1[`DBUFF_OFFSET], 
               bram_doutb[`SRPT_DATA_MSG_LEN]);
         new_entry[`ENTRY_DBUFF_ID]  = bram_doutb[`SRPT_DATA_DBUFF_ID];
         new_entry[`ENTRY_RPC_ID]    = bram_doutb[`SRPT_DATA_RPC_ID];
         new_entry[`ENTRY_REMAINING] = rem_doutb;
         new_entry[`ENTRY_PRIORITY]  = `SRPT_ACTIVE;

         bram_dina[`SRPT_DATA_RPC_ID]    = sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_RPC_ID];
         bram_dina[`SRPT_DATA_DBUFF_ID]  = sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_DBUFF_ID];
         bram_dina[`SRPT_DATA_DBUFFERED] = bram_doutb[`SRPT_DATA_DBUFFERED];
         bram_dina[`SRPT_DATA_GRANTED]   = sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_GRANTED];
         bram_dina[`SRPT_DATA_MSG_LEN]   = sendmsg_in_data_i_latch_stage_1[`SRPT_DATA_MSG_LEN];

         if (dbuff_ripe) begin
            if (bram_ripe) begin
               new_entry[`ENTRY_PRIORITY] = `SRPT_UPDATE; 
            end else begin
               new_entry[`ENTRY_PRIORITY] = `SRPT_ACTIVE; 
            end
         end

         rem_wea    = 1;
         bram_addra = dbuff_in_data_i_latch_stage_1[`DBUFF_DBUFF_ID];
      end else if (grant_in_empty_i_latch_stage_1) begin
         new_entry[`ENTRY_LIMIT]    = limit(grant_in_data_i_latch_stage_1[`GRANT_OFFSET], 
               bram_doutb[`SRPT_DATA_DBUFFERED], 
               bram_doutb[`SRPT_DATA_MSG_LEN]);
         new_entry[`ENTRY_DBUFF_ID] = bram_doutb[`SRPT_DATA_DBUFF_ID];
         new_entry[`ENTRY_RPC_ID]   = bram_doutb[`SRPT_DATA_RPC_ID];
         new_entry[`ENTRY_REMAINING] = rem_doutb;
         new_entry[`ENTRY_PRIORITY]  = `SRPT_ACTIVE;

         bram_dina[`SRPT_DATA_RPC_ID]    = bram_doutb[`SRPT_DATA_RPC_ID];
         bram_dina[`SRPT_DATA_DBUFF_ID]  = bram_doutb[`SRPT_DATA_DBUFF_ID];
         bram_dina[`SRPT_DATA_DBUFFERED] = dbuff_in_data_i_latch_stage_1[`DBUFF_OFFSET];
         bram_dina[`SRPT_DATA_GRANTED]   = bram_doutb[`SRPT_DATA_GRANTED];
         bram_dina[`SRPT_DATA_MSG_LEN]   = bram_doutb[`SRPT_DATA_MSG_LEN];
       
         if (grant_ripe) begin
            if (bram_ripe) begin
               new_entry[`ENTRY_PRIORITY] = `SRPT_UPDATE; 
            end else begin
               new_entry[`ENTRY_PRIORITY] = `SRPT_ACTIVE; 
            end
         end

         bram_addra = grant_in_data_i_latch_stage_1[`GRANT_DBUFF_ID];
      end else if (data_pkt_full_i && head[`ENTRY_PRIORITY] == `SRPT_ACTIVE) begin

         rem_wea  = 1;
         rem_dina = head[`ENTRY_REMAINING];
         rem_addra = head[`ENTRY_DBUFF_ID];

         // Is this RPC complete
         if (head[`ENTRY_REMAINING] <= `HOMA_PAYLOAD_SIZE) begin
            bram_dina = 0;
            bram_wea  = 1'b1;
         end

         bram_addra = head[`ENTRY_DBUFF_ID];
         bram_addra = 0;

         data_pkt_write_en_o = 1;
      end 

      // TODO the commented section below is better but this reduces critical path
      srpt_odd[0] = new_entry;
      srpt_odd[1] = srpt_queue[0];

      // If the priority differs or grantable bytes
      //if ((new_entry[`SRPT_DATA_PRIORITY] != srpt_queue[0][`SRPT_DATA_PRIORITY]) 
      //   ? (new_entry[`SRPT_DATA_PRIORITY] < srpt_queue[0][`SRPT_DATA_PRIORITY]) 
      //   : (new_entry[`SRPT_DATA_REMAINING] > srpt_queue[0][`SRPT_DATA_REMAINING])) begin

      //   srpt_odd[0] = srpt_queue[0];
      //   srpt_odd[1] = new_entry;

      //   if ((new_entry[`SRPT_DATA_DBUFF_ID] == srpt_queue[0][`SRPT_DATA_DBUFF_ID]) && 
      //      (new_entry[`SRPT_DATA_PRIORITY] == `SRPT_UPDATE)) begin
      //      probe = 1;
      //      srpt_odd[0][`SRPT_DATA_GRANTED]   = new_entry[`SRPT_DATA_GRANTED];
      //      srpt_odd[0][`SRPT_DATA_DBUFFERED] = new_entry[`SRPT_DATA_DBUFFERED];
      //   end 
      //end else begin 
      //   srpt_odd[1] = srpt_queue[0];
      //   srpt_odd[0] = new_entry;
      //end 

      // Compute srpt_odd
      for (entry = 2; entry < MAX_SRPT-1; entry=entry+2) begin
         // If the priority differs or grantable bytes
         if ((srpt_queue[entry-1][`ENTRY_PRIORITY] != srpt_queue[entry][`ENTRY_PRIORITY]) 
            ? (srpt_queue[entry-1][`ENTRY_PRIORITY] < srpt_queue[entry][`ENTRY_PRIORITY]) 
            : (srpt_queue[entry-1][`ENTRY_REMAINING] > srpt_queue[entry][`ENTRY_REMAINING])) begin

            srpt_odd[entry+1] = srpt_queue[entry-1];
            srpt_odd[entry]   = srpt_queue[entry];

            if ((srpt_queue[entry-1][`ENTRY_DBUFF_ID] == srpt_queue[entry][`ENTRY_DBUFF_ID]) && 
               (srpt_queue[entry-1][`ENTRY_PRIORITY] == `SRPT_UPDATE)) begin
               srpt_odd[entry][`ENTRY_LIMIT]   = srpt_queue[entry-1][`ENTRY_LIMIT];
            end 

         end else begin 
            srpt_odd[entry+1] = srpt_queue[entry];
            srpt_odd[entry]   = srpt_queue[entry-1];
         end 
      end

      // Compute srpt_even
      for (entry = 1; entry < MAX_SRPT; entry=entry+2) begin
         // If the priority differs or grantable bytes
         if ((srpt_queue[entry-1][`ENTRY_PRIORITY] != srpt_queue[entry][`ENTRY_PRIORITY]) 
            ? (srpt_queue[entry-1][`ENTRY_PRIORITY] < srpt_queue[entry][`ENTRY_PRIORITY]) 
            : (srpt_queue[entry-1][`ENTRY_REMAINING] > srpt_queue[entry][`ENTRY_REMAINING])) begin

            srpt_even[entry]   = srpt_queue[entry-1];
            srpt_even[entry-1] = srpt_queue[entry];

            if ((srpt_queue[entry-1][`ENTRY_DBUFF_ID] == srpt_queue[entry][`ENTRY_DBUFF_ID]) && 
               (srpt_queue[entry-1][`ENTRY_PRIORITY] == `SRPT_UPDATE)) begin
               srpt_even[entry-1][`ENTRY_LIMIT]   = srpt_queue[entry][`ENTRY_LIMIT];
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
         sendmsg_in_empty_i_latch_stage_0 <= 0; 
         sendmsg_in_data_i_latch_stage_0  <= 0;
         
         sendmsg_in_empty_i_latch_stage_1 <= 0; 
         sendmsg_in_data_i_latch_stage_1  <= 0;

         grant_in_empty_i_latch_stage_0   <= 0;
         grant_in_data_i_latch_stage_0    <= 0;

         grant_in_empty_i_latch_stage_1   <= 0;
         grant_in_data_i_latch_stage_1    <= 0;

         dbuff_in_empty_i_latch_stage_0   <= 0;
         dbuff_in_data_i_latch_stage_0    <= 0;

         dbuff_in_empty_i_latch_stage_1   <= 0;
         dbuff_in_data_i_latch_stage_1    <= 0;
 
         swap_type                        <= 0;

         for (rst_entry = 0; rst_entry < MAX_SRPT; rst_entry=rst_entry+1) begin
            srpt_queue[rst_entry][`ENTRY_RPC_ID]    <= 0;
            srpt_queue[rst_entry][`ENTRY_DBUFF_ID]  <= 0;
            srpt_queue[rst_entry][`ENTRY_REMAINING] <= 0; 
            srpt_queue[rst_entry][`ENTRY_LIMIT] <= 0; 
            srpt_queue[rst_entry][`ENTRY_PRIORITY]  <= `SRPT_EMPTY;
         end
      end else if (ap_ce && ap_start) begin

         sendmsg_in_empty_i_latch_stage_1 <= sendmsg_in_empty_i_latch_stage_0;
         sendmsg_in_data_i_latch_stage_1  <= sendmsg_in_data_i_latch_stage_0;

         dbuff_in_empty_i_latch_stage_1   <= dbuff_in_empty_i_latch_stage_0;
         dbuff_in_data_i_latch_stage_1    <= dbuff_in_data_i_latch_stage_0;

         grant_in_empty_i_latch_stage_1   <= grant_in_empty_i_latch_stage_0;
         grant_in_data_i_latch_stage_1    <= grant_in_data_i_latch_stage_0;
      
         if (sendmsg_in_empty_i) begin
            sendmsg_in_empty_i_latch_stage_0 <= sendmsg_in_empty_i;
            sendmsg_in_data_i_latch_stage_0 <= sendmsg_in_data_i;
         end else if (dbuff_in_empty_i) begin
            dbuff_in_empty_i_latch_stage_0 <= dbuff_in_empty_i;
            dbuff_in_data_i_latch_stage_0  <= dbuff_in_data_i;
         end else if (grant_in_empty_i) begin
            grant_in_empty_i_latch_stage_0 <= grant_in_empty_i;
            grant_in_data_i_latch_stage_0  <= grant_in_data_i;
         end else begin
            sendmsg_in_empty_i_latch_stage_0 <= 0;
            sendmsg_in_data_i_latch_stage_0  <= 0;

            dbuff_in_empty_i_latch_stage_0   <= 0;
            dbuff_in_data_i_latch_stage_0    <= 0;

            grant_in_empty_i_latch_stage_0   <= 0;
            grant_in_data_i_latch_stage_0    <= 0;
         end

         // Was there a sendmsg request that now has BRAM data availible?
         if (sendmsg_in_empty_i_latch_stage_1) begin
            /* An entry begins as active if:
             *   1) There are remaining bytes to send
             *   2) There is at least 1 more granted byte
             *   3) There is enough data buffered for one whole packet or to
             *   reach the end of the mesage
             */
            // TODO does it take too long to determine if we should insert?
            // if (sendmsg_ripe) begin
               // Adds the entry to the queue  
               for (entry = 0; entry < MAX_SRPT; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_odd[entry];
               end 
            //end

         end else if (dbuff_in_empty_i_latch_stage_1) begin
            // Is the entry able to be active
            // if (dbuff_ripe) begin
               // Adds either the reactivated message or the update to the queue
               for (entry = 0; entry < MAX_SRPT-1; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_odd[entry];
               end 
            //end

         end else if (grant_in_empty_i_latch_stage_1) begin
            // if (grant_ripe) begin
               // Adds either the reactivated message or the update to the queue
               for (entry = 0; entry < MAX_SRPT-1; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_odd[entry];
               end 
            //end

         end else if (data_pkt_full_i && head[`ENTRY_PRIORITY] == `SRPT_ACTIVE) begin
            /* We can deactivate an entry from the SRPT queue when 
             *   1) There are no more remaining bytes
             *   2) There are not enough grantable bytes
             *   3) There are not enough databuffered bytes
             */
            // TODO This is wrong here
            if ((head[`ENTRY_REMAINING] <= (head[`ENTRY_LIMIT] + `HOMA_PAYLOAD_SIZE + `HOMA_PAYLOAD_SIZE))) begin
               srpt_queue[0] <= srpt_queue[1];
               srpt_queue[1][`ENTRY_PRIORITY]  <= `SRPT_INVALIDATE;
            end else begin
               srpt_queue[0][`ENTRY_REMAINING] <= head[`ENTRY_REMAINING] - `HOMA_PAYLOAD_SIZE;
            end

         end else begin
            if (swap_type == 1'b0) begin
               // Assumes that write does not keep data around
               for (entry = 0; entry < MAX_SRPT; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_even[entry];
               end

               swap_type <= 1'b1;
            end else begin
               for (entry = 1; entry < MAX_SRPT-2; entry=entry+1) begin
                  srpt_queue[entry] <= srpt_odd[entry+1];
               end

               swap_type <= 1'b0;
            end
         end
      end 
   end 

   assign ap_ready = 1;
   assign ap_idle = 0;
   assign ap_done = 1;

endmodule

/**
 * srpt_grant_pkts_tb() - Test bench for srpt data queue
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

   reg                      sendmsg_in_empty_i;
   wire                     sendmsg_in_read_en_o;
   reg [`SRPT_DATA_SIZE-1:0]    sendmsg_in_data_i;

   reg                      grant_in_empty_i;
   wire                     grant_in_read_en_o;
   reg [`GRANT_SIZE-1:0]    grant_in_data_i;

   reg                      dbuff_in_empty_i;
   wire                     dbuff_in_read_en_o;
   reg [`DBUFF_SIZE-1:0]    dbuff_in_data_i;

   reg                      data_pkt_full_i;
   wire                     data_pkt_write_en_o;
   wire [`SRPT_DATA_SIZE-1:0] data_pkt_data_o;

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
			      .ap_idle(ap_idle),
			      .ap_done(ap_done), 
			      .ap_ready(ap_ready));

   task new_entry(input [15:0] rpc_id, input [9:0] dbuff_id, input [31:0] remaining, grantable, dbuffered, msg_len);
      begin
      
	   sendmsg_in_data_i[`SRPT_DATA_RPC_ID]    = rpc_id;
	   sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]  = dbuff_id;
	   sendmsg_in_data_i[`SRPT_DATA_REMAINING] = remaining;
	   sendmsg_in_data_i[`SRPT_DATA_GRANTED]   = grantable;
	   sendmsg_in_data_i[`SRPT_DATA_DBUFFERED] = dbuffered;
	   sendmsg_in_data_i[`SRPT_DATA_MSG_LEN]   = msg_len;

	   sendmsg_in_empty_i  = 1;

	   #5;
	   
	   sendmsg_in_empty_i  = 0;
   
      end
      
   endtask

   task new_dbuff(input [9:0] dbuff_id, input [31:0] dbuffered, msg_len);
      begin

      dbuff_in_data_i[`DBUFF_DBUFF_ID] = dbuff_id;
      dbuff_in_data_i[`DBUFF_MSG_LEN]  = msg_len;
      dbuff_in_data_i[`DBUFF_OFFSET]   = dbuffered;
      
	   dbuff_in_empty_i  = 1;

	   #5;
	   
	   dbuff_in_empty_i  = 0;
   
      end
      
   endtask

   task new_grant(input [9:0] dbuff_id, input [31:0] grant);
      begin

      grant_in_data_i[`GRANT_DBUFF_ID] = dbuff_id;
      grant_in_data_i[`GRANT_OFFSET]   = grant;
      
	   grant_in_empty_i  = 1;

	   #5;
	   
	   grant_in_empty_i  = 0;
   
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

      // Send reset signal
      ap_ce = 1; 
      ap_rst = 0;
      ap_start = 0;
      ap_rst = 1;

      #5;
      ap_rst = 0;
      ap_start = 1;

      #5;
      data_pkt_full_i = 1;
      new_entry(1, 1, 5000, 5000, 0, 5000);

      #20;
      new_dbuff(1, 5000, 5000);

      //#5;
      //if (sendmsg_in_read_en_o != 0 || grant_in_read_en_o != 0 || dbuff_in_read_en_o != 0) begin
      //   $display("Reading data when unavailible");
      //   $stop;
      //end

      //#5;

      //if (data_pkt_write_en_o != 0) begin
      //   $display("Writing data when unavailible");
      //   $stop;
      //end

      //#5;
     
      //// RPC ID, Dbuff ID, Remaining, Granted, Dbuffered, Length
      //new_entry(1, 1, 100000, 100000, 0, 100000);

      //#50;
      //if (data_pkt_write_en_o != 0) begin
      //   $display("Writing data that is unready");
      //   $stop;
      //end

      //#5;
      //new_dbuff(1, 50000, 100000);

      //#20;
      //data_pkt_full_i = 1;

      //#20;
      //data_pkt_full_i = 0;

      //#20;
      //new_dbuff(1, 100000, 100000);

      //#100;
      //data_pkt_full_i = 1;

      //#600;
      //data_pkt_full_i = 0;




      //#5;
      //new_entry(2, 2, 10000, 10000, 0, 10000);

      //#5;
      //new_entry(3, 3, 10000, 10000, 0, 10000);

      //#5;
      //new_entry(4, 4, 10000, 10000, 0, 10000);

      //#5;
      //new_entry(5, 5, 10000, 10000, 0, 10000);

      //#5;
      //new_entry(6, 6, 10000, 10000, 0, 10000);

      //#5;
      //new_entry(7, 7, 10000, 10000, 0, 10000);

      //#5;
      //new_entry(8, 8, 10000, 10000, 0, 10000);

      //#5;
      //new_entry(9, 9, 10000, 10000, 0, 10000);

      //#5;
      //new_dbuff(2, 10000, 10000);

      //#5;
      //new_dbuff(3, 10000, 10000);

      //#5;
      //new_dbuff(4, 10000, 10000);

      //#5;
      //new_dbuff(5, 10000, 10000);

      //#5;
      //new_dbuff(6, 10000, 10000);

      //#5;
      //new_dbuff(7, 10000, 10000);

      //#5;
      //new_dbuff(8, 10000, 10000);

      //#5;
      //new_dbuff(9, 10000, 10000);

      // #5;
      // new_entry(5, 10000, 5000, 0);


      // #5;
      // new_entry(6, 1, 5000, 1000);


      // #5;
      // new_entry(7, 10000, 10000, 10000);



      // new_entry(4, 4, 4, 0);

      // #15;

      // new_entry(3, 3, 3, 0);

      // #15;

      // new_entry(1, 1, 1, 0);

      // #15;

      // new_entry(2, 2, 2, 0);

      // #200;

      // new_entry(6, 3, 3, 0);

      // #15;

      // new_entry(7, 4, 4, 0);
      
      #1000
	$stop;
   end

endmodule
