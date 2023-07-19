`timescale 1ns / 1ps

/* verilator lint_off STMTDLY */

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
   wire [`ENTRY_SIZE-1:0] grant_pkt_data_o; // TODO This is bad
   wire	       grant_pkt_write_en_o;

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
   
	 // #5;

    //header_in_empty_i = 0;
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
      grant_pkt_full_o = 0;
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

      #15;

      new_entry(4, 4, 4, 0);

      #15;

      new_entry(3, 3, 3, 0);

      #15;

      new_entry(1, 1, 1, 0);

      #15;

      new_entry(2, 2, 2, 0);

      #200;

      new_entry(6, 3, 3, 0);

      #15;

      new_entry(7, 4, 4, 0);
      
      #1000
	$stop;
   end

endmodule
