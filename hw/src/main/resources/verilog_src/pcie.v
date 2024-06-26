`timescale 1ns / 1ps

`default_nettype none

module pcie_rtl#
  (
   parameter AXIS_PCIE_DATA_WIDTH = 512,
   parameter AXIS_PCIE_KEEP_WIDTH = (AXIS_PCIE_DATA_WIDTH/32),
   parameter AXIS_PCIE_RC_USER_WIDTH = AXIS_PCIE_DATA_WIDTH < 512 ? 75 : 161,
   parameter AXIS_PCIE_RQ_USER_WIDTH = AXIS_PCIE_DATA_WIDTH < 512 ? 60 : 137,
   parameter AXIS_PCIE_CQ_USER_WIDTH = AXIS_PCIE_DATA_WIDTH < 512 ? 85 : 183,
   parameter AXIS_PCIE_CC_USER_WIDTH = AXIS_PCIE_DATA_WIDTH < 512 ? 33 : 81,
   parameter RC_STRADDLE = AXIS_PCIE_DATA_WIDTH >= 256,
   parameter RQ_STRADDLE = AXIS_PCIE_DATA_WIDTH >= 512,
   parameter CQ_STRADDLE = AXIS_PCIE_DATA_WIDTH >= 512,
   parameter CC_STRADDLE = AXIS_PCIE_DATA_WIDTH >= 512,
   parameter RQ_SEQ_NUM_WIDTH = AXIS_PCIE_RQ_USER_WIDTH == 60 ? 4 : 6,
   parameter RQ_SEQ_NUM_ENABLE = 1,
   parameter IMM_ENABLE = 0,
   parameter IMM_WIDTH = 32,
   parameter PCIE_TAG_COUNT = 256,
   parameter READ_OP_TABLE_SIZE = PCIE_TAG_COUNT,
   parameter READ_TX_LIMIT = 2**(RQ_SEQ_NUM_WIDTH-1),
   parameter READ_CPLH_FC_LIMIT = AXIS_PCIE_RQ_USER_WIDTH == 60 ? 64 : 256,
   parameter READ_CPLD_FC_LIMIT = AXIS_PCIE_RQ_USER_WIDTH == 60 ? 1024-64 : 2048-256,
   parameter WRITE_OP_TABLE_SIZE = 2**(RQ_SEQ_NUM_WIDTH-1),
   parameter WRITE_TX_LIMIT = 2**(RQ_SEQ_NUM_WIDTH-1),
   parameter TLP_DATA_WIDTH = AXIS_PCIE_DATA_WIDTH,
   parameter TLP_STRB_WIDTH = TLP_DATA_WIDTH/32,
   parameter TLP_HDR_WIDTH = 128,
   parameter TLP_SEG_COUNT = 1,
   parameter TX_SEQ_NUM_COUNT = AXIS_PCIE_DATA_WIDTH < 512 ? 1 : 2,
   parameter TX_SEQ_NUM_WIDTH = RQ_SEQ_NUM_WIDTH-1,
   parameter TX_SEQ_NUM_ENABLE = RQ_SEQ_NUM_ENABLE,
   parameter PF_COUNT = 1,
   parameter VF_COUNT = 0,
   parameter F_COUNT = PF_COUNT+VF_COUNT,
   parameter TLP_FORCE_64_BIT_ADDR = 0,
   parameter CHECK_BUS_NUMBER = 0,
   parameter RAM_SEL_WIDTH = 2,
   parameter RAM_ADDR_WIDTH = 18,
   parameter RAM_SEG_COUNT = TLP_SEG_COUNT*2,
   parameter RAM_SEG_DATA_WIDTH = TLP_DATA_WIDTH*2/RAM_SEG_COUNT,
   parameter RAM_SEG_BE_WIDTH = RAM_SEG_DATA_WIDTH/8,
   parameter RAM_SEG_ADDR_WIDTH = RAM_ADDR_WIDTH-$clog2(RAM_SEG_COUNT*RAM_SEG_BE_WIDTH),
   parameter AXI_DATA_WIDTH = TLP_DATA_WIDTH,
   parameter AXI_STRB_WIDTH = (AXI_DATA_WIDTH/8),
   parameter AXI_ADDR_WIDTH = 26,
   parameter AXI_ID_WIDTH = 8,
   parameter PCIE_ADDR_WIDTH = 64,
   parameter LEN_WIDTH = 16,
   parameter TAG_WIDTH = 8,
   parameter PORTS = 2,
   parameter S_TAG_WIDTH = 8,
   parameter M_TAG_WIDTH = 8 + $clog2(PORTS),
   parameter S_RAM_SEL_WIDTH = 2,
   parameter M_RAM_SEL_WIDTH = 2 + $clog2(PORTS)
   )
   (
    input wire [15:0]					     pcie_rx_p,
    input wire [15:0]					     pcie_rx_n,
    output wire [15:0]					     pcie_tx_p,
    output wire [15:0]					     pcie_tx_n,
   
    input wire						     pcie_refclk_p,
    input wire						     pcie_refclk_n,
    input wire						     pcie_reset_n,
   
    output wire						     pcie_user_clk,
    output wire						     pcie_user_reset,

    output wire [AXI_ID_WIDTH-1:0]			     m_axi_awid,
    output wire [AXI_ADDR_WIDTH-1:0]			     m_axi_awaddr,
    output wire [7:0]					     m_axi_awlen,
    output wire [2:0]					     m_axi_awsize,
    output wire [1:0]					     m_axi_awburst,
    output wire						     m_axi_awlock,
    output wire [3:0]					     m_axi_awcache,
    output wire [2:0]					     m_axi_awprot,
    output wire						     m_axi_awvalid,
    output wire [3:0]					     m_axi_awregion,
    output wire [3:0]					     m_axi_awqos,
    input wire						     m_axi_awready,
   
    output wire [AXI_DATA_WIDTH-1:0]			     m_axi_wdata,
    output wire [AXI_STRB_WIDTH-1:0]			     m_axi_wstrb,
    output wire						     m_axi_wlast,
    output wire						     m_axi_wvalid,
    input wire						     m_axi_wready,
    input wire [AXI_ID_WIDTH-1:0]			     m_axi_bid,
    input wire [1:0]					     m_axi_bresp,
    input wire						     m_axi_bvalid,
    output wire						     m_axi_bready,
    output wire [AXI_ID_WIDTH-1:0]			     m_axi_arid,
    output wire [AXI_ADDR_WIDTH-1:0]			     m_axi_araddr,
    output wire [7:0]					     m_axi_arlen,
    output wire [2:0]					     m_axi_arsize,
    output wire [1:0]					     m_axi_arburst,
    output wire						     m_axi_arlock,
    output wire [3:0]					     m_axi_arcache,
    output wire [2:0]					     m_axi_arprot,
    output wire						     m_axi_arvalid,
    output wire [3:0]					     m_axi_arqos,
    output wire [3:0]					     m_axi_arregion,
    input wire						     m_axi_arready,
    input wire [AXI_ID_WIDTH-1:0]			     m_axi_rid,
    input wire [AXI_DATA_WIDTH-1:0]			     m_axi_rdata,
    input wire [1:0]					     m_axi_rresp,
    input wire						     m_axi_rlast,
    input wire						     m_axi_rvalid,
    output wire						     m_axi_rready,

    input wire [PORTS*PCIE_ADDR_WIDTH-1:0]		     dma_read_desc_pcie_addr,
    input wire [PORTS*S_RAM_SEL_WIDTH-1:0]		     dma_read_desc_ram_sel,
    input wire [PORTS*RAM_ADDR_WIDTH-1:0]		     dma_read_desc_ram_addr,
    input wire [PORTS*LEN_WIDTH-1:0]			     dma_read_desc_len,
    input wire [PORTS*S_TAG_WIDTH-1:0]			     dma_read_desc_tag,
    input wire [PORTS-1:0]				     dma_read_desc_valid,
    output wire [PORTS-1:0]				     dma_read_desc_ready,

    output wire [PORTS*S_TAG_WIDTH-1:0]			     dma_read_desc_status_tag,
    output wire [PORTS*4-1:0]				     dma_read_desc_status_error,
    output wire [PORTS-1:0]				     dma_read_desc_status_valid,
    input wire						     dma_read_desc_status_ready,

    input wire [PORTS*PCIE_ADDR_WIDTH-1:0]		     dma_write_desc_pcie_addr,
    input wire [PORTS*S_RAM_SEL_WIDTH-1:0]		     dma_write_desc_ram_sel,
    input wire [PORTS*RAM_ADDR_WIDTH-1:0]		     dma_write_desc_ram_addr,
    input wire [PORTS*IMM_WIDTH-1:0]			     dma_write_desc_imm,
    input wire [PORTS-1:0]				     dma_write_desc_imm_en,
    input wire [PORTS*LEN_WIDTH-1:0]			     dma_write_desc_len,
    input wire [PORTS*S_TAG_WIDTH-1:0]			     dma_write_desc_tag,
    input wire [PORTS-1:0]				     dma_write_desc_valid,
    output wire [PORTS-1:0]				     dma_write_desc_ready,

    output wire [PORTS*S_TAG_WIDTH-1:0]			     dma_write_desc_status_tag,
    output wire [PORTS*4-1:0]				     dma_write_desc_status_error,
    output wire [PORTS-1:0]				     dma_write_desc_status_valid,
    input wire						     dma_write_desc_status_ready,
    
    output wire [PORTS*RAM_SEG_COUNT*S_RAM_SEL_WIDTH-1:0]    ram_wr_cmd_sel,
    output wire [PORTS*RAM_SEG_COUNT*RAM_SEG_BE_WIDTH-1:0]   ram_wr_cmd_be,
    output wire [PORTS*RAM_SEG_COUNT*RAM_SEG_ADDR_WIDTH-1:0] ram_wr_cmd_addr,
    output wire [PORTS*RAM_SEG_COUNT*RAM_SEG_DATA_WIDTH-1:0] ram_wr_cmd_data,
    output wire [PORTS*RAM_SEG_COUNT-1:0]		     ram_wr_cmd_valid,
    input wire [PORTS*RAM_SEG_COUNT-1:0]		     ram_wr_cmd_ready,
    input wire [PORTS*RAM_SEG_COUNT-1:0]		     ram_wr_done,
    output wire [PORTS*RAM_SEG_COUNT*S_RAM_SEL_WIDTH-1:0]    ram_rd_cmd_sel,
    output wire [PORTS*RAM_SEG_COUNT*RAM_SEG_ADDR_WIDTH-1:0] ram_rd_cmd_addr,
    output wire [PORTS*RAM_SEG_COUNT-1:0]		     ram_rd_cmd_valid,
    input wire [PORTS*RAM_SEG_COUNT-1:0]		     ram_rd_cmd_ready,
    input wire [PORTS*RAM_SEG_COUNT*RAM_SEG_DATA_WIDTH-1:0]  ram_rd_resp_data,
    input wire [PORTS*RAM_SEG_COUNT-1:0]		     ram_rd_resp_valid,
    output wire [PORTS*RAM_SEG_COUNT-1:0]		     ram_rd_resp_ready
    );

   wire [PCIE_ADDR_WIDTH-1:0]			      m_axis_read_desc_dma_addr;
   wire [M_RAM_SEL_WIDTH-1:0]			      m_axis_read_desc_ram_sel;
   wire [RAM_ADDR_WIDTH-1:0]			      m_axis_read_desc_ram_addr;
   wire [LEN_WIDTH-1:0]				      m_axis_read_desc_len;
   wire [M_TAG_WIDTH-1:0]			      m_axis_read_desc_tag;
   wire						      m_axis_read_desc_valid;
   wire						      m_axis_read_desc_ready;
   

   wire [M_TAG_WIDTH-1:0]			      s_axis_read_desc_status_tag;
   wire [3:0]					      s_axis_read_desc_status_error;
   wire						      s_axis_read_desc_status_valid;
   

   wire [PCIE_ADDR_WIDTH-1:0]			      m_axis_write_desc_dma_addr;
   wire [M_RAM_SEL_WIDTH-1:0]			      m_axis_write_desc_ram_sel;
   wire [RAM_ADDR_WIDTH-1:0]			      m_axis_write_desc_ram_addr;
   wire [IMM_WIDTH-1:0]				      m_axis_write_desc_imm;
   wire						      m_axis_write_desc_imm_en;
   wire [LEN_WIDTH-1:0]				      m_axis_write_desc_len;
   wire [M_TAG_WIDTH-1:0]			      m_axis_write_desc_tag;
   wire						      m_axis_write_desc_valid;
   wire						      m_axis_write_desc_ready;

   wire [M_TAG_WIDTH-1:0]			      s_axis_write_desc_status_tag;
   wire [3:0]					      s_axis_write_desc_status_error;
   wire						      s_axis_write_desc_status_valid;

   wire [RAM_SEG_COUNT*M_RAM_SEL_WIDTH-1:0]	      if_ram_wr_cmd_sel;
   wire [RAM_SEG_COUNT*RAM_SEG_BE_WIDTH-1:0]	      if_ram_wr_cmd_be;
   wire [RAM_SEG_COUNT*RAM_SEG_ADDR_WIDTH-1:0]	      if_ram_wr_cmd_addr;
   wire [RAM_SEG_COUNT*RAM_SEG_DATA_WIDTH-1:0]	      if_ram_wr_cmd_data;
   wire [RAM_SEG_COUNT-1:0]			      if_ram_wr_cmd_valid;
   wire [RAM_SEG_COUNT-1:0]			      if_ram_wr_cmd_ready;
   wire [RAM_SEG_COUNT-1:0]			      if_ram_wr_done;
   wire [RAM_SEG_COUNT*M_RAM_SEL_WIDTH-1:0]	      if_ram_rd_cmd_sel;
   wire [RAM_SEG_COUNT*RAM_SEG_ADDR_WIDTH-1:0]	      if_ram_rd_cmd_addr;
   wire [RAM_SEG_COUNT-1:0]			      if_ram_rd_cmd_valid;
   wire [RAM_SEG_COUNT-1:0]			      if_ram_rd_cmd_ready;
   wire [RAM_SEG_COUNT*RAM_SEG_DATA_WIDTH-1:0]	      if_ram_rd_resp_data;
   wire [RAM_SEG_COUNT-1:0]			      if_ram_rd_resp_valid;
   wire [RAM_SEG_COUNT-1:0]			      if_ram_rd_resp_ready;
  
   wire [2:0]			      cfg_max_payload;
   wire [2:0]			      cfg_max_read_req;
   wire [3:0]			      cfg_rcb_status;
   wire [F_COUNT-1:0]		      ext_tag_enable;
   wire [F_COUNT*3-1:0]		      max_read_request_size;
   wire [F_COUNT*3-1:0]		      max_payload_size;
   
   wire [RQ_SEQ_NUM_WIDTH-1:0]	      s_axis_rq_seq_num_0;
   wire				      s_axis_rq_seq_num_valid_0;
   wire [RQ_SEQ_NUM_WIDTH-1:0]	      s_axis_rq_seq_num_1;
   wire				      s_axis_rq_seq_num_valid_1;
   
   wire [9:0]			      cfg_mgmt_addr;
   wire [7:0]			      cfg_mgmt_function_number;
   wire				      cfg_mgmt_write;
   wire [31:0]			      cfg_mgmt_write_data;
   wire [3:0]			      cfg_mgmt_byte_enable;
   wire				      cfg_mgmt_read;
   wire [31:0]			      cfg_mgmt_read_data;
   wire				      cfg_mgmt_read_write_done;

   wire [7:0]			      cfg_fc_ph;
   wire [11:0]			      cfg_fc_pd;
   wire [7:0]			      cfg_fc_nph;
   wire [11:0]			      cfg_fc_npd;
   wire [7:0]			      cfg_fc_cplh;
   wire [11:0]			      cfg_fc_cpld;
   wire [2:0]			      cfg_fc_sel;
   
   wire [3:0]			      cfg_interrupt_msi_enable;
   wire [7:0]			      cfg_interrupt_msi_vf_enable;
   wire [11:0]			      cfg_interrupt_msi_mmenable;
   wire				      cfg_interrupt_msi_mask_update;
   wire [31:0]			      cfg_interrupt_msi_data;
   wire [3:0]			      cfg_interrupt_msi_select;
   wire [31:0]			      cfg_interrupt_msi_int;
   wire [31:0]			      cfg_interrupt_msi_pending_status;
   wire				      cfg_interrupt_msi_pending_status_data_enable;
   wire [3:0]			      cfg_interrupt_msi_pending_status_function_num;
   wire				      cfg_interrupt_msi_sent;
   wire				      cfg_interrupt_msi_fail;
   wire [3:0]			      cfg_interrupt_msix_enable;
   wire [3:0]			      cfg_interrupt_msix_mask;
   wire [251:0]			      cfg_interrupt_msix_vf_enable;
   wire [251:0]			      cfg_interrupt_msix_vf_mask;
   wire [63:0]			      cfg_interrupt_msix_address;
   wire [31:0]			      cfg_interrupt_msix_data;
   wire				      cfg_interrupt_msix_int;
   wire [1:0]			      cfg_interrupt_msix_vec_pending;
   wire				      cfg_interrupt_msix_vec_pending_status;
   wire				      cfg_interrupt_msix_sent;
   wire				      cfg_interrupt_msix_fail;
   wire [2:0]			      cfg_interrupt_msi_attr;
   wire				      cfg_interrupt_msi_tph_present;
   wire [1:0]			      cfg_interrupt_msi_tph_type;
   wire [8:0]			      cfg_interrupt_msi_tph_st_tag;
   wire [7:0]			      cfg_interrupt_msi_function_number;
   
   wire [AXIS_PCIE_DATA_WIDTH-1:0]    axis_rq_tdata;
   wire [AXIS_PCIE_KEEP_WIDTH-1:0]    axis_rq_tkeep;
   wire				      axis_rq_tlast;
   wire				      axis_rq_tready;
   wire [AXIS_PCIE_RQ_USER_WIDTH-1:0] axis_rq_tuser;
   wire				      axis_rq_tvalid;
   
   wire [AXIS_PCIE_DATA_WIDTH-1:0]    axis_rc_tdata;
   wire [AXIS_PCIE_KEEP_WIDTH-1:0]    axis_rc_tkeep;
   wire				      axis_rc_tlast;
   wire				      axis_rc_tready;
   wire [AXIS_PCIE_RC_USER_WIDTH-1:0] axis_rc_tuser;
   wire				      axis_rc_tvalid;
   
   wire [AXIS_PCIE_DATA_WIDTH-1:0]    axis_cq_tdata;
   wire [AXIS_PCIE_KEEP_WIDTH-1:0]    axis_cq_tkeep;
   wire				      axis_cq_tlast;
   wire				      axis_cq_tready;
   wire [AXIS_PCIE_CQ_USER_WIDTH-1:0] axis_cq_tuser;
   wire				      axis_cq_tvalid;
   
   wire [AXIS_PCIE_DATA_WIDTH-1:0]    axis_cc_tdata;
   wire [AXIS_PCIE_KEEP_WIDTH-1:0]    axis_cc_tkeep;
   wire				      axis_cc_tlast;
   wire				      axis_cc_tready;
   wire [AXIS_PCIE_CC_USER_WIDTH-1:0] axis_cc_tuser;
   wire				      axis_cc_tvalid;

   wire [TLP_DATA_WIDTH-1:0]	      rx_cpl_tlp_data;
   wire [TLP_STRB_WIDTH-1:0]	      rx_cpl_tlp_strb;
   wire [TLP_SEG_COUNT*TLP_HDR_WIDTH-1:0] rx_cpl_tlp_hdr;
   wire [TLP_SEG_COUNT*4-1:0]		  rx_cpl_tlp_error;
   wire [TLP_SEG_COUNT-1:0]		  rx_cpl_tlp_valid;
   wire [TLP_SEG_COUNT-1:0]		  rx_cpl_tlp_sop;
   wire [TLP_SEG_COUNT-1:0]		  rx_cpl_tlp_eop;
   wire					  rx_cpl_tlp_ready;

   wire [TLP_DATA_WIDTH-1:0]		  rx_req_tlp_data;
   wire [TLP_STRB_WIDTH-1:0]		  rx_req_tlp_strb;
   wire [TLP_SEG_COUNT*TLP_HDR_WIDTH-1:0] rx_req_tlp_hdr;
   wire [TLP_SEG_COUNT*3-1:0]		  rx_req_tlp_bar_id;
   wire [TLP_SEG_COUNT*8-1:0]		  rx_req_tlp_func_num;
   wire [TLP_SEG_COUNT-1:0]		  rx_req_tlp_valid;
   wire [TLP_SEG_COUNT-1:0]		  rx_req_tlp_sop;
   wire [TLP_SEG_COUNT-1:0]		  rx_req_tlp_eop;
   wire					  rx_req_tlp_ready;
   
   wire [TLP_SEG_COUNT*TLP_HDR_WIDTH-1:0] tx_rd_req_tlp_hdr;
   wire [TLP_SEG_COUNT*TX_SEQ_NUM_WIDTH-1:0] tx_rd_req_tlp_seq;
   wire [TLP_SEG_COUNT-1:0]		     tx_rd_req_tlp_valid;
   wire [TLP_SEG_COUNT-1:0]		     tx_rd_req_tlp_sop;
   wire [TLP_SEG_COUNT-1:0]		     tx_rd_req_tlp_eop;
   wire					     tx_rd_req_tlp_ready;

   wire [TLP_DATA_WIDTH-1:0]		     tx_wr_req_tlp_data;
   wire [TLP_STRB_WIDTH-1:0]		     tx_wr_req_tlp_strb;
   wire [TLP_SEG_COUNT*TLP_HDR_WIDTH-1:0]    tx_wr_req_tlp_hdr;
   wire [TLP_SEG_COUNT*TX_SEQ_NUM_WIDTH-1:0] tx_wr_req_tlp_seq;
   wire [TLP_SEG_COUNT-1:0]		     tx_wr_req_tlp_valid;
   wire [TLP_SEG_COUNT-1:0]		     tx_wr_req_tlp_sop;
   wire [TLP_SEG_COUNT-1:0]		     tx_wr_req_tlp_eop;
   wire					     tx_wr_req_tlp_ready;

   wire [TLP_DATA_WIDTH-1:0]		     tx_cpl_tlp_data;
   wire [TLP_STRB_WIDTH-1:0]		     tx_cpl_tlp_strb;
   wire [TLP_SEG_COUNT*TLP_HDR_WIDTH-1:0]    tx_cpl_tlp_hdr;
   wire [TLP_SEG_COUNT-1:0]		     tx_cpl_tlp_valid;
   wire [TLP_SEG_COUNT-1:0]		     tx_cpl_tlp_sop;
   wire [TLP_SEG_COUNT-1:0]		     tx_cpl_tlp_eop;
   wire					     tx_cpl_tlp_ready;
   
   wire [TX_SEQ_NUM_COUNT*TX_SEQ_NUM_WIDTH-1:0]	axis_rd_req_tx_seq_num;
   wire [TX_SEQ_NUM_COUNT-1:0]			axis_rd_req_tx_seq_num_valid;

   wire [TX_SEQ_NUM_COUNT*TX_SEQ_NUM_WIDTH-1:0]	axis_wr_req_tx_seq_num;
   wire [TX_SEQ_NUM_COUNT-1:0]			axis_wr_req_tx_seq_num_valid;
   
   wire						pcie_sys_clk;
   wire						pcie_sys_clk_gt;

   IBUFDS_GTE4 #(
		 .REFCLK_HROW_CK_SEL(2'b00)
		 )
   ibufds_inst (
		.I             (pcie_refclk_p),
		.IB            (pcie_refclk_n),
		.CEB           (1'b0),
		.O             (pcie_sys_clk_gt),
		.ODIV2         (pcie_sys_clk)
		);

   dma_if_mux # (
		 .PORTS(PORTS),
		 .SEG_COUNT(RAM_SEG_COUNT),
		 .SEG_DATA_WIDTH(RAM_SEG_DATA_WIDTH),
		 .SEG_ADDR_WIDTH(RAM_SEG_ADDR_WIDTH),
		 .SEG_BE_WIDTH(RAM_SEG_BE_WIDTH),
		 .S_RAM_SEL_WIDTH(S_RAM_SEL_WIDTH),
		 .M_RAM_SEL_WIDTH(M_RAM_SEL_WIDTH),
		 .RAM_ADDR_WIDTH(RAM_SEG_ADDR_WIDTH+$clog2(RAM_SEG_COUNT)+$clog2(RAM_SEG_BE_WIDTH)),
		 .DMA_ADDR_WIDTH(PCIE_ADDR_WIDTH),
		 .IMM_ENABLE(0),
		 .IMM_WIDTH(32),
		 .LEN_WIDTH(LEN_WIDTH),
		 .S_TAG_WIDTH(S_TAG_WIDTH),
		 .M_TAG_WIDTH(M_TAG_WIDTH),
		 .ARB_TYPE_ROUND_ROBIN(0),
		 .ARB_LSB_HIGH_PRIORITY(1)
		 )
   dma_if_mux_inst (
		    .clk(pcie_user_clk),
		    .rst(pcie_user_reset),
		    
		    .m_axis_read_desc_dma_addr(m_axis_read_desc_dma_addr),
		    .m_axis_read_desc_ram_sel(m_axis_read_desc_ram_sel),
		    .m_axis_read_desc_ram_addr(m_axis_read_desc_ram_addr),
		    .m_axis_read_desc_len(m_axis_read_desc_len),
		    .m_axis_read_desc_tag(m_axis_read_desc_tag),
		    .m_axis_read_desc_valid(m_axis_read_desc_valid),
		    .m_axis_read_desc_ready(m_axis_read_desc_ready),
      
		    .s_axis_read_desc_status_tag(s_axis_read_desc_status_tag),
		    .s_axis_read_desc_status_error(s_axis_read_desc_status_error),
		    .s_axis_read_desc_status_valid(s_axis_read_desc_status_valid),

		    .m_axis_write_desc_dma_addr(m_axis_write_desc_dma_addr),
		    .m_axis_write_desc_ram_sel(m_axis_write_desc_ram_sel),
		    .m_axis_write_desc_ram_addr(m_axis_write_desc_ram_addr),
		    .m_axis_write_desc_imm(m_axis_write_desc_imm),
		    .m_axis_write_desc_imm_en(m_axis_write_desc_imm_en),
		    .m_axis_write_desc_len(m_axis_write_desc_len),
		    .m_axis_write_desc_tag(m_axis_write_desc_tag),
		    .m_axis_write_desc_valid(m_axis_write_desc_valid),
		    .m_axis_write_desc_ready(m_axis_write_desc_ready),
      
		    .s_axis_write_desc_status_tag(s_axis_write_desc_status_tag),
		    .s_axis_write_desc_status_error(s_axis_write_desc_status_error),
		    .s_axis_write_desc_status_valid(s_axis_write_desc_status_valid),
      
		    .s_axis_read_desc_dma_addr(dma_read_desc_pcie_addr), 
		    .s_axis_read_desc_ram_sel(dma_read_desc_ram_sel),
		    .s_axis_read_desc_ram_addr(dma_read_desc_ram_addr),
		    .s_axis_read_desc_len(dma_read_desc_len),      
		    .s_axis_read_desc_tag(dma_read_desc_tag),       
		    .s_axis_read_desc_valid(dma_read_desc_valid),         
		    .s_axis_read_desc_ready(dma_read_desc_ready),          
      
		    .m_axis_read_desc_status_tag(dma_read_desc_status_tag),
		    .m_axis_read_desc_status_error(dma_read_desc_status_error),
		    .m_axis_read_desc_status_valid(dma_read_desc_status_valid),

		    .s_axis_write_desc_dma_addr(dma_write_desc_pcie_addr),
		    .s_axis_write_desc_ram_sel(dma_write_desc_ram_sel),
		    .s_axis_write_desc_ram_addr(dma_write_desc_ram_addr),
		    .s_axis_write_desc_imm(0),
		    .s_axis_write_desc_imm_en(0),
		    .s_axis_write_desc_len(dma_write_desc_len),
		    .s_axis_write_desc_tag(dma_write_desc_tag),
		    .s_axis_write_desc_valid(dma_write_desc_valid),
		    .s_axis_write_desc_ready(dma_write_desc_ready),
      
		    .m_axis_write_desc_status_tag(dma_write_desc_status_tag),
		    .m_axis_write_desc_status_error(dma_write_desc_status_error),
		    .m_axis_write_desc_status_valid(dma_write_desc_status_valid),
      
		    .if_ram_wr_cmd_sel(if_ram_wr_cmd_sel),
		    .if_ram_wr_cmd_be(if_ram_wr_cmd_be),
		    .if_ram_wr_cmd_addr(if_ram_wr_cmd_addr),
		    .if_ram_wr_cmd_data(if_ram_wr_cmd_data),
		    .if_ram_wr_cmd_valid(if_ram_wr_cmd_valid),
		    .if_ram_wr_cmd_ready(if_ram_wr_cmd_ready),
		    .if_ram_wr_done(if_ram_wr_done),
		    .if_ram_rd_cmd_sel(if_ram_rd_cmd_sel),
		    .if_ram_rd_cmd_addr(if_ram_rd_cmd_addr),
		    .if_ram_rd_cmd_valid(if_ram_rd_cmd_valid),
		    .if_ram_rd_cmd_ready(if_ram_rd_cmd_ready),
		    .if_ram_rd_resp_data(if_ram_rd_resp_data),
		    .if_ram_rd_resp_valid(if_ram_rd_resp_valid),
		    .if_ram_rd_resp_ready(if_ram_rd_resp_ready),
      
		    .ram_wr_cmd_sel(ram_wr_cmd_sel),
		    .ram_wr_cmd_be(ram_wr_cmd_be),
		    .ram_wr_cmd_addr(ram_wr_cmd_addr),
		    .ram_wr_cmd_data(ram_wr_cmd_data),
		    .ram_wr_cmd_valid(ram_wr_cmd_valid),
		    .ram_wr_cmd_ready(ram_wr_cmd_ready),
		    .ram_wr_done(ram_wr_done),
		    .ram_rd_cmd_sel(ram_rd_cmd_sel),
		    .ram_rd_cmd_addr(ram_rd_cmd_addr),
		    .ram_rd_cmd_valid(ram_rd_cmd_valid),
		    .ram_rd_cmd_ready(ram_rd_cmd_ready),
		    .ram_rd_resp_data(ram_rd_resp_data),
		    .ram_rd_resp_valid(ram_rd_resp_valid),
		    .ram_rd_resp_ready(ram_rd_resp_ready)
		    );

   pcie4_uscale_plus_0 #(
			 )
   pcie4_inst (
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

	       .pcie_rq_seq_num0(s_axis_rq_seq_num_0),
	       .pcie_rq_seq_num_vld0(s_axis_rq_seq_num_valid_0),
	       .pcie_rq_seq_num1(s_axis_rq_seq_num_1),
	       .pcie_rq_seq_num_vld1(s_axis_rq_seq_num_valid_1),
      
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
	       .cfg_max_payload(cfg_max_payload),
	       .cfg_max_read_req(cfg_max_read_req),
	       .cfg_function_status(),
	       .cfg_function_power_state(),
	       .cfg_vf_status(),
	       .cfg_vf_power_state(),
	       .cfg_link_power_state(),

	       .cfg_mgmt_addr(cfg_mgmt_addr),
	       .cfg_mgmt_function_number(cfg_mgmt_function_number),
	       .cfg_mgmt_write(cfg_mgmt_write),
	       .cfg_mgmt_write_data(cfg_mgmt_write_data),
	       .cfg_mgmt_byte_enable(cfg_mgmt_byte_enable),
	       .cfg_mgmt_read(cfg_mgmt_read),
	       .cfg_mgmt_read_data(cfg_mgmt_read_data),
	       .cfg_mgmt_read_write_done(cfg_mgmt_read_write_done),
	       .cfg_mgmt_debug_access(1'b0),

	       .cfg_err_cor_out(),
	       .cfg_err_nonfatal_out(),
	       .cfg_err_fatal_out(),
	       .cfg_local_error_valid(),
	       .cfg_local_error_out(),
	       .cfg_ltssm_state(),
	       .cfg_rx_pm_state(),
	       .cfg_tx_pm_state(),
	       .cfg_rcb_status(cfg_rcb_status),
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

	       .cfg_fc_ph(cfg_fc_ph),
	       .cfg_fc_pd(cfg_fc_pd),
	       .cfg_fc_nph(cfg_fc_nph),
	       .cfg_fc_npd(cfg_fc_npd),
	       .cfg_fc_cplh(cfg_fc_cplh),
	       .cfg_fc_cpld(cfg_fc_cpld),
	       .cfg_fc_sel(cfg_fc_sel),

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

	       //.cfg_interrupt_int(4'd0),
	       //.cfg_interrupt_pending(4'd0),
	       //.cfg_interrupt_sent(),
	       //.cfg_interrupt_msix_enable(cfg_interrupt_msix_enable),
	       //.cfg_interrupt_msix_mask(cfg_interrupt_msix_mask),
	       //.cfg_interrupt_msix_vf_enable(cfg_interrupt_msix_vf_enable),
	       //.cfg_interrupt_msix_vf_mask(cfg_interrupt_msix_vf_mask),
	       //.cfg_interrupt_msix_address(cfg_interrupt_msix_address),
	       //.cfg_interrupt_msix_data(cfg_interrupt_msix_data),
	       //.cfg_interrupt_msix_int(cfg_interrupt_msix_int),
	       //.cfg_interrupt_msix_vec_pending(cfg_interrupt_msix_vec_pending),
	       //.cfg_interrupt_msix_vec_pending_status(cfg_interrupt_msix_vec_pending_status),
	       //.cfg_interrupt_msi_sent(cfg_interrupt_msix_sent),
	       //.cfg_interrupt_msi_fail(cfg_interrupt_msix_fail),
	       //.cfg_interrupt_msi_function_number(cfg_interrupt_msi_function_number),

	       .cfg_pm_aspm_l1_entry_reject(1'b0),
	       .cfg_pm_aspm_tx_l0s_entry_disable(1'b0),

	       .cfg_hot_reset_out(),

	       .cfg_config_space_enable(1'b1),
	       .cfg_req_pm_transition_l23_ready(1'b0),
	       .cfg_hot_reset_in(1'b0),

	       .cfg_ds_port_number(8'd0),
	       .cfg_ds_bus_number(8'd0),
	       .cfg_ds_device_number(5'd0),

	       .sys_clk(pcie_sys_clk),
	       .sys_clk_gt(pcie_sys_clk_gt),
	       .sys_reset(pcie_reset_n),

	       .phy_rdy_out()
	       );

   /* Device shim */
   pcie_us_if #(
		.AXIS_PCIE_DATA_WIDTH(AXIS_PCIE_DATA_WIDTH),
		.AXIS_PCIE_KEEP_WIDTH(AXIS_PCIE_KEEP_WIDTH),
		.AXIS_PCIE_RC_USER_WIDTH(AXIS_PCIE_RC_USER_WIDTH),
		.AXIS_PCIE_RQ_USER_WIDTH(AXIS_PCIE_RQ_USER_WIDTH),
		.AXIS_PCIE_CQ_USER_WIDTH(AXIS_PCIE_CQ_USER_WIDTH),
		.AXIS_PCIE_CC_USER_WIDTH(AXIS_PCIE_CC_USER_WIDTH),
		.RC_STRADDLE(RC_STRADDLE),
		.RQ_STRADDLE(RQ_STRADDLE),
		.CQ_STRADDLE(CQ_STRADDLE),
		.CC_STRADDLE(CC_STRADDLE),
		.RQ_SEQ_NUM_WIDTH(RQ_SEQ_NUM_WIDTH),
		.TLP_DATA_WIDTH(TLP_DATA_WIDTH),
		.TLP_STRB_WIDTH(TLP_STRB_WIDTH),
		.TLP_HDR_WIDTH(TLP_HDR_WIDTH),
		.TLP_SEG_COUNT(TLP_SEG_COUNT),
		.TX_SEQ_NUM_COUNT(TX_SEQ_NUM_COUNT),
		.TX_SEQ_NUM_WIDTH(TX_SEQ_NUM_WIDTH),
		.PF_COUNT(1),
		.VF_COUNT(0),
		.F_COUNT(F_COUNT),
		.READ_EXT_TAG_ENABLE(1),
		.READ_MAX_READ_REQ_SIZE(1),
		.READ_MAX_PAYLOAD_SIZE(1),
		.MSIX_ENABLE(0),
		.MSI_ENABLE(0)
		) 
   pcie_us_if_inst (
		    .clk(pcie_user_clk),
		    .rst(pcie_user_reset),
      
		    .s_axis_rc_tdata(axis_rc_tdata),
		    .s_axis_rc_tkeep(axis_rc_tkeep),
		    .s_axis_rc_tvalid(axis_rc_tvalid),
		    .s_axis_rc_tready(axis_rc_tready),
		    .s_axis_rc_tlast(axis_rc_tlast),
		    .s_axis_rc_tuser(axis_rc_tuser),
      
		    .m_axis_rq_tdata(axis_rq_tdata),
		    .m_axis_rq_tkeep(axis_rq_tkeep),
		    .m_axis_rq_tvalid(axis_rq_tvalid),
		    .m_axis_rq_tready(axis_rq_tready),
		    .m_axis_rq_tlast(axis_rq_tlast),
		    .m_axis_rq_tuser(axis_rq_tuser),
      
		    .s_axis_cq_tdata(axis_cq_tdata),
		    .s_axis_cq_tkeep(axis_cq_tkeep),
		    .s_axis_cq_tvalid(axis_cq_tvalid),
		    .s_axis_cq_tready(axis_cq_tready),
		    .s_axis_cq_tlast(axis_cq_tlast),
		    .s_axis_cq_tuser(axis_cq_tuser),

		    .m_axis_cc_tdata(axis_cc_tdata),
		    .m_axis_cc_tkeep(axis_cc_tkeep),
		    .m_axis_cc_tvalid(axis_cc_tvalid),
		    .m_axis_cc_tready(axis_cc_tready),
		    .m_axis_cc_tlast(axis_cc_tlast),
		    .m_axis_cc_tuser(axis_cc_tuser),

		    .s_axis_rq_seq_num_0(s_axis_rq_seq_num_0),
		    .s_axis_rq_seq_num_valid_0(s_axis_rq_seq_num_valid_0),
		    .s_axis_rq_seq_num_1(s_axis_rq_seq_num_1),
		    .s_axis_rq_seq_num_valid_1(s_axis_rq_seq_num_valid_1),

		    .cfg_mgmt_addr(cfg_mgmt_addr),
		    .cfg_mgmt_function_number(cfg_mgmt_function_number),
		    .cfg_mgmt_write(cfg_mgmt_write),
		    .cfg_mgmt_write_data(cfg_mgmt_write_data),
		    .cfg_mgmt_byte_enable(cfg_mgmt_byte_enable),
		    .cfg_mgmt_read(cfg_mgmt_read),
		    .cfg_mgmt_read_data(cfg_mgmt_read_data),
		    .cfg_mgmt_read_write_done(cfg_mgmt_read_write_done),

		    .cfg_max_payload(cfg_max_payload),
		    .cfg_max_read_req(cfg_max_read_req),

		    .cfg_fc_ph(cfg_fc_ph),
		    .cfg_fc_pd(cfg_fc_pd),
		    .cfg_fc_nph(cfg_fc_nph),
		    .cfg_fc_npd(cfg_fc_npd),
		    .cfg_fc_cplh(cfg_fc_cplh),
		    .cfg_fc_cpld(cfg_fc_cpld),
		    .cfg_fc_sel(cfg_fc_sel),

		    .cfg_interrupt_msi_enable(cfg_interrupt_msi_enable),
		    .cfg_interrupt_msi_vf_enable(cfg_interrupt_msi_vf_enable),
		    .cfg_interrupt_msi_mmenable(cfg_interrupt_msi_mmenable),
		    .cfg_interrupt_msi_mask_update(cfg_interrupt_msi_mask_update),
		    .cfg_interrupt_msi_data(cfg_interrupt_msi_data),
		    .cfg_interrupt_msi_select(cfg_interrupt_msi_select),
		    .cfg_interrupt_msi_int(cfg_interrupt_msi_int),
		    .cfg_interrupt_msi_pending_status(cfg_interrupt_msi_pending_status),
		    .cfg_interrupt_msi_pending_status_data_enable(cfg_interrupt_msi_pending_status_data_enable),
		    .cfg_interrupt_msi_pending_status_function_num(cfg_interrupt_msi_pending_status_function_num),
		    .cfg_interrupt_msi_sent(cfg_interrupt_msi_sent),
		    .cfg_interrupt_msi_fail(cfg_interrupt_msi_fail),
		    .cfg_interrupt_msix_enable(cfg_interrupt_msix_enable),
		    .cfg_interrupt_msix_mask(cfg_interrupt_msix_mask),
		    .cfg_interrupt_msix_vf_enable(cfg_interrupt_msix_vf_enable),
		    .cfg_interrupt_msix_vf_mask(cfg_interrupt_msix_vf_mask),
		    .cfg_interrupt_msix_address(cfg_interrupt_msix_address),
		    .cfg_interrupt_msix_data(cfg_interrupt_msix_data),
		    .cfg_interrupt_msix_int(cfg_interrupt_msix_int),
		    .cfg_interrupt_msix_vec_pending(cfg_interrupt_msix_vec_pending),
		    .cfg_interrupt_msix_vec_pending_status(cfg_interrupt_msix_vec_pending_status),
		    .cfg_interrupt_msix_sent(cfg_interrupt_msix_sent),
		    .cfg_interrupt_msix_fail(cfg_interrupt_msix_fail),
		    .cfg_interrupt_msi_attr(cfg_interrupt_msi_attr),
		    .cfg_interrupt_msi_tph_present(cfg_interrupt_msi_tph_present),
		    .cfg_interrupt_msi_tph_type(cfg_interrupt_msi_tph_type),
		    .cfg_interrupt_msi_tph_st_tag(cfg_interrupt_msi_tph_st_tag),
		    .cfg_interrupt_msi_function_number(cfg_interrupt_msi_function_number),

		    .rx_req_tlp_data(rx_req_tlp_data),
		    .rx_req_tlp_strb(rx_req_tlp_strb),
		    .rx_req_tlp_hdr(rx_req_tlp_hdr),
		    .rx_req_tlp_bar_id(rx_req_tlp_bar_id),
		    .rx_req_tlp_func_num(rx_req_tlp_func_num),
		    .rx_req_tlp_valid(rx_req_tlp_valid),
		    .rx_req_tlp_sop(rx_req_tlp_sop),
		    .rx_req_tlp_eop(rx_req_tlp_eop),
		    .rx_req_tlp_ready(rx_req_tlp_ready),

		    .rx_cpl_tlp_data(rx_cpl_tlp_data),
		    .rx_cpl_tlp_strb(rx_cpl_tlp_strb),
		    .rx_cpl_tlp_hdr(rx_cpl_tlp_hdr),
		    .rx_cpl_tlp_error(rx_cpl_tlp_error),
		    .rx_cpl_tlp_valid(rx_cpl_tlp_valid),
		    .rx_cpl_tlp_sop(rx_cpl_tlp_sop),
		    .rx_cpl_tlp_eop(rx_cpl_tlp_eop),
		    .rx_cpl_tlp_ready(rx_cpl_tlp_ready),

		    .tx_rd_req_tlp_hdr(tx_rd_req_tlp_hdr),
		    .tx_rd_req_tlp_seq(tx_rd_req_tlp_seq),
		    .tx_rd_req_tlp_valid(tx_rd_req_tlp_valid),
		    .tx_rd_req_tlp_sop(tx_rd_req_tlp_sop),
		    .tx_rd_req_tlp_eop(tx_rd_req_tlp_eop),
		    .tx_rd_req_tlp_ready(tx_rd_req_tlp_ready),

		    .m_axis_rd_req_tx_seq_num(axis_rd_req_tx_seq_num),
		    .m_axis_rd_req_tx_seq_num_valid(axis_rd_req_tx_seq_num_valid),

		    .tx_wr_req_tlp_data(tx_wr_req_tlp_data),
		    .tx_wr_req_tlp_strb(tx_wr_req_tlp_strb),
		    .tx_wr_req_tlp_hdr(tx_wr_req_tlp_hdr),
		    .tx_wr_req_tlp_seq(tx_wr_req_tlp_seq),
		    .tx_wr_req_tlp_valid(tx_wr_req_tlp_valid),
		    .tx_wr_req_tlp_sop(tx_wr_req_tlp_sop),
		    .tx_wr_req_tlp_eop(tx_wr_req_tlp_eop),
		    .tx_wr_req_tlp_ready(tx_wr_req_tlp_ready),

		    .m_axis_wr_req_tx_seq_num(axis_wr_req_tx_seq_num),
		    .m_axis_wr_req_tx_seq_num_valid(axis_wr_req_tx_seq_num_valid),

		    .tx_cpl_tlp_data(tx_cpl_tlp_data),
		    .tx_cpl_tlp_strb(tx_cpl_tlp_strb),
		    .tx_cpl_tlp_hdr(tx_cpl_tlp_hdr),
		    .tx_cpl_tlp_valid(tx_cpl_tlp_valid),
		    .tx_cpl_tlp_sop(tx_cpl_tlp_sop),
		    .tx_cpl_tlp_eop(tx_cpl_tlp_eop),
		    .tx_cpl_tlp_ready(tx_cpl_tlp_ready),

		    .tx_msix_wr_req_tlp_data(),
		    .tx_msix_wr_req_tlp_strb(),
		    .tx_msix_wr_req_tlp_hdr(),
		    .tx_msix_wr_req_tlp_valid(),
		    .tx_msix_wr_req_tlp_sop(),
		    .tx_msix_wr_req_tlp_eop(),
		    .tx_msix_wr_req_tlp_ready(),

		    .tx_fc_ph_av(),
		    .tx_fc_pd_av(),
		    .tx_fc_nph_av(),
		    .tx_fc_npd_av(),
		    .tx_fc_cplh_av(),
		    .tx_fc_cpld_av(),

		    .ext_tag_enable(ext_tag_enable),
		    .max_read_request_size(max_read_request_size),
		    .max_payload_size(max_payload_size),
		    .msix_enable(),
		    .msix_mask(),

		    .msi_irq()
		    );

   dma_if_pcie #(
		 .TLP_DATA_WIDTH(TLP_DATA_WIDTH),
		 .TLP_STRB_WIDTH(TLP_STRB_WIDTH),
		 .TLP_HDR_WIDTH(TLP_HDR_WIDTH),
		 .TLP_SEG_COUNT(TLP_SEG_COUNT),
		 .TX_SEQ_NUM_COUNT(TX_SEQ_NUM_COUNT),
		 .TX_SEQ_NUM_WIDTH(TX_SEQ_NUM_WIDTH),
		 .TX_SEQ_NUM_ENABLE(TX_SEQ_NUM_ENABLE),
		 .RAM_SEL_WIDTH(M_RAM_SEL_WIDTH),
		 .RAM_ADDR_WIDTH(RAM_ADDR_WIDTH),
		 .RAM_SEG_COUNT(RAM_SEG_COUNT),
		 .RAM_SEG_DATA_WIDTH(RAM_SEG_DATA_WIDTH),
		 .RAM_SEG_BE_WIDTH(RAM_SEG_BE_WIDTH),
		 .RAM_SEG_ADDR_WIDTH(RAM_SEG_ADDR_WIDTH),
		 .PCIE_ADDR_WIDTH(PCIE_ADDR_WIDTH),
		 .PCIE_TAG_COUNT(PCIE_TAG_COUNT),
		 .IMM_ENABLE(IMM_ENABLE),
		 .IMM_WIDTH(IMM_WIDTH),
		 .LEN_WIDTH(LEN_WIDTH),
		 .TAG_WIDTH(M_TAG_WIDTH),
		 .READ_OP_TABLE_SIZE(READ_OP_TABLE_SIZE),
		 .READ_TX_LIMIT(READ_TX_LIMIT),
		 .READ_CPLH_FC_LIMIT(READ_CPLH_FC_LIMIT),
		 .READ_CPLD_FC_LIMIT(READ_CPLD_FC_LIMIT),
		 .WRITE_OP_TABLE_SIZE(WRITE_OP_TABLE_SIZE),
		 .WRITE_TX_LIMIT(WRITE_TX_LIMIT),
		 .TLP_FORCE_64_BIT_ADDR(TLP_FORCE_64_BIT_ADDR),
		 .CHECK_BUS_NUMBER(CHECK_BUS_NUMBER)
		 ) 
   dma_if_pcie_inst (
		     .clk(pcie_user_clk),
		     .rst(pcie_user_reset),
      
		     .rx_cpl_tlp_data(rx_cpl_tlp_data),
		     .rx_cpl_tlp_hdr(rx_cpl_tlp_hdr),
		     .rx_cpl_tlp_error(rx_cpl_tlp_error),
		     .rx_cpl_tlp_valid(rx_cpl_tlp_valid),
		     .rx_cpl_tlp_sop(rx_cpl_tlp_sop),
		     .rx_cpl_tlp_eop(rx_cpl_tlp_eop),
		     .rx_cpl_tlp_ready(rx_cpl_tlp_ready),
      
		     .tx_rd_req_tlp_hdr(tx_rd_req_tlp_hdr),
		     .tx_rd_req_tlp_seq(tx_rd_req_tlp_seq),
		     .tx_rd_req_tlp_valid(tx_rd_req_tlp_valid),
		     .tx_rd_req_tlp_sop(tx_rd_req_tlp_sop),
		     .tx_rd_req_tlp_eop(tx_rd_req_tlp_eop),
		     .tx_rd_req_tlp_ready(tx_rd_req_tlp_ready),
      
		     .tx_wr_req_tlp_data(tx_wr_req_tlp_data),
		     .tx_wr_req_tlp_strb(tx_wr_req_tlp_strb),
		     .tx_wr_req_tlp_hdr(tx_wr_req_tlp_hdr),
		     .tx_wr_req_tlp_seq(tx_wr_req_tlp_seq),
		     .tx_wr_req_tlp_valid(tx_wr_req_tlp_valid),
		     .tx_wr_req_tlp_sop(tx_wr_req_tlp_sop),
		     .tx_wr_req_tlp_eop(tx_wr_req_tlp_eop),
		     .tx_wr_req_tlp_ready(tx_wr_req_tlp_ready),
      
		     .s_axis_rd_req_tx_seq_num(axis_rd_req_tx_seq_num),
		     .s_axis_rd_req_tx_seq_num_valid(axis_rd_req_tx_seq_num_valid),
      
		     .s_axis_wr_req_tx_seq_num(axis_wr_req_tx_seq_num),
		     .s_axis_wr_req_tx_seq_num_valid(axis_wr_req_tx_seq_num_valid),
      
		     .s_axis_read_desc_pcie_addr(m_axis_read_desc_dma_addr),
		     .s_axis_read_desc_ram_sel(m_axis_read_desc_ram_sel),
		     .s_axis_read_desc_ram_addr(m_axis_read_desc_ram_addr),
		     .s_axis_read_desc_len(m_axis_read_desc_len),
		     .s_axis_read_desc_tag(m_axis_read_desc_tag),
		     .s_axis_read_desc_valid(m_axis_read_desc_valid),
		     .s_axis_read_desc_ready(m_axis_read_desc_ready),

		     .m_axis_read_desc_status_tag(s_axis_read_desc_status_tag),
		     .m_axis_read_desc_status_error(s_axis_read_desc_status_error),
		     .m_axis_read_desc_status_valid(s_axis_read_desc_status_valid),

		     .s_axis_write_desc_pcie_addr(m_axis_write_desc_dma_addr),
		     .s_axis_write_desc_ram_sel(m_axis_write_desc_ram_sel),
		     .s_axis_write_desc_ram_addr(m_axis_write_desc_ram_addr),
		     .s_axis_write_desc_imm(m_axis_write_desc_imm),
		     .s_axis_write_desc_imm_en(m_axis_write_desc_imm_en),
		     .s_axis_write_desc_len(m_axis_write_desc_len),
		     .s_axis_write_desc_tag(m_axis_write_desc_tag),
		     .s_axis_write_desc_valid(m_axis_write_desc_valid),
		     .s_axis_write_desc_ready(m_axis_write_desc_ready),

		     .m_axis_write_desc_status_tag(s_axis_write_desc_status_tag),
		     .m_axis_write_desc_status_error(s_axis_write_desc_status_error),
		     .m_axis_write_desc_status_valid(s_axis_write_desc_status_valid),

		     .ram_wr_cmd_sel(if_ram_wr_cmd_sel),
		     .ram_wr_cmd_be(if_ram_wr_cmd_be),
		     .ram_wr_cmd_addr(if_ram_wr_cmd_addr),
		     .ram_wr_cmd_data(if_ram_wr_cmd_data),
		     .ram_wr_cmd_valid(if_ram_wr_cmd_valid),
		     .ram_wr_cmd_ready(if_ram_wr_cmd_ready),
		     .ram_wr_done(if_ram_wr_done),
		     .ram_rd_cmd_sel(if_ram_rd_cmd_sel),
		     .ram_rd_cmd_addr(if_ram_rd_cmd_addr),
		     .ram_rd_cmd_valid(if_ram_rd_cmd_valid),
		     .ram_rd_cmd_ready(if_ram_rd_cmd_ready),
		     .ram_rd_resp_data(if_ram_rd_resp_data),
		     .ram_rd_resp_valid(if_ram_rd_resp_valid),
		     .ram_rd_resp_ready(if_ram_rd_resp_ready),
      
		     .read_enable(1'b1),
		     .write_enable(1'b1),
		     .ext_tag_enable(ext_tag_enable),
		     .rcb_128b(cfg_rcb_status[0]),
		     .requester_id({8'd0, 5'd0, 3'd0}),
		     .max_read_request_size(max_read_request_size),
		     .max_payload_size(max_payload_size),
      
		     .status_rd_busy(),
		     .status_wr_busy(),
		     .status_error_cor(),
		     .status_error_uncor(),
		     .stat_rd_op_start_tag(),
		     .stat_rd_op_start_len(),
		     .stat_rd_op_start_valid(),
		     .stat_rd_op_finish_tag(),
		     .stat_rd_op_finish_status(),
		     .stat_rd_op_finish_valid(),
		     .stat_rd_req_start_tag(),
		     .stat_rd_req_start_len(),
		     .stat_rd_req_start_valid(),
		     .stat_rd_req_finish_tag(),
		     .stat_rd_req_finish_status(),
		     .stat_rd_req_finish_valid(),
		     .stat_rd_req_timeout(),
		     .stat_rd_op_table_full(),
		     .stat_rd_no_tags(),
		     .stat_rd_tx_limit(),
		     .stat_rd_tx_stall(),
		     .stat_wr_op_start_tag(),
		     .stat_wr_op_start_len(),
		     .stat_wr_op_start_valid(),
		     .stat_wr_op_finish_tag(),
		     .stat_wr_op_finish_status(),
		     .stat_wr_op_finish_valid(),
		     .stat_wr_req_start_tag(),
		     .stat_wr_req_start_len(),
		     .stat_wr_req_start_valid(),
		     .stat_wr_req_finish_tag(),
		     .stat_wr_req_finish_status(),
		     .stat_wr_req_finish_valid(),
		     .stat_wr_op_table_full(),
		     .stat_wr_tx_limit(),
		     .stat_wr_tx_stall()
      
		     );
   
   
   pcie_axi_master #(
		     .TLP_DATA_WIDTH(TLP_DATA_WIDTH),
		     .TLP_STRB_WIDTH(TLP_STRB_WIDTH),
		     .TLP_HDR_WIDTH(TLP_HDR_WIDTH),
		     .TLP_SEG_COUNT(TLP_SEG_COUNT),
		     .AXI_DATA_WIDTH(AXI_DATA_WIDTH),
		     .AXI_ADDR_WIDTH(AXI_ADDR_WIDTH),
		     .AXI_STRB_WIDTH(AXI_STRB_WIDTH),
		     .AXI_ID_WIDTH(AXI_ID_WIDTH),
		     .AXI_MAX_BURST_LEN(256),
		     .TLP_FORCE_64_BIT_ADDR(TLP_FORCE_64_BIT_ADDR)
		     ) 
   pcie_axim (
	      .clk(pcie_user_clk),
	      .rst(pcie_user_reset),

	      .rx_req_tlp_data(rx_req_tlp_data),
	      .rx_req_tlp_hdr(rx_req_tlp_hdr),
	      .rx_req_tlp_valid(rx_req_tlp_valid),
	      .rx_req_tlp_sop(rx_req_tlp_sop),
	      .rx_req_tlp_eop(rx_req_tlp_eop),
	      .rx_req_tlp_ready(rx_req_tlp_ready),

	      .tx_cpl_tlp_data(tx_cpl_tlp_data),
	      .tx_cpl_tlp_strb(tx_cpl_tlp_strb),
	      .tx_cpl_tlp_hdr(tx_cpl_tlp_hdr),
	      .tx_cpl_tlp_valid(tx_cpl_tlp_valid),
	      .tx_cpl_tlp_sop(tx_cpl_tlp_sop),
	      .tx_cpl_tlp_eop(tx_cpl_tlp_eop),
	      .tx_cpl_tlp_ready(tx_cpl_tlp_ready),
      
	      .m_axi_awid(m_axi_awid),
	      .m_axi_awaddr(m_axi_awaddr),
	      .m_axi_awlen(m_axi_awlen),
	      .m_axi_awsize(m_axi_awsize),
	      .m_axi_awburst(m_axi_awburst),
	      .m_axi_awlock(m_axi_awlock),
	      .m_axi_awcache(m_axi_awcache),
	      .m_axi_awprot(m_axi_awprot),
	      .m_axi_awvalid(m_axi_awvalid),
	      .m_axi_awready(m_axi_awready),
	      .m_axi_wdata(m_axi_wdata),
	      .m_axi_wstrb(m_axi_wstrb),
	      .m_axi_wlast(m_axi_wlast),
	      .m_axi_wvalid(m_axi_wvalid),
	      .m_axi_wready(m_axi_wready),
	      .m_axi_bid(m_axi_bid),
	      .m_axi_bresp(m_axi_bresp),
	      .m_axi_bvalid(m_axi_bvalid),
	      .m_axi_bready(m_axi_bready),
	      .m_axi_arid(m_axi_arid),
	      .m_axi_araddr(m_axi_araddr),
	      .m_axi_arlen(m_axi_arlen),
	      .m_axi_arsize(m_axi_arsize),
	      .m_axi_arburst(m_axi_arburst),
	      .m_axi_arlock(m_axi_arlock),
	      .m_axi_arcache(m_axi_arcache),
	      .m_axi_arprot(m_axi_arprot),
	      .m_axi_arvalid(m_axi_arvalid),
	      .m_axi_arready(m_axi_arready),
	      .m_axi_rid(m_axi_rid),
	      .m_axi_rdata(m_axi_rdata),
	      .m_axi_rresp(m_axi_rresp),
	      .m_axi_rlast(m_axi_rlast),
	      .m_axi_rvalid(m_axi_rvalid),
	      .m_axi_rready(m_axi_rready),

	      .completer_id({8'd0, 5'd0, 3'd0}),
	      // .completer_id_enable(1'b0),
	      .max_payload_size(max_payload_size),

	      .status_error_cor(),
	      .status_error_uncor()
	      );
endmodule
