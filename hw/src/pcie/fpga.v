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
 * FPGA top-level module
 */
module fpga #
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
     * PCI express
     */
     
    //(* X_INTERFACE_INFO = "xilinx.com:interface:pcie_7x_mgt_rtl:1.0 pci_express_x16 rxp" *)
    input wire [15:0]							     pcie_rx_p,
    //(* X_INTERFACE_INFO = "xilinx.com:interface:pcie_7x_mgt_rtl:1.0 pci_express_x16 rxn" *)
    input wire [15:0]							     pcie_rx_n,
    //(* X_INTERFACE_INFO = "xilinx.com:interface:pcie_7x_mgt_rtl:1.0 pci_express_x16 txp" *)
    output wire [15:0]							     pcie_tx_p,
    //(* X_INTERFACE_INFO = "xilinx.com:interface:pcie_7x_mgt_rtl:1.0 pci_express_x16 txn" *)
    output wire [15:0]							     pcie_tx_n,
    //(* X_INTERFACE_INFO = "xilinx.com:interface:diff_clock_rtl:1.0 diff_clk_in CLK_P" *)
    input wire								     pcie_refclk_p,
    //(* X_INTERFACE_INFO = "xilinx.com:interface:diff_clock_rtl:1.0 diff_clk_in CLK_N" *)
    input wire								     pcie_refclk_n,
    input wire								     pcie_reset_n,
 
    
    // Clock and reset
    output wire pcie_user_clk,
    output wire pcie_user_reset,
   
 
    /*
     * AXI Master output
     */
    output wire [AXI_ID_WIDTH-1:0]					     m_axi_h2c_awid,
    output wire [AXI_ADDR_WIDTH-1:0]					     m_axi_h2c_awaddr,
    output wire [7:0]							     m_axi_h2c_awlen,
    output wire [2:0]							     m_axi_h2c_awsize,
    output wire [1:0]							     m_axi_h2c_awburst,
    output wire								     m_axi_h2c_awlock,
    output wire [3:0]							     m_axi_h2c_awcache,
    output wire [2:0]							     m_axi_h2c_awprot,
    output wire								     m_axi_h2c_awvalid,
    input wire								     m_axi_h2c_awready,
    output wire [AXI_DATA_WIDTH-1:0]					     m_axi_h2c_wdata,
    output wire [AXI_STRB_WIDTH-1:0]					     m_axi_h2c_wstrb,
    output wire								     m_axi_h2c_wlast,
    output wire								     m_axi_h2c_wvalid,
    input wire								     m_axi_h2c_wready,
    input wire [AXI_ID_WIDTH-1:0]					     m_axi_h2c_bid,
    input wire [1:0]							     m_axi_h2c_bresp,
    input wire								     m_axi_h2c_bvalid,
    output wire								     m_axi_h2c_bready,
    output wire [AXI_ID_WIDTH-1:0]					     m_axi_h2c_arid,
    output wire [AXI_ADDR_WIDTH-1:0]					     m_axi_h2c_araddr,
    output wire [7:0]							     m_axi_h2c_arlen,
    output wire [2:0]							     m_axi_h2c_arsize,
    output wire [1:0]							     m_axi_h2c_arburst,
    output wire								     m_axi_h2c_arlock,
    output wire [3:0]							     m_axi_h2c_arcache,
    output wire [2:0]							     m_axi_h2c_arprot,
    output wire								     m_axi_h2c_arvalid,
    input wire								     m_axi_h2c_arready,
    input wire [AXI_ID_WIDTH-1:0]					     m_axi_h2c_rid,
    input wire [AXI_DATA_WIDTH-1:0]					     m_axi_h2c_rdata,
    input wire [1:0]							     m_axi_h2c_rresp,
    input wire								     m_axi_h2c_rlast,
    input wire								     m_axi_h2c_rvalid,
    output wire								     m_axi_h2c_rready,
  
    /*
     * AXI Master output
     */
    output wire [AXI_ID_WIDTH-1:0]					     m_axi_c2h_awid,
    output wire [AXI_ADDR_WIDTH-1:0]					     m_axi_c2h_awaddr,
    output wire [7:0]							     m_axi_c2h_awlen,
    output wire [2:0]							     m_axi_c2h_awsize,
    output wire [1:0]							     m_axi_c2h_awburst,
    output wire								     m_axi_c2h_awlock,
    output wire [3:0]							     m_axi_c2h_awcache,
    output wire [2:0]							     m_axi_c2h_awprot,
    output wire								     m_axi_c2h_awvalid,
    input wire								     m_axi_c2h_awready,
    output wire [AXI_DATA_WIDTH-1:0]					     m_axi_c2h_wdata,
    output wire [AXI_STRB_WIDTH-1:0]					     m_axi_c2h_wstrb,
    output wire								     m_axi_c2h_wlast,
    output wire								     m_axi_c2h_wvalid,
    input wire								     m_axi_c2h_wready,
    input wire [AXI_ID_WIDTH-1:0]					     m_axi_c2h_bid,
    input wire [1:0]							     m_axi_c2h_bresp,
    input wire								     m_axi_c2h_bvalid,
    output wire								     m_axi_c2h_bready,
    output wire [AXI_ID_WIDTH-1:0]					     m_axi_c2h_arid,
    output wire [AXI_ADDR_WIDTH-1:0]					     m_axi_c2h_araddr,
    output wire [7:0]							     m_axi_c2h_arlen,
    output wire [2:0]							     m_axi_c2h_arsize,
    output wire [1:0]							     m_axi_c2h_arburst,
    output wire								     m_axi_c2h_arlock,
    output wire [3:0]							     m_axi_c2h_arcache,
    output wire [2:0]							     m_axi_c2h_arprot,
    output wire								     m_axi_c2h_arvalid,
    input wire								     m_axi_c2h_arready,
    input wire [AXI_ID_WIDTH-1:0]					     m_axi_c2h_rid,
    input wire [AXI_DATA_WIDTH-1:0]					     m_axi_c2h_rdata,
    input wire [1:0]							     m_axi_c2h_rresp,
    input wire								     m_axi_c2h_rlast,
    input wire								     m_axi_c2h_rvalid,
    output wire								     m_axi_c2h_rready,
 
    input wire [PCIE_ADDR_WIDTH + AXI_ADDR_WIDTH + 16 + DMA_TAG_WIDTH - 1:0] pcie_dma_read_desc_tdata,
    input wire								     pcie_dma_read_desc_tvalid,
    output wire								     pcie_dma_read_desc_tready,

    output wire [DMA_TAG_WIDTH + 4 - 1:0]				     pcie_dma_read_desc_status_tdata,
    output wire								     pcie_dma_read_desc_status_tvalid,

    input wire [PCIE_ADDR_WIDTH + AXI_ADDR_WIDTH + 16 + DMA_TAG_WIDTH - 1:0] pcie_dma_write_desc_tdata, 
    input wire								     pcie_dma_write_desc_tvalid,
    output wire								     pcie_dma_write_desc_tready,

    output wire [DMA_TAG_WIDTH + 4 - 1:0]				     pcie_dma_write_desc_status_tdata,
    output wire								     pcie_dma_write_desc_status_tvalid,
 
 
output wire [AXIS_PCIE_DATA_WIDTH-1:0]    axis_rq_tdata,
output wire [AXIS_PCIE_KEEP_WIDTH-1:0]    axis_rq_tkeep,
output wire                               axis_rq_tlast,
output wire                               axis_rq_tready,
output wire [AXIS_PCIE_RQ_USER_WIDTH-1:0] axis_rq_tuser,
output wire                               axis_rq_tvalid,

output wire [AXIS_PCIE_DATA_WIDTH-1:0]    axis_rc_tdata,
output wire [AXIS_PCIE_KEEP_WIDTH-1:0]    axis_rc_tkeep,
output wire                               axis_rc_tlast,
output wire                               axis_rc_tready,
output wire [AXIS_PCIE_RC_USER_WIDTH-1:0] axis_rc_tuser,
output wire                               axis_rc_tvalid,

output wire [AXIS_PCIE_DATA_WIDTH-1:0]    axis_cq_tdata,
output wire [AXIS_PCIE_KEEP_WIDTH-1:0]    axis_cq_tkeep,
output wire                               axis_cq_tlast,
output wire                               axis_cq_tready,
output wire [AXIS_PCIE_CQ_USER_WIDTH-1:0] axis_cq_tuser,
output wire                               axis_cq_tvalid,

output wire [AXIS_PCIE_DATA_WIDTH-1:0]    axis_cc_tdata,
output wire [AXIS_PCIE_KEEP_WIDTH-1:0]    axis_cc_tkeep,
output wire                               axis_cc_tlast,
output wire                               axis_cc_tready,
output wire [AXIS_PCIE_CC_USER_WIDTH-1:0] axis_cc_tuser,
output wire                               axis_cc_tvalid
);
   wire [PCIE_ADDR_WIDTH-1:0]						     pcie_dma_read_desc_pcie_addr;
   wire [AXI_ADDR_WIDTH-1:0]						     pcie_dma_read_desc_axi_addr;
   wire [15:0]								     pcie_dma_read_desc_len;
   wire [DMA_TAG_WIDTH-1:0]						     pcie_dma_read_desc_tag; 
   assign {pcie_dma_read_desc_tag,  pcie_dma_read_desc_len, pcie_dma_read_desc_axi_addr, pcie_dma_read_desc_pcie_addr} = pcie_dma_read_desc_tdata;

   wire [DMA_TAG_WIDTH-1:0]						     pcie_dma_read_desc_status_tag;
   wire [3:0]								     pcie_dma_read_desc_status_error;
   assign pcie_dma_read_desc_status_tdata = {pcie_dma_read_desc_status_error, pcie_dma_read_desc_status_tag};

   wire [PCIE_ADDR_WIDTH-1:0]	      pcie_dma_write_desc_pcie_addr;
   wire [AXI_ADDR_WIDTH-1:0]	      pcie_dma_write_desc_axi_addr;
   wire [15:0]			      pcie_dma_write_desc_len;
   wire [DMA_TAG_WIDTH-1:0]	      pcie_dma_write_desc_tag;
   assign {pcie_dma_write_desc_tag,  pcie_dma_write_desc_len, pcie_dma_write_desc_axi_addr, pcie_dma_write_desc_pcie_addr} = pcie_dma_write_desc_tdata;
 
   wire [DMA_TAG_WIDTH-1:0]	      pcie_dma_write_desc_status_tag;
   wire [3:0]			      pcie_dma_write_desc_status_error;
   assign pcie_dma_write_desc_status_tdata = {pcie_dma_write_desc_status_error, pcie_dma_write_desc_status_tag};

// PCIe
wire pcie_sys_clk;
wire pcie_sys_clk_gt;

IBUFDS_GTE4 #(
    .REFCLK_HROW_CK_SEL(2'b00)
)
ibufds_gte4_pcie_mgt_refclk_inst (
    .I             (pcie_refclk_p),
    .IB            (pcie_refclk_n),
    .CEB           (1'b0),
    .O             (pcie_sys_clk_gt),
    .ODIV2         (pcie_sys_clk)
);

pcie4_uscale_plus_0
pcie4_uscale_plus_inst (
    .pci_exp_txn(pcie_tx_n),
    .pci_exp_txp(pcie_tx_p),
    .pci_exp_rxn(pcie_rx_n),
    .pci_exp_rxp(pcie_rx_p),
    .user_clk(pcie_user_clk),
    .user_reset(pcie_user_reset),
    .user_lnk_up(),

    .s_axis_rq_tdata(axis_rq_tdata),
    .s_axis_rq_tkeep(axis_rq_tkeep),
    .s_axis_rq_tlast(axis_rq_tlast),
    .s_axis_rq_tready(axis_rq_tready),
    .s_axis_rq_tuser(axis_rq_tuser),
    .s_axis_rq_tvalid(axis_rq_tvalid),

    .m_axis_rc_tdata(axis_rc_tdata),
    .m_axis_rc_tkeep(axis_rc_tkeep),
    .m_axis_rc_tlast(axis_rc_tlast),
    .m_axis_rc_tready(axis_rc_tready),
    .m_axis_rc_tuser(axis_rc_tuser),
    .m_axis_rc_tvalid(axis_rc_tvalid),

    .m_axis_cq_tdata(axis_cq_tdata),
    .m_axis_cq_tkeep(axis_cq_tkeep),
    .m_axis_cq_tlast(axis_cq_tlast),
    .m_axis_cq_tready(axis_cq_tready),
    .m_axis_cq_tuser(axis_cq_tuser),
    .m_axis_cq_tvalid(axis_cq_tvalid),

    .s_axis_cc_tdata(axis_cc_tdata),
    .s_axis_cc_tkeep(axis_cc_tkeep),
    .s_axis_cc_tlast(axis_cc_tlast),
    .s_axis_cc_tready(axis_cc_tready),
    .s_axis_cc_tuser(axis_cc_tuser),
    .s_axis_cc_tvalid(axis_cc_tvalid),

    .pcie_rq_seq_num0(),
    .pcie_rq_seq_num_vld0(),
    .pcie_rq_seq_num1(),
    .pcie_rq_seq_num_vld1(),
    .pcie_rq_tag0(),
    .pcie_rq_tag1(),
    .pcie_rq_tag_av(),
    .pcie_rq_tag_vld0(),
    .pcie_rq_tag_vld1(),

    .pcie_tfc_nph_av(),
    .pcie_tfc_npd_av(),

    .pcie_cq_np_req(1'b1),
    .pcie_cq_np_req_count(),

    .cfg_phy_link_down(),
    .cfg_phy_link_status(),
    .cfg_negotiated_width(),
    .cfg_current_speed(),
    .cfg_max_payload(1024),
    .cfg_max_read_req(512),
    .cfg_function_status(),
    .cfg_function_power_state(),
    .cfg_vf_status(),
    .cfg_vf_power_state(),
    .cfg_link_power_state(),

    .cfg_err_cor_out(),
    .cfg_err_nonfatal_out(),
    .cfg_err_fatal_out(),
    .cfg_local_error_valid(),
    .cfg_local_error_out(),
    .cfg_ltssm_state(),
    .cfg_rx_pm_state(),
    .cfg_tx_pm_state(),
    .cfg_rcb_status(),
    .cfg_obff_enable(),
    .cfg_pl_status_change(),
    .cfg_tph_requester_enable(),
    .cfg_tph_st_mode(),
    .cfg_vf_tph_requester_enable(),
    .cfg_vf_tph_st_mode(),

    .cfg_msg_received(),
    .cfg_msg_received_data(),
    .cfg_msg_received_type(),
    .cfg_msg_transmit(1'b0),
    .cfg_msg_transmit_type(3'd0),
    .cfg_msg_transmit_data(32'd0),
    .cfg_msg_transmit_done(),

    .cfg_fc_ph(),
    .cfg_fc_pd(),
    .cfg_fc_nph(),
    .cfg_fc_npd(),
    .cfg_fc_cplh(),
    .cfg_fc_cpld(),
    .cfg_fc_sel(3'd0),

    .cfg_dsn(64'd0),

    .cfg_bus_number(),

    .cfg_power_state_change_ack(1'b1),
    .cfg_power_state_change_interrupt(),

    .cfg_err_cor_in(),
    .cfg_err_uncor_in(),
    .cfg_flr_in_process(),
    .cfg_flr_done(4'd0),
    .cfg_vf_flr_in_process(),
    .cfg_vf_flr_func_num(8'd0),
    .cfg_vf_flr_done(8'd0),

    .cfg_link_training_enable(1'b1),

    .cfg_pm_aspm_l1_entry_reject(1'b0),
    .cfg_pm_aspm_tx_l0s_entry_disable(1'b0),

    .cfg_hot_reset_out(),

    .cfg_config_space_enable(1'b1),
    .cfg_req_pm_transition_l23_ready(1'b0),
    .cfg_hot_reset_in(1'b0),

    .cfg_ds_port_number(8'd0),
    .cfg_ds_bus_number(8'd0),
    .cfg_ds_device_number(5'd0),
    //.cfg_ds_function_number(3'd0),

    //.cfg_subsys_vend_id(16'h1234),
			
    .sys_clk(pcie_sys_clk),
    .sys_clk_gt(pcie_sys_clk_gt),
    .sys_reset(pcie_reset_n),

    .phy_rdy_out()
);

fpga_core #(
    .AXIS_PCIE_DATA_WIDTH(AXIS_PCIE_DATA_WIDTH),
    .AXIS_PCIE_KEEP_WIDTH(AXIS_PCIE_KEEP_WIDTH),
    .AXIS_PCIE_RC_USER_WIDTH(AXIS_PCIE_RC_USER_WIDTH),
    .AXIS_PCIE_RQ_USER_WIDTH(AXIS_PCIE_RQ_USER_WIDTH),
    .AXIS_PCIE_CQ_USER_WIDTH(AXIS_PCIE_CQ_USER_WIDTH),
    .AXIS_PCIE_CC_USER_WIDTH(AXIS_PCIE_CC_USER_WIDTH),
    .PCIE_ADDR_WIDTH(PCIE_ADDR_WIDTH),
    .AXI_DATA_WIDTH(AXI_DATA_WIDTH),
    .AXI_STRB_WIDTH(AXI_STRB_WIDTH),
    .AXI_ADDR_WIDTH(AXI_ADDR_WIDTH),
    .AXI_ID_WIDTH(AXI_ID_WIDTH),
    .DMA_TAG_WIDTH(DMA_TAG_WIDTH)
)
core_inst (
    /*
     * Clock: 250 MHz
     * Synchronous reset
     */
    .clk(pcie_user_clk),
    .rst(pcie_user_reset),
	
    .m_axi_h2c_awid(m_axi_h2c_awid),
    .m_axi_h2c_awaddr(m_axi_h2c_awaddr),
    .m_axi_h2c_awlen(m_axi_h2c_awlen),
    .m_axi_h2c_awsize(m_axi_h2c_awsize),
    .m_axi_h2c_awburst(m_axi_h2c_awburst),
    .m_axi_h2c_awlock(m_axi_h2c_awlock),
    .m_axi_h2c_awcache(m_axi_h2c_awcache),
    .m_axi_h2c_awprot(m_axi_h2c_awprot),
    .m_axi_h2c_awvalid(m_axi_h2c_awvalid),
    .m_axi_h2c_awready(m_axi_h2c_awready),
    .m_axi_h2c_wdata(m_axi_h2c_wdata),
    .m_axi_h2c_wstrb(m_axi_h2c_wstrb),
    .m_axi_h2c_wlast(m_axi_h2c_wlast),
    .m_axi_h2c_wvalid(m_axi_h2c_wvalid),
    .m_axi_h2c_wready(m_axi_h2c_wready),
    .m_axi_h2c_bid(m_axi_h2c_bid),
    .m_axi_h2c_bresp(m_axi_h2c_bresp),
    .m_axi_h2c_bvalid(m_axi_h2c_bvalid),
    .m_axi_h2c_bready(m_axi_h2c_bready),
    .m_axi_h2c_arid(m_axi_h2c_arid),
    .m_axi_h2c_araddr(m_axi_h2c_araddr),
    .m_axi_h2c_arlen(m_axi_h2c_arlen),
    .m_axi_h2c_arsize(m_axi_h2c_arsize),
    .m_axi_h2c_arburst(m_axi_h2c_arburst),
    .m_axi_h2c_arlock(m_axi_h2c_arlock),
    .m_axi_h2c_arcache(m_axi_h2c_arcache),
    .m_axi_h2c_arprot(m_axi_h2c_arprot),
    .m_axi_h2c_arvalid(m_axi_h2c_arvalid),
    .m_axi_h2c_arready(m_axi_h2c_arready),
    .m_axi_h2c_rid(m_axi_h2c_rid),
    .m_axi_h2c_rdata(m_axi_h2c_rdata),
    .m_axi_h2c_rresp(m_axi_h2c_rresp),
    .m_axi_h2c_rlast(m_axi_h2c_rlast),
    .m_axi_h2c_rvalid(m_axi_h2c_rvalid),
    .m_axi_h2c_rready(m_axi_h2c_rready),
  
    .m_axi_c2h_awid(m_axi_c2h_awid),
    .m_axi_c2h_awaddr(m_axi_c2h_awaddr),
    .m_axi_c2h_awlen(m_axi_c2h_awlen),
    .m_axi_c2h_awsize(m_axi_c2h_awsize),
    .m_axi_c2h_awburst(m_axi_c2h_awburst),
    .m_axi_c2h_awlock(m_axi_c2h_awlock),
    .m_axi_c2h_awcache(m_axi_c2h_awcache),
    .m_axi_c2h_awprot(m_axi_c2h_awprot),
    .m_axi_c2h_awvalid(m_axi_c2h_awvalid),
    .m_axi_c2h_awready(m_axi_c2h_awready),
    .m_axi_c2h_wdata(m_axi_c2h_wdata),
    .m_axi_c2h_wstrb(m_axi_c2h_wstrb),
    .m_axi_c2h_wlast(m_axi_c2h_wlast),
    .m_axi_c2h_wvalid(m_axi_c2h_wvalid),
    .m_axi_c2h_wready(m_axi_c2h_wready),
    .m_axi_c2h_bid(m_axi_c2h_bid),
    .m_axi_c2h_bresp(m_axi_c2h_bresp),
    .m_axi_c2h_bvalid(m_axi_c2h_bvalid),
    .m_axi_c2h_bready(m_axi_c2h_bready),
    .m_axi_c2h_arid(m_axi_c2h_arid),
    .m_axi_c2h_araddr(m_axi_c2h_araddr),
    .m_axi_c2h_arlen(m_axi_c2h_arlen),
    .m_axi_c2h_arsize(m_axi_c2h_arsize),
    .m_axi_c2h_arburst(m_axi_c2h_arburst),
    .m_axi_c2h_arlock(m_axi_c2h_arlock),
    .m_axi_c2h_arcache(m_axi_c2h_arcache),
    .m_axi_c2h_arprot(m_axi_c2h_arprot),
    .m_axi_c2h_arvalid(m_axi_c2h_arvalid),
    .m_axi_c2h_arready(m_axi_c2h_arready),
    .m_axi_c2h_rid(m_axi_c2h_rid),
    .m_axi_c2h_rdata(m_axi_c2h_rdata),
    .m_axi_c2h_rresp(m_axi_c2h_rresp),
    .m_axi_c2h_rlast(m_axi_c2h_rlast),
    .m_axi_c2h_rvalid(m_axi_c2h_rvalid),
    .m_axi_c2h_rready(m_axi_c2h_rready),

    .pcie_dma_read_desc_pcie_addr(pcie_dma_read_desc_pcie_addr),
    .pcie_dma_read_desc_axi_addr(pcie_dma_read_desc_axi_addr),
    .pcie_dma_read_desc_len(pcie_dma_read_desc_len),
    .pcie_dma_read_desc_tag(pcie_dma_read_desc_tag),
    .pcie_dma_read_desc_valid(pcie_dma_read_desc_tvalid),
    .pcie_dma_read_desc_ready(pcie_dma_read_desc_tready),
 
    .pcie_dma_read_desc_status_tag(pcie_dma_read_desc_status_tag),
    .pcie_dma_read_desc_status_error(pcie_dma_read_desc_status_error),
    .pcie_dma_read_desc_status_valid(pcie_dma_read_desc_status_tvalid),
 
    .pcie_dma_write_desc_pcie_addr(pcie_dma_write_desc_pcie_addr),
    .pcie_dma_write_desc_axi_addr(pcie_dma_write_desc_axi_addr),
    .pcie_dma_write_desc_len(pcie_dma_write_desc_len),
    .pcie_dma_write_desc_tag(pcie_dma_write_desc_tag),
    .pcie_dma_write_desc_valid(pcie_dma_write_desc_tvalid),
    .pcie_dma_write_desc_ready(pcie_dma_write_desc_tready),
 
    .pcie_dma_write_desc_status_tag(pcie_dma_write_desc_status_tag),
    .pcie_dma_write_desc_status_error(pcie_dma_write_desc_status_error),
    .pcie_dma_write_desc_status_valid(pcie_dma_write_desc_status_tvalid),
	   
    /*
     * PCIe
     */
    .m_axis_rq_tdata(axis_rq_tdata),
    .m_axis_rq_tkeep(axis_rq_tkeep),
    .m_axis_rq_tlast(axis_rq_tlast),
    .m_axis_rq_tready(axis_rq_tready),
    .m_axis_rq_tuser(axis_rq_tuser),
    .m_axis_rq_tvalid(axis_rq_tvalid),

    .s_axis_rc_tdata(axis_rc_tdata),
    .s_axis_rc_tkeep(axis_rc_tkeep),
    .s_axis_rc_tlast(axis_rc_tlast),
    .s_axis_rc_tready(axis_rc_tready),
    .s_axis_rc_tuser(axis_rc_tuser),
    .s_axis_rc_tvalid(axis_rc_tvalid),

    .s_axis_cq_tdata(axis_cq_tdata),
    .s_axis_cq_tkeep(axis_cq_tkeep),
    .s_axis_cq_tlast(axis_cq_tlast),
    .s_axis_cq_tready(axis_cq_tready),
    .s_axis_cq_tuser(axis_cq_tuser),
    .s_axis_cq_tvalid(axis_cq_tvalid),

    .m_axis_cc_tdata(axis_cc_tdata),
    .m_axis_cc_tkeep(axis_cc_tkeep),
    .m_axis_cc_tlast(axis_cc_tlast),
    .m_axis_cc_tready(axis_cc_tready),
    .m_axis_cc_tuser(axis_cc_tuser),
    .m_axis_cc_tvalid(axis_cc_tvalid)
);

endmodule

`resetall
