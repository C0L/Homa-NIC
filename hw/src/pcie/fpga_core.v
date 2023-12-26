/*

Copyright (c) 2018 Alex Forencich

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

// Language: Verilog 2001

`resetall
`timescale 1ns / 1ps
`default_nettype none

/*
 * FPGA core logic
 */
module fpga_core #
(
    parameter AXIS_PCIE_DATA_WIDTH = 512,
    parameter AXIS_PCIE_KEEP_WIDTH = 16,
    parameter AXIS_PCIE_RC_USER_WIDTH = 161,
    parameter AXIS_PCIE_RQ_USER_WIDTH = 137,
    parameter AXIS_PCIE_CQ_USER_WIDTH = 183,
    parameter AXIS_PCIE_CC_USER_WIDTH = 81,
  
    parameter PCIE_ADDR_WIDTH = 64,

    parameter AXI_DATA_WIDTH = 512,
    parameter AXI_STRB_WIDTH = 64,
    parameter AXI_ADDR_WIDTH = 48,
    parameter AXI_ID_WIDTH = 8,

    parameter DMA_TAG_WIDTH = 8
)
(
    /*
     * Clock: 250 MHz
     * Synchronous reset
     */
    input wire				      clk,
    input wire				      rst,

    /*
     * AXI Master output
     */
    output wire [AXI_ID_WIDTH-1:0]	      m_axi_h2c_awid,
    output wire [AXI_ADDR_WIDTH-1:0]	      m_axi_h2c_awaddr,
    output wire [7:0]			      m_axi_h2c_awlen,
    output wire [2:0]			      m_axi_h2c_awsize,
    output wire [1:0]			      m_axi_h2c_awburst,
    output wire				      m_axi_h2c_awlock,
    output wire [3:0]			      m_axi_h2c_awcache,
    output wire [2:0]			      m_axi_h2c_awprot,
    output wire				      m_axi_h2c_awvalid,
    input wire				      m_axi_h2c_awready,
    output wire [AXI_DATA_WIDTH-1:0]	      m_axi_h2c_wdata,
    output wire [AXI_STRB_WIDTH-1:0]	      m_axi_h2c_wstrb,
    output wire				      m_axi_h2c_wlast,
    output wire				      m_axi_h2c_wvalid,
    input wire				      m_axi_h2c_wready,
    input wire [AXI_ID_WIDTH-1:0]	      m_axi_h2c_bid,
    input wire [1:0]			      m_axi_h2c_bresp,
    input wire				      m_axi_h2c_bvalid,
    output wire				      m_axi_h2c_bready,
    output wire [AXI_ID_WIDTH-1:0]	      m_axi_h2c_arid,
    output wire [AXI_ADDR_WIDTH-1:0]	      m_axi_h2c_araddr,
    output wire [7:0]			      m_axi_h2c_arlen,
    output wire [2:0]			      m_axi_h2c_arsize,
    output wire [1:0]			      m_axi_h2c_arburst,
    output wire				      m_axi_h2c_arlock,
    output wire [3:0]			      m_axi_h2c_arcache,
    output wire [2:0]			      m_axi_h2c_arprot,
    output wire				      m_axi_h2c_arvalid,
    input wire				      m_axi_h2c_arready,
    input wire [AXI_ID_WIDTH-1:0]	      m_axi_h2c_rid,
    input wire [AXI_DATA_WIDTH-1:0]	      m_axi_h2c_rdata,
    input wire [1:0]			      m_axi_h2c_rresp,
    input wire				      m_axi_h2c_rlast,
    input wire				      m_axi_h2c_rvalid,
    output wire				      m_axi_h2c_rready,
  
    /*
     * AXI Master output
     */
    output wire [AXI_ID_WIDTH-1:0]	      m_axi_c2h_awid,
    output wire [AXI_ADDR_WIDTH-1:0]	      m_axi_c2h_awaddr,
    output wire [7:0]			      m_axi_c2h_awlen,
    output wire [2:0]			      m_axi_c2h_awsize,
    output wire [1:0]			      m_axi_c2h_awburst,
    output wire				      m_axi_c2h_awlock,
    output wire [3:0]			      m_axi_c2h_awcache,
    output wire [2:0]			      m_axi_c2h_awprot,
    output wire				      m_axi_c2h_awvalid,
    input wire				      m_axi_c2h_awready,
    output wire [AXI_DATA_WIDTH-1:0]	      m_axi_c2h_wdata,
    output wire [AXI_STRB_WIDTH-1:0]	      m_axi_c2h_wstrb,
    output wire				      m_axi_c2h_wlast,
    output wire				      m_axi_c2h_wvalid,
    input wire				      m_axi_c2h_wready,
    input wire [AXI_ID_WIDTH-1:0]	      m_axi_c2h_bid,
    input wire [1:0]			      m_axi_c2h_bresp,
    input wire				      m_axi_c2h_bvalid,
    output wire				      m_axi_c2h_bready,
    output wire [AXI_ID_WIDTH-1:0]	      m_axi_c2h_arid,
    output wire [AXI_ADDR_WIDTH-1:0]	      m_axi_c2h_araddr,
    output wire [7:0]			      m_axi_c2h_arlen,
    output wire [2:0]			      m_axi_c2h_arsize,
    output wire [1:0]			      m_axi_c2h_arburst,
    output wire				      m_axi_c2h_arlock,
    output wire [3:0]			      m_axi_c2h_arcache,
    output wire [2:0]			      m_axi_c2h_arprot,
    output wire				      m_axi_c2h_arvalid,
    input wire				      m_axi_c2h_arready,
    input wire [AXI_ID_WIDTH-1:0]	      m_axi_c2h_rid,
    input wire [AXI_DATA_WIDTH-1:0]	      m_axi_c2h_rdata,
    input wire [1:0]			      m_axi_c2h_rresp,
    input wire				      m_axi_c2h_rlast,
    input wire				      m_axi_c2h_rvalid,
    output wire				      m_axi_c2h_rready,

    input wire [PCIE_ADDR_WIDTH-1:0]	      pcie_dma_read_desc_pcie_addr,
    input wire [AXI_ADDR_WIDTH-1:0]	      pcie_dma_read_desc_axi_addr,
    input wire [15:0]			      pcie_dma_read_desc_len,
    input wire [DMA_TAG_WIDTH-1:0]	      pcie_dma_read_desc_tag,
    input wire				      pcie_dma_read_desc_valid,
    output wire				      pcie_dma_read_desc_ready,
 
    output wire [DMA_TAG_WIDTH-1:0]	      pcie_dma_read_desc_status_tag,
    output wire [3:0]			      pcie_dma_read_desc_status_error,
    output wire				      pcie_dma_read_desc_status_valid,
 
    input wire [PCIE_ADDR_WIDTH-1:0]	      pcie_dma_write_desc_pcie_addr,
    input wire [AXI_ADDR_WIDTH-1:0]	      pcie_dma_write_desc_axi_addr,
    input wire [15:0]			      pcie_dma_write_desc_len,
    input wire [DMA_TAG_WIDTH-1:0]	      pcie_dma_write_desc_tag,
    input wire				      pcie_dma_write_desc_valid,
    output wire				      pcie_dma_write_desc_ready,
 
    output wire [DMA_TAG_WIDTH-1:0]	      pcie_dma_write_desc_status_tag,
    output wire [3:0]			      pcie_dma_write_desc_status_error,
    output wire				      pcie_dma_write_desc_status_valid,

    /*
     * PCIe
     */
    output wire [AXIS_PCIE_DATA_WIDTH-1:0]    m_axis_rq_tdata,
    output wire [AXIS_PCIE_KEEP_WIDTH-1:0]    m_axis_rq_tkeep,
    output wire				      m_axis_rq_tlast,
    input wire				      m_axis_rq_tready,
    output wire [AXIS_PCIE_RQ_USER_WIDTH-1:0] m_axis_rq_tuser,
    output wire				      m_axis_rq_tvalid,

    input wire [AXIS_PCIE_DATA_WIDTH-1:0]     s_axis_rc_tdata,
    input wire [AXIS_PCIE_KEEP_WIDTH-1:0]     s_axis_rc_tkeep,
    input wire				      s_axis_rc_tlast,
    output wire				      s_axis_rc_tready,
    input wire [AXIS_PCIE_RC_USER_WIDTH-1:0]  s_axis_rc_tuser,
    input wire				      s_axis_rc_tvalid,

    input wire [AXIS_PCIE_DATA_WIDTH-1:0]     s_axis_cq_tdata,
    input wire [AXIS_PCIE_KEEP_WIDTH-1:0]     s_axis_cq_tkeep,
    input wire				      s_axis_cq_tlast,
    output wire				      s_axis_cq_tready,
    input wire [AXIS_PCIE_CQ_USER_WIDTH-1:0]  s_axis_cq_tuser,
    input wire				      s_axis_cq_tvalid,

    output wire [AXIS_PCIE_DATA_WIDTH-1:0]    m_axis_cc_tdata,
    output wire [AXIS_PCIE_KEEP_WIDTH-1:0]    m_axis_cc_tkeep,
    output wire				      m_axis_cc_tlast,
    input wire				      m_axis_cc_tready,
    output wire [AXIS_PCIE_CC_USER_WIDTH-1:0] m_axis_cc_tuser,
    output wire				      m_axis_cc_tvalid
);
  
pcie_us_axi_master #(
    .AXIS_PCIE_DATA_WIDTH(AXIS_PCIE_DATA_WIDTH),
    .AXIS_PCIE_KEEP_WIDTH(AXIS_PCIE_KEEP_WIDTH),
    .AXIS_PCIE_CQ_USER_WIDTH(AXIS_PCIE_CQ_USER_WIDTH),
    .AXIS_PCIE_CC_USER_WIDTH(AXIS_PCIE_CC_USER_WIDTH),
    .AXI_DATA_WIDTH(AXI_DATA_WIDTH),
    .AXI_ADDR_WIDTH(AXI_ADDR_WIDTH),
    .AXI_STRB_WIDTH(AXI_STRB_WIDTH),
    .AXI_ID_WIDTH(AXI_ID_WIDTH)
)
pcie_us_axi_master_inst (
    .clk(clk),
    .rst(rst),

    /*
     * AXI input (CQ)
     */
    .s_axis_cq_tdata(s_axis_cq_tdata),
    .s_axis_cq_tkeep(s_axis_cq_tkeep),
    .s_axis_cq_tvalid(s_axis_cq_tvalid),
    .s_axis_cq_tready(s_axis_cq_tready),
    .s_axis_cq_tlast(s_axis_cq_tlast),
    .s_axis_cq_tuser(s_axis_cq_tuser),

    /*
     * AXI output (CC)
     */
    .m_axis_cc_tdata(m_axis_cc_tdata),
    .m_axis_cc_tkeep(m_axis_cc_tkeep),
    .m_axis_cc_tvalid(m_axis_cc_tvalid),
    .m_axis_cc_tready(m_axis_cc_tready),
    .m_axis_cc_tlast(m_axis_cc_tlast),
    .m_axis_cc_tuser(m_axis_cc_tuser),

    /*
     * AXI Master output
     */
    .m_axi_awid(m_axi_h2c_awid),
    .m_axi_awaddr(m_axi_h2c_awaddr),
    .m_axi_awlen(m_axi_h2c_awlen),
    .m_axi_awsize(m_axi_h2c_awsize),
    .m_axi_awburst(m_axi_h2c_awburst),
    .m_axi_awlock(m_axi_h2c_awlock),
    .m_axi_awcache(m_axi_h2c_awcache),
    .m_axi_awprot(m_axi_h2c_awprot),
    .m_axi_awvalid(m_axi_h2c_awvalid),
    .m_axi_awready(m_axi_h2c_awready),
    .m_axi_wdata(m_axi_h2c_wdata),
    .m_axi_wstrb(m_axi_h2c_wstrb),
    .m_axi_wlast(m_axi_h2c_wlast),
    .m_axi_wvalid(m_axi_h2c_wvalid),
    .m_axi_wready(m_axi_h2c_wready),
    .m_axi_bid(m_axi_h2c_bid),
    .m_axi_bresp(m_axi_h2c_bresp),
    .m_axi_bvalid(m_axi_h2c_bvalid),
    .m_axi_bready(m_axi_h2c_bready),
    .m_axi_arid(m_axi_h2c_arid),
    .m_axi_araddr(m_axi_h2c_araddr),
    .m_axi_arlen(m_axi_h2c_arlen),
    .m_axi_arsize(m_axi_h2c_arsize),
    .m_axi_arburst(m_axi_h2c_arburst),
    .m_axi_arlock(m_axi_h2c_arlock),
    .m_axi_arcache(m_axi_h2c_arcache),
    .m_axi_arprot(m_axi_h2c_arprot),
    .m_axi_arvalid(m_axi_h2c_arvalid),
    .m_axi_arready(m_axi_h2c_arready),
    .m_axi_rid(m_axi_h2c_rid),
    .m_axi_rdata(m_axi_h2c_rdata),
    .m_axi_rresp(m_axi_h2c_rresp),
    .m_axi_rlast(m_axi_h2c_rlast),
    .m_axi_rvalid(m_axi_h2c_rvalid),
    .m_axi_rready(m_axi_h2c_rready),

    /*
     * Configuration
     */
    .completer_id({8'd0, 5'd0, 3'd1}),
    .completer_id_enable(1'b0),
    .max_payload_size(1024),

    /*
     * Status
     */
    .status_error_cor(),
    .status_error_uncor()
);
   
wire [AXIS_PCIE_DATA_WIDTH-1:0]    axis_rc_tdata_r;
wire [AXIS_PCIE_KEEP_WIDTH-1:0]    axis_rc_tkeep_r;
wire                               axis_rc_tlast_r;
wire                               axis_rc_tready_r;
wire [AXIS_PCIE_RC_USER_WIDTH-1:0] axis_rc_tuser_r;
wire                               axis_rc_tvalid_r;

axis_register #(
    .DATA_WIDTH(AXIS_PCIE_DATA_WIDTH),
    .KEEP_ENABLE(1),
    .KEEP_WIDTH(AXIS_PCIE_KEEP_WIDTH),
    .LAST_ENABLE(1),
    .ID_ENABLE(0),
    .DEST_ENABLE(0),
    .USER_ENABLE(1),
    .USER_WIDTH(AXIS_PCIE_RC_USER_WIDTH)
)
rc_reg (
    .clk(clk),
    .rst(rst),

    /*
     * AXI input
     */
    .s_axis_tdata(s_axis_rc_tdata),
    .s_axis_tkeep(s_axis_rc_tkeep),
    .s_axis_tvalid(s_axis_rc_tvalid),
    .s_axis_tready(s_axis_rc_tready),
    .s_axis_tlast(s_axis_rc_tlast),
    .s_axis_tid(0),
    .s_axis_tdest(0),
    .s_axis_tuser(s_axis_rc_tuser),

    /*
     * AXI output
     */
    .m_axis_tdata(axis_rc_tdata_r),
    .m_axis_tkeep(axis_rc_tkeep_r),
    .m_axis_tvalid(axis_rc_tvalid_r),
    .m_axis_tready(axis_rc_tready_r),
    .m_axis_tlast(axis_rc_tlast_r),
    .m_axis_tid(),
    .m_axis_tdest(),
    .m_axis_tuser(axis_rc_tuser_r)
);

pcie_us_axi_dma #(
    .AXIS_PCIE_DATA_WIDTH(AXIS_PCIE_DATA_WIDTH),
    .AXIS_PCIE_KEEP_WIDTH(AXIS_PCIE_KEEP_WIDTH),
    .AXIS_PCIE_RC_USER_WIDTH(AXIS_PCIE_RC_USER_WIDTH),
    .AXIS_PCIE_RQ_USER_WIDTH(AXIS_PCIE_RQ_USER_WIDTH),
    .AXI_DATA_WIDTH(AXI_DATA_WIDTH),
    .AXI_ADDR_WIDTH(AXI_ADDR_WIDTH),
    .AXI_STRB_WIDTH(AXI_STRB_WIDTH),
    .AXI_ID_WIDTH(AXI_ID_WIDTH),
    .AXI_MAX_BURST_LEN(16),
    .PCIE_ADDR_WIDTH(PCIE_ADDR_WIDTH),
    .PCIE_TAG_COUNT(256),
    .LEN_WIDTH(16),
    .TAG_WIDTH(DMA_TAG_WIDTH)
)
pcie_us_axi_dma_inst (
    .clk(clk),
    .rst(rst),

    /*
     * AXI input (RC)
     */
    .s_axis_rc_tdata(axis_rc_tdata_r),
    .s_axis_rc_tkeep(axis_rc_tkeep_r),
    .s_axis_rc_tvalid(axis_rc_tvalid_r),
    .s_axis_rc_tready(axis_rc_tready_r),
    .s_axis_rc_tlast(axis_rc_tlast_r),
    .s_axis_rc_tuser(axis_rc_tuser_r),
		      
    /*
     * AXI output (RQ)
     */
    .m_axis_rq_tdata(m_axis_rq_tdata),
    .m_axis_rq_tkeep(m_axis_rq_tkeep),
    .m_axis_rq_tvalid(m_axis_rq_tvalid),
    .m_axis_rq_tready(m_axis_rq_tready),
    .m_axis_rq_tlast(m_axis_rq_tlast),
    .m_axis_rq_tuser(m_axis_rq_tuser),

    /*
     * AXI read descriptor input
     */
    .s_axis_read_desc_pcie_addr(pcie_dma_read_desc_pcie_addr),
    .s_axis_read_desc_axi_addr(pcie_dma_read_desc_axi_addr),
    .s_axis_read_desc_len(pcie_dma_read_desc_len),
    .s_axis_read_desc_tag(pcie_dma_read_desc_tag),
    .s_axis_read_desc_valid(pcie_dma_read_desc_valid),
    .s_axis_read_desc_ready(pcie_dma_read_desc_ready),

    /*
     * AXI read descriptor status output
     */
    .m_axis_read_desc_status_tag(pcie_dma_read_desc_status_tag),
    .m_axis_read_desc_status_error(pcie_dma_read_desc_status_error),
    .m_axis_read_desc_status_valid(pcie_dma_read_desc_status_valid),

    /*
     * AXI write descriptor input
     */
    .s_axis_write_desc_pcie_addr(pcie_dma_write_desc_pcie_addr),
    .s_axis_write_desc_axi_addr(pcie_dma_write_desc_axi_addr),
    .s_axis_write_desc_len(pcie_dma_write_desc_len),
    .s_axis_write_desc_tag(pcie_dma_write_desc_tag),
    .s_axis_write_desc_valid(pcie_dma_write_desc_valid),
    .s_axis_write_desc_ready(pcie_dma_write_desc_ready),

    /*
     * AXI write descriptor status output
     */
    .m_axis_write_desc_status_tag(pcie_dma_write_desc_status_tag),
    .m_axis_write_desc_status_error(pcie_dma_write_desc_status_error),
    .m_axis_write_desc_status_valid(pcie_dma_write_desc_status_valid),

    /*
     * AXI Master output
     */
    .m_axi_awid(m_axi_c2h_awid),
    .m_axi_awaddr(m_axi_c2h_awaddr),
    .m_axi_awlen(m_axi_c2h_awlen),
    .m_axi_awsize(m_axi_c2h_awsize),
    .m_axi_awburst(m_axi_c2h_awburst),
    .m_axi_awlock(m_axi_c2h_awlock),
    .m_axi_awcache(m_axi_c2h_awcache),
    .m_axi_awprot(m_axi_c2h_awprot),
    .m_axi_awvalid(m_axi_c2h_awvalid),
    .m_axi_awready(m_axi_c2h_awready),
    .m_axi_wdata(m_axi_c2h_wdata),
    .m_axi_wstrb(m_axi_c2h_wstrb),
    .m_axi_wlast(m_axi_c2h_wlast),
    .m_axi_wvalid(m_axi_c2h_wvalid),
    .m_axi_wready(m_axi_c2h_wready),
    .m_axi_bid(m_axi_c2h_bid),
    .m_axi_bresp(m_axi_c2h_bresp),
    .m_axi_bvalid(m_axi_c2h_bvalid),
    .m_axi_bready(m_axi_c2h_bready),
    .m_axi_arid(m_axi_c2h_arid),
    .m_axi_araddr(m_axi_c2h_araddr),
    .m_axi_arlen(m_axi_c2h_arlen),
    .m_axi_arsize(m_axi_c2h_arsize),
    .m_axi_arburst(m_axi_c2h_arburst),
    .m_axi_arlock(m_axi_c2h_arlock),
    .m_axi_arcache(m_axi_c2h_arcache),
    .m_axi_arprot(m_axi_c2h_arprot),
    .m_axi_arvalid(m_axi_c2h_arvalid),
    .m_axi_arready(m_axi_c2h_arready),
    .m_axi_rid(m_axi_c2h_rid),
    .m_axi_rdata(m_axi_c2h_rdata),
    .m_axi_rresp(m_axi_c2h_rresp),
    .m_axi_rlast(m_axi_c2h_rlast),
    .m_axi_rvalid(m_axi_c2h_rvalid),
    .m_axi_rready(m_axi_c2h_rready),

    /*
     * Configuration
     */
    .read_enable(1'b1),
    .write_enable(1'b1),
    .ext_tag_enable(1'b1),
    .requester_id({8'd0, 5'd0, 3'd0}),
    .requester_id_enable(1'b0),
    .max_read_request_size(512),
    .max_payload_size(1024),

    /*
     * Status
     */
    .status_error_cor(),
    .status_error_uncor()
);
   
endmodule

`resetall
