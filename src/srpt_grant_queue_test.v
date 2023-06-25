`timescale 1ns / 1ps

module srpt_grant_queue_test ();

reg ap_clk=0;
reg ap_rst;
reg ap_ce;
reg ap_start;
reg ap_continue;


wire ap_idle;
wire ap_done;
wire ap_ready;

reg header_in_empty_i;
reg [124:0] header_in_data_i;
reg grant_pkt_full_o;

wire header_in_read_en_o;
wire [94:0] grant_pkt_data_o;
wire grant_pkt_write_en_o;

// header_in
// peer_id        124:110
// local_id       109:96
// message_length 95:64
// incoming       63:32
// data_offset    31:0

srpt_grant_pkts srpt_queue(.ap_clk(ap_clk), .ap_rst(ap_rst), .ap_ce(ap_ce), .ap_start(ap_start), .ap_continue(ap_continue), 
    .header_in_empty_i(header_in_empty_i), .header_in_read_en_o(header_in_read_en_o), .header_in_data_i(header_in_data_i), 
    .grant_pkt_full_o(grant_pkt_full_o), .grant_pkt_write_en_o(grant_pkt_write_en_o), .grant_pkt_data_o(grant_pkt_data_o),
    .ap_idle(ap_idle), .ap_done(ap_done), .ap_ready(ap_ready));


initial begin
  ap_clk = 0;

forever begin
  #2.5 ap_clk = ~ap_clk;
end end

    initial begin
        header_in_data_i = {124'b0};
        header_in_empty_i = 1;
        grant_pkt_full_o = 1;
        // Send reset signal
        ap_ce = 1; 
        ap_rst = 1; 
        
        #5;
        ap_rst = 0;
        
        //#5;
        ap_start = 1;
        header_in_data_i = {14'bb1110111011101110, 14'b1100110011001100, 32'b10000, 32'b100, 32'b0};
        header_in_empty_i = 0;
        grant_pkt_full_o = 0;

        //#5;
        //header_in_data_i = {14'b10, 14'b1, 32'hFFFFFFFF, 32'b10, 32'b0};
        //header_in_empty_i = 0;
        //grant_pkt_full_o = 0;
        
        //#5;
        //header_in_data_i = {14'b1, 14'b1, 14'b1, 32'b10000, 32'b100, 32'b0};
        //header_in_data_i = {14'b10, 14'b1, 32'hFFFF, 32'b10, 32'b0};
        //header_in_empty_i = 0;
        //grant_pkt_full_o = 0;
        
        #5;
    
        header_in_empty_i = 1;
        grant_pkt_full_o = 0;
        
        #5;
        header_in_empty_i = 1;
        grant_pkt_full_o = 1;
    end

endmodule
