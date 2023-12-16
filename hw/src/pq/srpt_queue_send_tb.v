`include "srpt_queue.v"

/**
 * srpt_queue_send_tb() - Test bench for srpt queue
 */
/* verilator lint_off STMTDLY */
module srpt_queue_send_tb();
`include "util.v"
   
   reg ap_clk=0;
   reg ap_rst;

   reg S_AXIS_TVALID;
   wire	S_AXIS_TREADY;
   reg [`QUEUE_ENTRY_SIZE-1:0] S_AXIS_TDATA;

   reg			       M_AXIS_TREADY;
   wire			       M_AXIS_TVALID;
   wire [`QUEUE_ENTRY_SIZE-1:0]	M_AXIS_TDATA;

   integer			i;
   
   srpt_queue #(.MAX_RPCS(64), 
		.TYPE("sendmsg")) 
   srpt_queue_dut (.ap_clk(ap_clk), 
		   .ap_rst(ap_rst), 
		   .S_AXIS_TVALID(S_AXIS_TVALID),
		   .S_AXIS_TREADY(S_AXIS_TREADY),
		   .S_AXIS_TDATA(S_AXIS_TDATA),
		   .M_AXIS_TVALID(M_AXIS_TVALID),
		   .M_AXIS_TREADY(M_AXIS_TREADY),
		   .M_AXIS_TDATA(M_AXIS_TDATA));

   task sendmsg_test_single_entry();
   begin
      enqueue(1, 1, 0, 0, 10000);
      for (i = 0; i < 10000 / `HOMA_PAYLOAD_SIZE + 1; i=i+1) begin
	  dequeue();
      end
      
      test_is_empty();
   end
   endtask // sendmsg_test_single_entry
 
   initial begin
      ap_clk = 0;

      forever begin
	 #2.5 ap_clk = ~ap_clk;
      end 
   end
   
   initial begin
      M_AXIS_TREADY = 0;
      S_AXIS_TDATA  = 0;
      S_AXIS_TVALID = 0;

      reset();

      sendmsg_test_single_entry();
      
      // RPC ID, DBUFF ID, INIT GRANT, INIT DBUFF, MSG LEN, INITIAL STATE

      // get_output();
      #1000;
      $stop;
   end

endmodule
