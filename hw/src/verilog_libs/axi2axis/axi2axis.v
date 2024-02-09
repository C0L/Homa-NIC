////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	demofull.v
// {{{
// Project:	WB2AXIPSP: bus bridges and other odds and ends
//
// Purpose:	Demonstrate a formally verified AXI4 core with a (basic)
//		interface.  This interface is explained below.
//
// Performance: This core has been designed for a total throughput of one beat
//		per clock cycle.  Both read and write channels can achieve
//	this.  The write channel will also introduce two clocks of latency,
//	assuming no other latency from the master.  This means it will take
//	a minimum of 3+AWLEN clock cycles per transaction of (1+AWLEN) beats,
//	including both address and acknowledgment cycles.  The read channel
//	will introduce a single clock of latency, requiring 2+ARLEN cycles
//	per transaction of 1+ARLEN beats.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019-2022, Gisselquist Technology, LLC
// This file is part of the WB2AXIP project.
//
// The WB2AXIP project contains free software and gateware, licensed under the
// Apache License, Version 2.0 (the "License").  You may not use this project,
// or this file, except in compliance with the License.  You may obtain a copy
// of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//
////////////////////////////////////////////////////////////////////////////////

`default_nettype	none
module axi2axis #(parameter integer C_S_AXI_ID_WIDTH = 4,
		  parameter integer C_S_AXI_DATA_WIDTH = 512,
		  parameter integer C_S_AXI_ADDR_WIDTH = 32)
   
   (input wire				      s_axis_tvalid,
    output reg				      s_axis_tready,
    input wire [C_S_AXI_DATA_WIDTH-1:0]	      s_axis_tdata,
    input wire				      s_axis_tlast,

    output reg				      m_axis_tvalid,
    input wire				      m_axis_tready,
    output reg [C_S_AXI_DATA_WIDTH-1:0]	      m_axis_tdata,
    output reg [31:0]			      m_axis_tuser,
    output reg				      m_axis_tlast,

    // AXI signals
    // Global Clock Signal
    input wire				      s_axi_aclk,
    // Global Reset Signal. This Signal is Active LOW
    input wire				      s_axi_aresetn,

    // Write address channel
    // Write Address ID
    input wire [C_S_AXI_ID_WIDTH-1 : 0]	      s_axi_awid,
    // Write address
    input wire [C_S_AXI_ADDR_WIDTH-1 : 0]     s_axi_awaddr,
    // Burst length. The burst length gives the exact number of
    // transfers in a burst
    input wire [7 : 0]			      s_axi_awlen,
    // Burst size. This signal indicates the size of each transfer
    // in the burst
    input wire [2 : 0]			      s_axi_awsize,
    // Burst type. The burst type and the size information,
    // determine how the address for each transfer within the burst
    // is calculated.
    input wire [1 : 0]			      s_axi_awburst,
    // Lock type. Provides additional information about the
    // atomic characteristics of the transfer.
    input wire				      s_axi_awlock,
    // Memory type. This signal indicates how transactions
    // are required to progress through a system.
    input wire [3 : 0]			      s_axi_awcache,
    // Protection type. This signal indicates the privilege
    // and security level of the transaction, and whether
    // the transaction is a data access or an instruction access.
    input wire [2 : 0]			      s_axi_awprot,
    // Quality of Service, QoS identifier sent for each
    // write transaction.
    input wire [3 : 0]			      s_axi_awqos,
    // Region identifier. Permits a single physical interface
    // on a slave to be used for multiple logical interfaces.
    // Write address valid. This signal indicates that
    // the channel is signaling valid write address and
    // control information.
    input wire				      s_axi_awvalid,
    // Write address ready. This signal indicates that
    // the slave is ready to accept an address and associated
    // control signals.
    output wire				      s_axi_awready,
    // Write data channel
    // Write Data
    input wire [C_S_AXI_DATA_WIDTH-1 : 0]     s_axi_wdata,
    // Write strobes. This signal indicates which byte
    // lanes hold valid data. There is one write strobe
    // bit for each eight bits of the write data bus.
    input wire [(C_S_AXI_DATA_WIDTH/8)-1 : 0] s_axi_wstrb,
    // Write last. This signal indicates the last transfer
    // in a write burst.
    input wire				      s_axi_wlast,
    // Optional User-defined signal in the write data channel.
    // Write valid. This signal indicates that valid write
    // data and strobes are available.
    input wire				      s_axi_wvalid,
    // Write ready. This signal indicates that the slave
    // can accept the write data.
    output wire				      s_axi_wready,
    // Write response channel
    // Response ID tag. This signal is the ID tag of the
    // write response.
    output wire [C_S_AXI_ID_WIDTH-1 : 0]      s_axi_bid,
    // Write response. This signal indicates the status
    // of the write transaction.
    output wire [1 : 0]			      s_axi_bresp,
    // Optional User-defined signal in the write response channel.
    // Write response valid. This signal indicates that the
    // channel is signaling a valid write response.
    output wire				      s_axi_bvalid,
    // Response ready. This signal indicates that the master
    // can accept a write response.
    input wire				      s_axi_bready,
    // Read address channel
    // Read address ID. This signal is the identification
    // tag for the read address group of signals.
    input wire [C_S_AXI_ID_WIDTH-1 : 0]	      s_axi_arid,
    // Read address. This signal indicates the initial
    // address of a read burst transaction.
    input wire [C_S_AXI_ADDR_WIDTH-1 : 0]     s_axi_araddr,
    // Burst length. The burst length gives the exact number of
    // transfers in a burst
    input wire [7 : 0]			      s_axi_arlen,
    // Burst size. This signal indicates the size of each transfer
    // in the burst
    input wire [2 : 0]			      s_axi_arsize,
    // Burst type. The burst type and the size information,
    // determine how the address for each transfer within the
    // burst is calculated.
    input wire [1 : 0]			      s_axi_arburst,
    // Lock type. Provides additional information about the
    // atomic characteristics of the transfer.
    input wire				      s_axi_arlock,
    // Memory type. This signal indicates how transactions
    // are required to progress through a system.
    input wire [3 : 0]			      s_axi_arcache,
    // Protection type. This signal indicates the privilege
    // and security level of the transaction, and whether
    // the transaction is a data access or an instruction access.
    input wire [2 : 0]			      s_axi_arprot,
    // Quality of Service, QoS identifier sent for each
    // read transaction.
    input wire [3 : 0]			      s_axi_arqos,
    // Region identifier. Permits a single physical interface
    // on a slave to be used for multiple logical interfaces.
    // Optional User-defined signal in the read address channel.
    // Write address valid. This signal indicates that
    // the channel is signaling valid read address and
    // control information.
    input wire				      s_axi_arvalid,
    // Read address ready. This signal indicates that
    // the slave is ready to accept an address and associated
    // control signals.
    output wire				      s_axi_arready,
    // Read data (return) channel
    // Read ID tag. This signal is the identification tag
    // for the read data group of signals generated by the slave.
    output wire [C_S_AXI_ID_WIDTH-1 : 0]      s_axi_rid,
    // Read Data
    output wire [C_S_AXI_DATA_WIDTH-1 : 0]    s_axi_rdata,
    // Read response. This signal indicates the status of
    // the read transfer.
    output wire [1 : 0]			      s_axi_rresp,
    // Read last. This signal indicates the last transfer
    // in a read burst.
    output wire				      s_axi_rlast,
    // Optional User-defined signal in the read address channel.
    // Read valid. This signal indicates that the channel
    // is signaling the required read data.
    output wire				      s_axi_rvalid,
    // Read ready. This signal indicates that the master can
    // accept the read data and response information.
    input wire				      s_axi_rready
    );

   // Local declarations
   // More useful shorthand definitions
   // localparam				      AW = C_S_AXI_ADDR_WIDTH;
   // localparam				      DW = C_S_AXI_DATA_WIDTH;
   // localparam				      IW = C_S_AXI_ID_WIDTH;
   
   // Double buffer the write response channel only
   reg [C_S_AXI_ID_WIDTH-1 : 0]		      r_bid;
   reg					      r_bvalid;
   reg [C_S_AXI_ID_WIDTH-1 : 0]		      axi_bid;
   reg					      axi_bvalid;

   reg					      axi_awready, axi_wready;
   reg [C_S_AXI_ADDR_WIDTH-1:0]		      waddr;
   wire [C_S_AXI_ADDR_WIDTH-1:0]	      next_wr_addr;

   // Vivado will warn about wlen only using 4-bits.  This is
   // to be expected, since the axi_addr module only needs to use
   // the bottom four bits of wlen to determine address increments
   reg [7:0]				      wlen;
   // Vivado will also warn about the top bit of wsize being unused.
   // This is also to be expected for a DATA_WIDTH of 32-bits.
   reg [2:0]				      wsize;
   reg [1:0]				      wburst;

   wire					      m_awvalid, m_awlock;
   reg					      m_awready;
   wire [C_S_AXI_ADDR_WIDTH-1:0]	      m_awaddr;
   wire [1:0]				      m_awburst;
   wire [2:0]				      m_awsize;
   wire [7:0]				      m_awlen;
   wire [C_S_AXI_ID_WIDTH-1:0]		      m_awid;

   wire [C_S_AXI_ADDR_WIDTH-1:0]	      next_rd_addr;

   // Vivado will warn about rlen only using 4-bits.  This is
   // to be expected, since for a DATA_WIDTH of 32-bits, the axi_addr
   // module only uses the bottom four bits of rlen to determine
   // address increments
   reg [7:0]				      rlen;
   // Vivado will also warn about the top bit of wsize being unused.
   // This is also to be expected for a DATA_WIDTH of 32-bits.
   reg [2:0]				      rsize;
   reg [1:0]				      rburst;
   reg [C_S_AXI_ID_WIDTH-1:0]		      rid;
   reg					      rlock;
   reg					      axi_arready;
   reg [8:0]				      axi_rlen;
   reg [C_S_AXI_ADDR_WIDTH-1:0]		      raddr;

   // Read skid buffer
   reg					      rskd_valid, rskd_last, rskd_lock;
   wire					      rskd_ready;
   reg [C_S_AXI_ID_WIDTH-1:0]		      rskd_id;

   // }}}
   ////////////////////////////////////////////////////////////////////////
   //
   // AW Skid buffer
   // {{{
   ////////////////////////////////////////////////////////////////////////
   //
   //
   skidbuffer #(.DW(C_S_AXI_ADDR_WIDTH+2+3+1+8+C_S_AXI_ID_WIDTH),
		.OPT_LOWPOWER(1'b0),
		.OPT_OUTREG(1'b0)
		) awbuf(.i_clk(s_axi_aclk), .i_reset(!s_axi_aresetn),
			.i_valid(s_axi_awvalid), .o_ready(s_axi_awready),
			.i_data({s_axi_awaddr, s_axi_awburst, s_axi_awsize,
				 s_axi_awlock, s_axi_awlen, s_axi_awid}),
			.o_valid(m_awvalid), .i_ready(m_awready),
			.o_data({m_awaddr, m_awburst, m_awsize, m_awlock, m_awlen, m_awid})
			);
   ////////////////////////////////////////////////////////////////////////
   //
   // Write processing
   ////////////////////////////////////////////////////////////////////////

   // axi_awready, axi_wready
   initial	axi_awready = 1;
   initial	axi_wready  = 0;
   always @(posedge s_axi_aclk)
     if (!s_axi_aresetn)
       begin
	  axi_awready  <= 1;
	  axi_wready   <= 0;
       end else if (m_awvalid && m_awready)
	 begin
	    axi_awready <= 0;
	    axi_wready  <= 1;
	 end else if (s_axi_wvalid && s_axi_wready)
	   begin
	      axi_awready <= (s_axi_wlast)&&(!s_axi_bvalid || s_axi_bready);
	      axi_wready  <= (!s_axi_wlast);
	   end else if (!axi_awready)
	     begin
		if (s_axi_wready)
		  axi_awready <= 1'b0;
		else if (r_bvalid && !s_axi_bready)
		  axi_awready <= 1'b0;
		else
		  axi_awready <= 1'b1;
	     end

   // Next write address calculation
   always @(posedge s_axi_aclk)
     if (m_awready)
       begin
	  waddr    <= m_awaddr;
	  wburst   <= m_awburst;
	  wsize    <= m_awsize;
	  wlen     <= m_awlen;
       end else if (s_axi_wvalid)
	 waddr <= next_wr_addr;

   axi_addr #(.AW(C_S_AXI_ADDR_WIDTH), .DW(C_S_AXI_DATA_WIDTH)
	      ) get_next_wr_addr(waddr, wsize, wburst, wlen,
				 next_wr_addr
				 );
   // o_w*
   // {{{
   always @(posedge s_axi_aclk)
     begin
	m_axis_tvalid <= (s_axi_wvalid && s_axi_wready && m_axis_tready);
	m_axis_tdata  <= s_axi_wdata;
	m_axis_tuser  <= waddr;
	
	if (!s_axi_aresetn)
	  m_axis_tvalid <= 0;
     end

   // Write return path
   // r_bvalid
   initial	r_bvalid = 0;
   always @(posedge s_axi_aclk)
     if (!s_axi_aresetn)
       r_bvalid <= 1'b0;
     else if (s_axi_wvalid && s_axi_wready && s_axi_wlast
	      &&(s_axi_bvalid && !s_axi_bready))
       r_bvalid <= 1'b1;
     else if (s_axi_bready)
       r_bvalid <= 1'b0;

   // r_bid, axi_bid
   initial	r_bid = 0;
   initial	axi_bid = 0;
   always @(posedge s_axi_aclk)
     begin
	if (m_awready && (m_awvalid))
	  r_bid    <= m_awid;

	if (!s_axi_bvalid || s_axi_bready)
	  axi_bid <= r_bid;

	if (!s_axi_aresetn)
	  begin
	     r_bid <= 0;
	     axi_bid <= 0;
	  end
     end

   // axi_bvalid
   initial	axi_bvalid = 0;
   always @(posedge s_axi_aclk)
     if (!s_axi_aresetn)
       axi_bvalid <= 0;
     else if (s_axi_wvalid && s_axi_wready && s_axi_wlast)
       axi_bvalid <= 1;
     else if (s_axi_bready)
       axi_bvalid <= r_bvalid;

   // m_awready
   always @(*)
     begin
	m_awready = axi_awready;
	if (s_axi_wvalid && s_axi_wready && s_axi_wlast
	    && (!s_axi_bvalid || s_axi_bready))
	  m_awready = 1;
     end

   // At one time, axi_awready was the same as S_AXI_AWREADY.  Now, though,
   // with the extra write address skid buffer, this is no longer the case.
   // S_AXI_AWREADY is handled/created/managed by the skid buffer.
   //
   // assign S_AXI_AWREADY = axi_awready;
   //
   // The rest of these signals can be set according to their registered
   // values above.
   assign	s_axi_wready  = axi_wready;
   assign	s_axi_bvalid  = axi_bvalid;
   assign	s_axi_bid     = axi_bid;
   //
   // This core does not produce any bus errors, nor does it support
   // exclusive access, so 2'b00 will always be the correct response.
   assign	s_axi_bresp = 2'b00;
   
   ////////////////////////////////////////////////////////////////////////
   //
   // Read processing
   ////////////////////////////////////////////////////////////////////////

   // axi_arready
   initial axi_arready = 1;
   always @(posedge s_axi_aclk)
     if (!s_axi_aresetn)
       axi_arready <= 1;
     else if (s_axi_arvalid && s_axi_arready)
       axi_arready <= (s_axi_arlen==0)&&(s_axis_tready);
     else if (s_axis_tready)
       axi_arready <= (axi_rlen <= 1);

   // axi_rlen
   initial	axi_rlen = 0;
   always @(posedge s_axi_aclk)
     if (!s_axi_aresetn)
       axi_rlen <= 0;
     else if (s_axi_arvalid && s_axi_arready)
       axi_rlen <= s_axi_arlen + (s_axis_tready ? 0:1);
     else if (s_axis_tready)
       axi_rlen <= axi_rlen - 1;

   // Next read address calculation
   always @(posedge s_axi_aclk)
     if (s_axis_tready)
       raddr <= next_rd_addr;
     else if (s_axi_arready)
       begin
	  raddr <= s_axi_araddr;
	  if (!s_axi_arvalid)
	    raddr <= 0;
       end

   // r*
   always @(posedge s_axi_aclk)
     if (s_axi_arready)
       begin
	  rburst   <= s_axi_arburst;
	  rsize    <= s_axi_arsize;
	  rlen     <= s_axi_arlen;
	  rid      <= s_axi_arid;
	  rlock    <= s_axi_arlock && s_axi_arvalid;

	  if (!s_axi_arvalid)
	    begin
	       rburst   <= 0;
	       rsize    <= 0;
	       rlen     <= 0;
	       rid      <= 0;
	       rlock    <= 0;
	    end
       end

   axi_addr #(.AW(C_S_AXI_ADDR_WIDTH), .DW(C_S_AXI_DATA_WIDTH)
	      ) get_next_rd_addr((s_axi_arready ? s_axi_araddr : raddr),
				 (s_axi_arready ? s_axi_arsize : rsize),
				 (s_axi_arready ? s_axi_arburst: rburst),
				 (s_axi_arready ? s_axi_arlen  : rlen),
				 next_rd_addr);

   // o_rd, o_raddr
   always @(*)
     begin
	s_axis_tready = (s_axi_arvalid || !s_axi_arready);
	if (s_axi_rvalid && !s_axi_rready)
	  s_axis_tready = 0;
	if (rskd_valid && !rskd_ready)
	  s_axis_tready = 0;
	// o_raddr = (S_AXI_ARREADY ? S_AXI_ARADDR : raddr);
     end

   // rskd_valid
   initial	rskd_valid = 0;
   always @(posedge s_axi_aclk)
     if (!s_axi_aresetn)
       rskd_valid <= 0;
     else if (s_axis_tready)
       rskd_valid <= 1;
     else if (rskd_ready)
       rskd_valid <= 0;

   // rskd_id
   always @(posedge s_axi_aclk)
     if (!rskd_valid || rskd_ready)
       begin
	  if (s_axi_arvalid && s_axi_arready)
	    rskd_id <= s_axi_arid;
	  else
	    rskd_id <= rid;
       end

   // rskd_last
   initial rskd_last   = 0;
   always @(posedge s_axi_aclk)
     if (!rskd_valid || rskd_ready)
       begin
	  rskd_last <= 0;
	  if (s_axis_tready && axi_rlen == 1)
	    rskd_last <= 1;
	  if (s_axi_arvalid && s_axi_arready && s_axi_arlen == 0)
	    rskd_last <= 1;
       end

   // rskd_lock
   always @(posedge s_axi_aclk)
     if (!s_axi_aresetn)
       rskd_lock <= 1'b0;
     else if (!rskd_valid || rskd_ready)
       begin
	  rskd_lock <= 0;
	  if (s_axis_tready)
	    begin
	       if (s_axi_arvalid && s_axi_arready)
		 rskd_lock <= s_axi_arlock;
	       else
		 rskd_lock <= rlock;
	    end
       end

   // Outgoing read skidbuffer
   skidbuffer #(.OPT_LOWPOWER(1'b0),
		.OPT_OUTREG(1),
		.DW(C_S_AXI_ID_WIDTH+2+C_S_AXI_DATA_WIDTH)
		) rskid (.i_clk(s_axi_aclk), .i_reset(!s_axi_aresetn),
			 .i_valid(rskd_valid), .o_ready(rskd_ready),
			 .i_data({rskd_id, rskd_lock, rskd_last, s_axis_tdata}),
			 // .i_data({ rskd_id, rskd_lock, rskd_last, i_rdata }),
			 .o_valid(s_axi_rvalid), .i_ready(s_axi_rready),
			 .o_data({ s_axi_rid, s_axi_rresp[0], s_axi_rlast, s_axi_rdata })
			 );

   assign	s_axi_rresp[1] = 1'b0;
   assign	s_axi_arready = axi_arready;

endmodule
