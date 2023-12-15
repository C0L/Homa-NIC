task reset();
   begin
      // Send reset signal
      ap_rst = 1;
      #5;
      ap_rst = 0;
      #5;
   end
endtask // reset
 
task enqueue(input [15:0] rpc_id, 
	     input [9:0]  dbuff_id, 
	     input [19:0] initial_grant, 
	     input [19:0] initial_dbuff, 
	     input [19:0] msg_len);
   
   begin
      S_AXIS_TDATA[`QUEUE_ENTRY_RPC_ID]    = rpc_id;
      S_AXIS_TDATA[`QUEUE_ENTRY_DBUFF_ID]  = dbuff_id;
      S_AXIS_TDATA[`QUEUE_ENTRY_REMAINING] = msg_len;
      S_AXIS_TDATA[`QUEUE_ENTRY_DBUFFERED] = initial_dbuff;
      S_AXIS_TDATA[`QUEUE_ENTRY_GRANTED]   = initial_grant;
      S_AXIS_TDATA[`QUEUE_ENTRY_PRIORITY]  = `SRPT_ACTIVE;
      
      $display(
	       "enqueue:   \n \
     RPC ID:   %0d \n \
     DBUFF ID: %0d \n \
     REM:      %0d \n \
     DBUFFD:   %0d \n \
     GRANTED:  %0d \n \
     PRIORITY: %0d \n ",
	       S_AXIS_TDATA[`QUEUE_ENTRY_RPC_ID], 
	       S_AXIS_TDATA[`QUEUE_ENTRY_DBUFF_ID], 
	       S_AXIS_TDATA[`QUEUE_ENTRY_REMAINING], 
	       S_AXIS_TDATA[`QUEUE_ENTRY_DBUFFERED], 
	       S_AXIS_TDATA[`QUEUE_ENTRY_GRANTED], 
	       S_AXIS_TDATA[`QUEUE_ENTRY_PRIORITY]
	       );
      
      S_AXIS_TVALID = 1;
      wait(S_AXIS_TREADY == 1);
      #5;
      S_AXIS_TVALID = 0;
   end
   
endtask // enqueue

task dequeue();
   begin
      wait(M_AXIS_TVALID == 1);
      
      M_AXIS_TREADY = 1;
      
      $display(
	       "dequeue:   \n \
     RPC ID:   %0d \n \
     DBUFF ID: %0d \n \
     REM:      %0d \n \
     DBUFFD:   %0d \n \
     GRANTED:  %0d \n \
     PRIORITY: %0d \n ",
	       M_AXIS_TDATA[`QUEUE_ENTRY_RPC_ID], 
	       M_AXIS_TDATA[`QUEUE_ENTRY_DBUFF_ID], 
	       M_AXIS_TDATA[`QUEUE_ENTRY_REMAINING], 
	       M_AXIS_TDATA[`QUEUE_ENTRY_DBUFFERED], 
	       M_AXIS_TDATA[`QUEUE_ENTRY_GRANTED], 
	       M_AXIS_TDATA[`QUEUE_ENTRY_PRIORITY]
	       );

      #5;
      M_AXIS_TREADY = 0;

   end
endtask // get_pkt
   
task test_is_empty();
    begin
       if(M_AXIS_TVALID) begin
	  $display("FAILURE: Data pkts should be empty");
	  $stop;
       end
    end
endtask

