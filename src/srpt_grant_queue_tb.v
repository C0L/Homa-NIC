`timescale 1ns / 1ps

/* verilator lint_off STMTDLY */

`define HEADER_SIZE 58
`define HDR_PEER_ID 57:44
`define HDR_RPC_ID 43:30
`define HDR_MSG_LEN 29:20
`define HDR_INCOMING 19:10
`define HDR_OFFSET 9:0

module srpt_grant_queue_tb();

   reg ap_clk=0;
   reg ap_rst;
   reg ap_ce;
   reg ap_start;
   reg ap_continue;

   wire	ap_idle;
   wire	ap_done;
   wire	ap_ready;

   reg	header_in_empty_i;
   reg [`HEADER_SIZE-1:0] header_in_data_i;
   reg	       grant_pkt_full_o;

   wire	       header_in_read_en_o;
   wire [51-1:0] grant_pkt_data_o;
   wire	       grant_pkt_write_en_o;

   // header_in
   // peer_id        124:110
   // local_id       109:96
   // message_length 95:64
   // incoming       63:32
   // data_offset    31:0

   srpt_grant_pkts srpt_queue(.ap_clk(ap_clk), 
			      .ap_rst(ap_rst), 
			      .ap_ce(ap_ce), 
			      .ap_start(ap_start), 
			      .ap_continue(ap_continue), 
			      .header_in_empty_i(header_in_empty_i), 
			      .header_in_read_en_o(header_in_read_en_o), 
			      .header_in_data_i(header_in_data_i), 
			      .grant_pkt_full_o(grant_pkt_full_o), 
			      .grant_pkt_write_en_o(grant_pkt_write_en_o), 
			      .grant_pkt_data_o(grant_pkt_data_o),
			      .ap_idle(ap_idle), 
			      .ap_done(ap_done), 
			      .ap_ready(ap_ready));

   task new_entry(input [13:0] rpc_id, peer_id, input [9:0] msg_len, offset);
      begin
      
	 header_in_data_i[`HDR_PEER_ID] = peer_id;
	 header_in_data_i[`HDR_RPC_ID] = rpc_id;
	 header_in_data_i[`HDR_MSG_LEN] = msg_len;
	 header_in_data_i[`HDR_OFFSET] = offset;
	 header_in_data_i[`HDR_INCOMING] = 0;
	 header_in_empty_i = 0;

	 #5;
	 
	 header_in_empty_i = 1;
   
	 // #5;

    //header_in_empty_i = 0;
      end
      
   endtask
   
   initial begin
      ap_clk = 0;

      forever begin
	 #2.5 ap_clk = ~ap_clk;
      end 
   end
   
   initial begin
      header_in_data_i = {`HEADER_SIZE{1'b0}};
      header_in_empty_i = 1;
      grant_pkt_full_o = 1;
      // Send reset signal
      ap_ce = 1; 
      ap_rst = 0;
      ap_start = 0;
      ap_rst = 1;

      #5;
      ap_rst = 0;
      ap_start = 1;
      
      //new_entry(5, 5, 5, 0);
      //new_entry(4, 4, 4, 0);
      //new_entry(3, 3, 3, 0);
      //new_entry(2, 2, 2, 0);
      new_entry(1, 1, 1, 0);

      
      //header_in_data_i = {14'b11101110111011, 14'b00110011001100, 10'b10000, 10'b100, 32'b0};
      //header_in_empty_i = 0;
      //grant_pkt_full_o = 0;

      //#5;
      //header_in_data_i = {14'b10, 14'b1, 32'hFFFFFFFF, 32'b10, 32'b0};
      //header_in_empty_i = 0;
      //grant_pkt_full_o = 0;
      
      //#5;
      //header_in_data_i = {14'b1, 14'b1, 14'b1, 32'b10000, 32'b100, 32'b0};
      //header_in_data_i = {14'b10, 14'b1, 32'hFFFF, 32'b10, 32'b0};
      //header_in_empty_i = 0;
      //grant_pkt_full_o = 0;
      
      //#5;

      //if (header_in_read_en_o != 1'b1) begin
      //	 $display("Incorrect Input FIFO Handshake");
      //end
      //
      //header_in_empty_i = 1;
      //grant_pkt_full_o = 0;
      //
      //#5;
      //header_in_empty_i = 1;
      //grant_pkt_full_o = 1;

      #1000
	$stop;
   end

endmodule
