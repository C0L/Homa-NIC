# xilinx IP and the verilog module type (not the name of inst!!)
create_ip -name pcie4_uscale_plus -vendor xilinx.com -library ip -module_name pcie4_uscale_plus_0

set_property -dict [list \
    CONFIG.PL_LINK_CAP_MAX_LINK_SPEED {8.0_GT/s} \
    CONFIG.PL_LINK_CAP_MAX_LINK_WIDTH {X16} \
    CONFIG.AXISTEN_IF_EXT_512_CQ_STRADDLE {true} \
    CONFIG.AXISTEN_IF_EXT_512_RQ_STRADDLE {true} \
    CONFIG.AXISTEN_IF_EXT_512_RC_4TLP_STRADDLE {true} \
    CONFIG.axisten_if_enable_client_tag {true} \
    CONFIG.axisten_if_width {512_bit} \
    CONFIG.extended_tag_field {true} \
    CONFIG.pf0_dev_cap_max_payload {1024_bytes} \
    CONFIG.axisten_freq {250} \
    CONFIG.PF0_CLASS_CODE {058000} \
    CONFIG.PF0_DEVICE_ID {0001} \
    CONFIG.PF0_SUBSYSTEM_ID {90fa} \
    CONFIG.PF0_SUBSYSTEM_VENDOR_ID {10ee} \
    CONFIG.pf0_bar0_64bit {true} \
    CONFIG.pf0_bar0_prefetchable {false} \
    CONFIG.pf0_bar0_scale {Megabytes} \
    CONFIG.pf0_bar0_size {64} \
    CONFIG.pf0_msi_enabled {false} \
    CONFIG.pf0_msix_enabled {false} \
    CONFIG.PF0_MSIX_CAP_TABLE_SIZE {01F} \
    CONFIG.PF0_MSIX_CAP_TABLE_BIR {BAR_5:4} \
    CONFIG.PF0_MSIX_CAP_TABLE_OFFSET {00000000} \
    CONFIG.PF0_MSIX_CAP_PBA_BIR {BAR_5:4} \
    CONFIG.PF0_MSIX_CAP_PBA_OFFSET {00008000} \
    CONFIG.MSI_X_OPTIONS {MSI-X_External} \
    CONFIG.vendor_id {1234} \
] [get_ips pcie4_uscale_plus_0]

create_ip -name proc_sys_reset -vendor xilinx.com -library ip -module_name ProcessorSystemReset

create_ip -name clk_wiz -vendor xilinx.com -library ip -module_name ClockWizard

set_property -dict [list \
    CONFIG.CLKIN1_JITTER_PS {33.330000000000005} \
    CONFIG.CLKOUT1_JITTER {88.577} \
    CONFIG.CLKOUT1_PHASE_ERROR {77.836} \
    CONFIG.CLKOUT1_REQUESTED_OUT_FREQ {200.000} \
    CONFIG.CLKOUT2_JITTER {101.475} \
    CONFIG.CLKOUT2_PHASE_ERROR {77.836} \
    CONFIG.CLKOUT2_USED {true} \
    CONFIG.MMCM_CLKFBOUT_MULT_F {4.000} \
    CONFIG.MMCM_CLKIN1_PERIOD {3.333} \
    CONFIG.MMCM_CLKIN2_PERIOD {10.0} \
    CONFIG.MMCM_CLKOUT0_DIVIDE_F {6.000} \
    CONFIG.MMCM_CLKOUT1_DIVIDE {12} \
    CONFIG.NUM_OUT_CLKS {2} \
    CONFIG.PRIM_IN_FREQ {300.000} \
    CONFIG.PRIM_SOURCE {Differential_clock_capable_pin} \
    CONFIG.RESET_TYPE {ACTIVE_HIGH} \
  ] [get_ips ClockWizard]
