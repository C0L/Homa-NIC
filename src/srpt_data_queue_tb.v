`timescale 1ns / 1ps

/* verilator lint_off STMTDLY */
module srpt_grant_queue_tb();

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

   task new_entry(input [9:0] dbuff_id, input [31:0] remaining, grantable, dbuffered);
      begin
      
	   sendmsg_in_data_i[`SRPT_DATA_DBUFF_ID]  = dbuff_id;
	   sendmsg_in_data_i[`SRPT_DATA_REMAINING] = remaining;
	   sendmsg_in_data_i[`SRPT_DATA_GRANTED]   = grantable;
	   sendmsg_in_data_i[`SRPT_DATA_DBUFFERED] = dbuffered;

	   sendmsg_in_empty_i  = 1;

	   #5;
	   
	   sendmsg_in_empty_i  = 0;
   
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
      sendmsg_in_data_i = {`SRPT_DATA_SIZE{1'b0}};

	   sendmsg_in_empty_i  = 0;
	   grant_in_empty_i    = 0;
	   dbuff_in_empty_i    = 0;
	   data_pkt_full_i  = 0;

      // Send reset signal
      ap_ce = 1; 
      ap_rst = 0;
      ap_start = 0;
      ap_rst = 1;

      #5;
      ap_rst = 0;
      ap_start = 1;

      #5;
      
      new_entry(1, 10000, 5000, 5000);

      // #15;

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
