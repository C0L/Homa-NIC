
################################################################
# This is a generated script based on design: homa
#
# Though there are limitations about the generated script,
# the main purpose of this utility is to make learning
# IP Integrator Tcl commands easier.
################################################################

namespace eval _tcl {
proc get_script_folder {} {
   set script_path [file normalize [info script]]
   set script_folder [file dirname $script_path]
   return $script_folder
}
}
variable script_folder
set script_folder [_tcl::get_script_folder]

################################################################
# Check if script is running in correct Vivado version.
################################################################
set scripts_vivado_version 2023.1
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
   puts ""
   catch {common::send_gid_msg -ssname BD::TCL -id 2041 -severity "ERROR" "This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_bd_tcl to create an updated script."}

   return 1
}

################################################################
# START
################################################################

# To test this script, run the following commands from Vivado Tcl console:
# source homa_script.tcl

# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./myproj/project_1.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project project_1 myproj -part xcu250-figd2104-2L-e
   set_property BOARD_PART xilinx.com:au250:part0:1.3 [current_project]
}


# CHANGE DESIGN NAME HERE
variable design_name
set design_name homa

# If you do not already have an existing IP Integrator design open,
# you can create a design using the following command:
#    create_bd_design $design_name

# Creating design if needed
set errMsg ""
set nRet 0

set cur_design [current_bd_design -quiet]
set list_cells [get_bd_cells -quiet]

if { ${design_name} eq "" } {
   # USE CASES:
   #    1) Design_name not set

   set errMsg "Please set the variable <design_name> to a non-empty value."
   set nRet 1

} elseif { ${cur_design} ne "" && ${list_cells} eq "" } {
   # USE CASES:
   #    2): Current design opened AND is empty AND names same.
   #    3): Current design opened AND is empty AND names diff; design_name NOT in project.
   #    4): Current design opened AND is empty AND names diff; design_name exists in project.

   if { $cur_design ne $design_name } {
      common::send_gid_msg -ssname BD::TCL -id 2001 -severity "INFO" "Changing value of <design_name> from <$design_name> to <$cur_design> since current design is empty."
      set design_name [get_property NAME $cur_design]
   }
   common::send_gid_msg -ssname BD::TCL -id 2002 -severity "INFO" "Constructing design in IPI design <$cur_design>..."

} elseif { ${cur_design} ne "" && $list_cells ne "" && $cur_design eq $design_name } {
   # USE CASES:
   #    5) Current design opened AND has components AND same names.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 1
} elseif { [get_files -quiet ${design_name}.bd] ne "" } {
   # USE CASES: 
   #    6) Current opened design, has components, but diff names, design_name exists in project.
   #    7) No opened design, design_name exists in project.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 2

} else {
   # USE CASES:
   #    8) No opened design, design_name not in project.
   #    9) Current opened design, has components, but diff names, design_name not in project.

   common::send_gid_msg -ssname BD::TCL -id 2003 -severity "INFO" "Currently there is no design <$design_name> in project, so creating one..."

   create_bd_design $design_name

   common::send_gid_msg -ssname BD::TCL -id 2004 -severity "INFO" "Making design <$design_name> as current_bd_design."
   current_bd_design $design_name

}

common::send_gid_msg -ssname BD::TCL -id 2005 -severity "INFO" "Currently the variable <design_name> is equal to \"$design_name\"."

if { $nRet != 0 } {
   catch {common::send_gid_msg -ssname BD::TCL -id 2006 -severity "ERROR" $errMsg}
   return $nRet
}

set bCheckIPsPassed 1
##################################################################
# CHECK IPs
##################################################################
set bCheckIPs 1
if { $bCheckIPs == 1 } {
   set list_check_ips "\ 
xilinx.com:ip:axi_clock_converter:2.1\
xilinx.com:ip:clk_wiz:6.0\
xilinx.com:ip:proc_sys_reset:5.0\
xilinx.com:ip:xdma:4.1\
xilinx.com:ip:axi_datamover:5.1\
xilinx.com:ip:ila:6.2\
xilinx.com:ip:axi_bram_ctrl:4.1\
xilinx.com:ip:blk_mem_gen:8.4\
xilinx.com:ip:axis_data_fifo:2.0\
xilinx.com:hls:homa:1.0\
xilinx.com:ip:xlconstant:1.1\
xilinx.com:ip:util_ds_buf:2.2\
xilinx.com:hls:interface:1.0\
"

   set list_ips_missing ""
   common::send_gid_msg -ssname BD::TCL -id 2011 -severity "INFO" "Checking if the following IPs exist in the project's IP catalog: $list_check_ips ."

   foreach ip_vlnv $list_check_ips {
      set ip_obj [get_ipdefs -all $ip_vlnv]
      if { $ip_obj eq "" } {
         lappend list_ips_missing $ip_vlnv
      }
   }

   if { $list_ips_missing ne "" } {
      catch {common::send_gid_msg -ssname BD::TCL -id 2012 -severity "ERROR" "The following IPs are not found in the IP Catalog:\n  $list_ips_missing\n\nResolution: Please add the repository containing the IP(s) to the project." }
      set bCheckIPsPassed 0
   }

}

if { $bCheckIPsPassed != 1 } {
  common::send_gid_msg -ssname BD::TCL -id 2023 -severity "WARNING" "Will not continue with creation of design due to the error(s) above."
  return 3
}

##################################################################
# DESIGN PROCs
##################################################################



# Procedure to create entire design; Provide argument to make
# procedure reusable. If parentCell is "", will use root.
proc create_root_design { parentCell } {

  variable script_folder
  variable design_name

  if { $parentCell eq "" } {
     set parentCell [get_bd_cells /]
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2090 -severity "ERROR" "Unable to find parent cell <$parentCell>!"}
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2091 -severity "ERROR" "Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."}
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj


  # Create interface ports
  set default_300mhz_clk0 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 default_300mhz_clk0 ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {300000000} \
   ] $default_300mhz_clk0

  set pci_express_x16 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:pcie_7x_mgt_rtl:1.0 pci_express_x16 ]

  set pcie_refclk [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 pcie_refclk ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {100000000} \
   ] $pcie_refclk


  # Create ports
  set resetn [ create_bd_port -dir I -type rst resetn ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_LOW} \
 ] $resetn
  set pcie_perstn [ create_bd_port -dir I -type rst pcie_perstn ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_LOW} \
 ] $pcie_perstn

  # Create instance: axi_clock_converter_0, and set properties
  set axi_clock_converter_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_clock_converter:2.1 axi_clock_converter_0 ]

  # Create instance: mainClk, and set properties
  set mainClk [ create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:6.0 mainClk ]
  set_property -dict [list \
    CONFIG.CLKIN1_JITTER_PS {33.330000000000005} \
    CONFIG.CLKOUT1_JITTER {88.577} \
    CONFIG.CLKOUT1_PHASE_ERROR {77.836} \
    CONFIG.CLKOUT1_REQUESTED_OUT_FREQ {200} \
    CONFIG.CLK_IN1_BOARD_INTERFACE {default_300mhz_clk0} \
    CONFIG.MMCM_CLKFBOUT_MULT_F {4.000} \
    CONFIG.MMCM_CLKIN1_PERIOD {3.333} \
    CONFIG.MMCM_CLKIN2_PERIOD {10.0} \
    CONFIG.MMCM_CLKOUT0_DIVIDE_F {6.000} \
    CONFIG.MMCM_DIVCLK_DIVIDE {1} \
    CONFIG.PRIM_SOURCE {Differential_clock_capable_pin} \
    CONFIG.RESET_BOARD_INTERFACE {resetn} \
    CONFIG.RESET_PORT {resetn} \
    CONFIG.RESET_TYPE {ACTIVE_LOW} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $mainClk


  # Create instance: axi_clock_converter_1, and set properties
  set axi_clock_converter_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_clock_converter:2.1 axi_clock_converter_1 ]
  set_property CONFIG.ADDR_WIDTH {48} $axi_clock_converter_1


  # Create instance: proc_sys_reset_0, and set properties
  set proc_sys_reset_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_0 ]
  set_property -dict [list \
    CONFIG.RESET_BOARD_INTERFACE {resetn} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $proc_sys_reset_0


  # Create instance: xdma_0, and set properties
  set xdma_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xdma:4.1 xdma_0 ]
  set_property -dict [list \
    CONFIG.PCIE_BOARD_INTERFACE {pci_express_x16} \
    CONFIG.SYS_RST_N_BOARD_INTERFACE {pcie_perstn} \
    CONFIG.axi_addr_width {48} \
    CONFIG.axibar_num {1} \
    CONFIG.axilite_master_en {false} \
    CONFIG.axist_bypass_en {false} \
    CONFIG.c_m_axi_num_write {32} \
    CONFIG.c_s_axi_supports_narrow_burst {false} \
    CONFIG.device_port_type {PCI_Express_Endpoint_device} \
    CONFIG.dma_reset_source_sel {Phy_Ready} \
    CONFIG.en_axi_slave_if {true} \
    CONFIG.en_transceiver_status_ports {false} \
    CONFIG.enable_ltssm_dbg {false} \
    CONFIG.enable_pcie_debug_ports {False} \
    CONFIG.functional_mode {AXI_Bridge} \
    CONFIG.mode_selection {Advanced} \
    CONFIG.pf0_bar0_64bit {true} \
    CONFIG.pf0_bar0_scale {Megabytes} \
    CONFIG.pf0_bar0_size {64} \
    CONFIG.ref_clk_freq {100_MHz} \
    CONFIG.xdma_axi_intf_mm {AXI_Memory_Mapped} \
    CONFIG.xdma_rnum_chnl {4} \
    CONFIG.xdma_wnum_chnl {4} \
  ] $xdma_0


  # Create instance: axi_datamover_0, and set properties
  set axi_datamover_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_datamover:5.1 axi_datamover_0 ]
  set_property -dict [list \
    CONFIG.c_addr_width {48} \
    CONFIG.c_dummy {1} \
    CONFIG.c_enable_mm2s_adv_sig {0} \
    CONFIG.c_enable_s2mm_adv_sig {0} \
    CONFIG.c_include_mm2s_dre {false} \
    CONFIG.c_include_s2mm_dre {true} \
    CONFIG.c_m_axi_mm2s_data_width {512} \
    CONFIG.c_m_axi_mm2s_id_width {0} \
    CONFIG.c_m_axi_s2mm_data_width {512} \
    CONFIG.c_m_axi_s2mm_id_width {0} \
    CONFIG.c_m_axis_mm2s_tdata_width {512} \
    CONFIG.c_mm2s_btt_used {23} \
    CONFIG.c_mm2s_burst_size {2} \
    CONFIG.c_mm2s_include_sf {false} \
    CONFIG.c_s2mm_btt_used {23} \
    CONFIG.c_s2mm_burst_size {2} \
    CONFIG.c_s2mm_support_indet_btt {false} \
    CONFIG.c_s_axis_s2mm_tdata_width {512} \
    CONFIG.c_single_interface {1} \
  ] $axi_datamover_0


  # Create instance: ila_0, and set properties
  set ila_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_0 ]
  set_property -dict [list \
    CONFIG.C_SLOT_0_AXIS_TDATA_WIDTH {512} \
    CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} \
  ] $ila_0


  # Create instance: ila_1, and set properties
  set ila_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_1 ]
  set_property CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4} $ila_1


  # Create instance: ila_3, and set properties
  set ila_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_3 ]
  set_property -dict [list \
    CONFIG.C_SLOT_0_AXIS_TDATA_WIDTH {512} \
    CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} \
  ] $ila_3


  # Create instance: ila_4, and set properties
  set ila_4 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_4 ]
  set_property -dict [list \
    CONFIG.C_SLOT_0_AXIS_TDATA_WIDTH {512} \
    CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} \
  ] $ila_4


  # Create instance: port_to_msgbuff, and set properties
  set port_to_msgbuff [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 port_to_msgbuff ]
  set_property CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} $port_to_msgbuff


  # Create instance: stream_out_ila2, and set properties
  set stream_out_ila2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 stream_out_ila2 ]
  set_property CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} $stream_out_ila2


  # Create instance: axi_bram_ctrl_0, and set properties
  set axi_bram_ctrl_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_0 ]
  set_property CONFIG.DATA_WIDTH {512} $axi_bram_ctrl_0


  # Create instance: blk_mem_gen_0, and set properties
  set blk_mem_gen_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 blk_mem_gen_0 ]
  set_property -dict [list \
    CONFIG.Memory_Type {True_Dual_Port_RAM} \
    CONFIG.use_bram_block {BRAM_Controller} \
  ] $blk_mem_gen_0


  # Create instance: axis_data_fifo_0, and set properties
  set axis_data_fifo_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:2.0 axis_data_fifo_0 ]
  set_property -dict [list \
    CONFIG.FIFO_DEPTH {16} \
    CONFIG.TDATA_NUM_BYTES {8} \
  ] $axis_data_fifo_0


  # Create instance: ila_5, and set properties
  set ila_5 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_5 ]
  set_property CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} $ila_5


  # Create instance: ila_6, and set properties
  set ila_6 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_6 ]
  set_property CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} $ila_6


  # Create instance: homa, and set properties
  set homa [ create_bd_cell -type ip -vlnv xilinx.com:hls:homa:1.0 homa ]

  # Create instance: xlconstant_0, and set properties
  set xlconstant_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_0 ]
  set_property CONFIG.CONST_VAL {0} $xlconstant_0


  # Create instance: util_ds_buf, and set properties
  set util_ds_buf [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_ds_buf:2.2 util_ds_buf ]
  set_property -dict [list \
    CONFIG.DIFF_CLK_IN_BOARD_INTERFACE {pcie_refclk} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $util_ds_buf


  # Create instance: ila_2, and set properties
  set ila_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_2 ]
  set_property -dict [list \
    CONFIG.C_MONITOR_TYPE {Native} \
    CONFIG.C_NUM_OF_PROBES {3} \
    CONFIG.C_PROBE0_WIDTH {64} \
    CONFIG.C_PROBE1_WIDTH {64} \
    CONFIG.C_PROBE2_TYPE {1} \
    CONFIG.C_PROBE2_WIDTH {512} \
  ] $ila_2


  # Create instance: ila_7, and set properties
  set ila_7 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_7 ]
  set_property -dict [list \
    CONFIG.C_MONITOR_TYPE {Native} \
    CONFIG.C_NUM_OF_PROBES {3} \
    CONFIG.C_PROBE0_WIDTH {64} \
    CONFIG.C_PROBE1_WIDTH {64} \
    CONFIG.C_PROBE2_TYPE {1} \
    CONFIG.C_PROBE2_WIDTH {512} \
  ] $ila_7


  # Create instance: port_to_msgbuff1, and set properties
  set port_to_msgbuff1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 port_to_msgbuff1 ]
  set_property CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} $port_to_msgbuff1


  # Create instance: port_to_metadata, and set properties
  set port_to_metadata [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 port_to_metadata ]
  set_property CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} $port_to_metadata


  # Create instance: interface_0, and set properties
  set interface_0 [ create_bd_cell -type ip -vlnv xilinx.com:hls:interface:1.0 interface_0 ]

  # Create interface connections
  connect_bd_intf_net -intf_net axi_bram_ctrl_0_BRAM_PORTA [get_bd_intf_pins axi_bram_ctrl_0/BRAM_PORTA] [get_bd_intf_pins blk_mem_gen_0/BRAM_PORTA]
  connect_bd_intf_net -intf_net axi_clock_converter_0_M_AXI [get_bd_intf_pins axi_clock_converter_0/M_AXI] [get_bd_intf_pins axi_bram_ctrl_0/S_AXI]
  connect_bd_intf_net -intf_net axi_clock_converter_1_M_AXI [get_bd_intf_pins axi_clock_converter_1/M_AXI] [get_bd_intf_pins xdma_0/S_AXI_B]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXI [get_bd_intf_pins axi_datamover_0/M_AXI] [get_bd_intf_pins axi_clock_converter_1/S_AXI]
connect_bd_intf_net -intf_net [get_bd_intf_nets axi_datamover_0_M_AXI] [get_bd_intf_pins axi_datamover_0/M_AXI] [get_bd_intf_pins ila_1/SLOT_0_AXI]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXIS_MM2S [get_bd_intf_pins axi_datamover_0/M_AXIS_MM2S] [get_bd_intf_pins homa/r_data_queue_i]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXIS_MM2S_STS [get_bd_intf_pins axi_datamover_0/M_AXIS_MM2S_STS] [get_bd_intf_pins homa/r_status_queue_i]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXIS_S2MM_STS [get_bd_intf_pins axi_datamover_0/M_AXIS_S2MM_STS] [get_bd_intf_pins homa/w_status_queue_i]
  connect_bd_intf_net -intf_net axis_data_fifo_0_M_AXIS [get_bd_intf_pins axis_data_fifo_0/M_AXIS] [get_bd_intf_pins interface_0/addr_in]
connect_bd_intf_net -intf_net [get_bd_intf_nets axis_data_fifo_0_M_AXIS] [get_bd_intf_pins axis_data_fifo_0/M_AXIS] [get_bd_intf_pins ila_5/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net axis_interconnect_0_M00_AXIS [get_bd_intf_pins interface_0/sendmsg] [get_bd_intf_pins homa/sendmsg_i]
connect_bd_intf_net -intf_net [get_bd_intf_nets axis_interconnect_0_M00_AXIS] [get_bd_intf_pins interface_0/sendmsg] [get_bd_intf_pins stream_out_ila2/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net axis_interconnect_0_M02_AXIS [get_bd_intf_pins interface_0/h2c_port_to_msgbuff] [get_bd_intf_pins homa/h2c_port_to_msgbuff_i]
connect_bd_intf_net -intf_net [get_bd_intf_nets axis_interconnect_0_M02_AXIS] [get_bd_intf_pins interface_0/h2c_port_to_msgbuff] [get_bd_intf_pins port_to_msgbuff/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net default_300mhz_clk0_1 [get_bd_intf_ports default_300mhz_clk0] [get_bd_intf_pins mainClk/CLK_IN1_D]
  connect_bd_intf_net -intf_net homa_1_link_egress [get_bd_intf_pins homa/link_egress_o] [get_bd_intf_pins homa/link_ingress_i]
  connect_bd_intf_net -intf_net homa_1_r_cmd_queue_o [get_bd_intf_pins homa/r_cmd_queue_o] [get_bd_intf_pins axi_datamover_0/S_AXIS_MM2S_CMD]
connect_bd_intf_net -intf_net [get_bd_intf_nets homa_1_r_cmd_queue_o] [get_bd_intf_pins homa/r_cmd_queue_o] [get_bd_intf_pins ila_0/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net homa_1_w_cmd_queue_o [get_bd_intf_pins homa/w_cmd_queue_o] [get_bd_intf_pins axi_datamover_0/S_AXIS_S2MM_CMD]
connect_bd_intf_net -intf_net [get_bd_intf_nets homa_1_w_cmd_queue_o] [get_bd_intf_pins homa/w_cmd_queue_o] [get_bd_intf_pins ila_3/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net homa_1_w_data_queue_o [get_bd_intf_pins homa/w_data_queue_o] [get_bd_intf_pins axi_datamover_0/S_AXIS_S2MM]
connect_bd_intf_net -intf_net [get_bd_intf_nets homa_1_w_data_queue_o] [get_bd_intf_pins homa/w_data_queue_o] [get_bd_intf_pins ila_4/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net interface_0_c2h_port_to_metadata [get_bd_intf_pins interface_0/c2h_port_to_metadata] [get_bd_intf_pins homa/c2h_port_to_metadata_i]
connect_bd_intf_net -intf_net [get_bd_intf_nets interface_0_c2h_port_to_metadata] [get_bd_intf_pins interface_0/c2h_port_to_metadata] [get_bd_intf_pins port_to_metadata/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net interface_0_c2h_port_to_msgbuff [get_bd_intf_pins interface_0/c2h_port_to_msgbuff] [get_bd_intf_pins homa/c2h_port_to_msgbuff_i]
connect_bd_intf_net -intf_net [get_bd_intf_nets interface_0_c2h_port_to_msgbuff] [get_bd_intf_pins interface_0/c2h_port_to_msgbuff] [get_bd_intf_pins port_to_msgbuff1/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net interface_0_infmem_PORTA [get_bd_intf_pins blk_mem_gen_0/BRAM_PORTB] [get_bd_intf_pins interface_0/infmem_PORTA]
  connect_bd_intf_net -intf_net interface_0_recvmsg [get_bd_intf_pins interface_0/recvmsg] [get_bd_intf_pins homa/recvmsg_i]
  connect_bd_intf_net -intf_net pcie_refclk_1 [get_bd_intf_ports pcie_refclk] [get_bd_intf_pins util_ds_buf/CLK_IN_D]
  connect_bd_intf_net -intf_net xdma_0_M_AXI_B [get_bd_intf_pins xdma_0/M_AXI_B] [get_bd_intf_pins axi_clock_converter_0/S_AXI]
  connect_bd_intf_net -intf_net xdma_0_pcie_mgt [get_bd_intf_ports pci_express_x16] [get_bd_intf_pins xdma_0/pcie_mgt]

  # Create port connections
  connect_bd_net -net Net [get_bd_pins blk_mem_gen_0/doutb] [get_bd_pins ila_7/probe2] [get_bd_pins interface_0/infmem_Dout_A]
  connect_bd_net -net axi_bram_ctrl_0_bram_addr_a [get_bd_pins axi_bram_ctrl_0/bram_addr_a] [get_bd_pins ila_2/probe0] [get_bd_pins blk_mem_gen_0/addra]
  connect_bd_net -net axi_bram_ctrl_0_bram_we_a [get_bd_pins axi_bram_ctrl_0/bram_we_a] [get_bd_pins ila_2/probe1] [get_bd_pins blk_mem_gen_0/wea]
  connect_bd_net -net axi_bram_ctrl_0_bram_wrdata_a [get_bd_pins axi_bram_ctrl_0/bram_wrdata_a] [get_bd_pins ila_2/probe2] [get_bd_pins blk_mem_gen_0/dina]
  connect_bd_net -net axi_clock_converter_0_m_axi_awaddr [get_bd_pins axi_clock_converter_0/m_axi_awaddr] [get_bd_pins axis_data_fifo_0/s_axis_tdata] [get_bd_pins axi_bram_ctrl_0/s_axi_awaddr] [get_bd_pins ila_6/probe1]
  connect_bd_net -net axi_clock_converter_0_m_axi_wvalid [get_bd_pins axi_clock_converter_0/m_axi_wvalid] [get_bd_pins axis_data_fifo_0/s_axis_tvalid] [get_bd_pins axi_bram_ctrl_0/s_axi_wvalid] [get_bd_pins ila_6/probe7]
  connect_bd_net -net interface_0_infmem_Addr_A [get_bd_pins interface_0/infmem_Addr_A] [get_bd_pins ila_7/probe1] [get_bd_pins blk_mem_gen_0/addrb]
  connect_bd_net -net interface_0_infmem_WEN_A [get_bd_pins interface_0/infmem_WEN_A] [get_bd_pins ila_7/probe0] [get_bd_pins blk_mem_gen_0/web]
  connect_bd_net -net mainClk_clk_out1 [get_bd_pins mainClk/clk_out1] [get_bd_pins axi_clock_converter_0/m_axi_aclk] [get_bd_pins axi_clock_converter_1/s_axi_aclk] [get_bd_pins axi_datamover_0/m_axi_mm2s_aclk] [get_bd_pins axi_datamover_0/m_axis_mm2s_cmdsts_aclk] [get_bd_pins axi_datamover_0/m_axi_s2mm_aclk] [get_bd_pins axi_datamover_0/m_axis_s2mm_cmdsts_awclk] [get_bd_pins proc_sys_reset_0/slowest_sync_clk] [get_bd_pins ila_1/clk] [get_bd_pins ila_0/clk] [get_bd_pins ila_3/clk] [get_bd_pins ila_4/clk] [get_bd_pins port_to_msgbuff/clk] [get_bd_pins stream_out_ila2/clk] [get_bd_pins axi_bram_ctrl_0/s_axi_aclk] [get_bd_pins axis_data_fifo_0/s_axis_aclk] [get_bd_pins ila_6/clk] [get_bd_pins ila_5/clk] [get_bd_pins homa/ap_clk] [get_bd_pins ila_2/clk] [get_bd_pins ila_7/clk] [get_bd_pins port_to_metadata/clk] [get_bd_pins port_to_msgbuff1/clk] [get_bd_pins interface_0/ap_clk]
  connect_bd_net -net mainClk_locked [get_bd_pins mainClk/locked] [get_bd_pins proc_sys_reset_0/dcm_locked]
  connect_bd_net -net pcie_perstn_1 [get_bd_ports pcie_perstn] [get_bd_pins xdma_0/sys_rst_n]
  connect_bd_net -net proc_sys_reset_0_peripheral_aresetn [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins axi_datamover_0/m_axis_s2mm_cmdsts_aresetn] [get_bd_pins axi_datamover_0/m_axi_s2mm_aresetn] [get_bd_pins axi_datamover_0/m_axis_mm2s_cmdsts_aresetn] [get_bd_pins axi_datamover_0/m_axi_mm2s_aresetn] [get_bd_pins axi_clock_converter_1/s_axi_aresetn] [get_bd_pins axi_clock_converter_0/m_axi_aresetn] [get_bd_pins axi_bram_ctrl_0/s_axi_aresetn] [get_bd_pins axis_data_fifo_0/s_axis_aresetn] [get_bd_pins homa/ap_rst_n] [get_bd_pins interface_0/ap_rst_n]
  connect_bd_net -net resetn_2 [get_bd_ports resetn] [get_bd_pins proc_sys_reset_0/ext_reset_in] [get_bd_pins mainClk/resetn]
  connect_bd_net -net util_ds_buf_IBUF_DS_ODIV2 [get_bd_pins util_ds_buf/IBUF_DS_ODIV2] [get_bd_pins xdma_0/sys_clk]
  connect_bd_net -net util_ds_buf_IBUF_OUT [get_bd_pins util_ds_buf/IBUF_OUT] [get_bd_pins xdma_0/sys_clk_gt]
  connect_bd_net -net xdma_0_axi_aclk [get_bd_pins xdma_0/axi_aclk] [get_bd_pins axi_clock_converter_0/s_axi_aclk] [get_bd_pins axi_clock_converter_1/m_axi_aclk]
  connect_bd_net -net xdma_0_axi_aresetn [get_bd_pins xdma_0/axi_aresetn] [get_bd_pins axi_clock_converter_0/s_axi_aresetn] [get_bd_pins axi_clock_converter_1/m_axi_aresetn]
  connect_bd_net -net xlconstant_0_dout [get_bd_pins xlconstant_0/dout] [get_bd_pins xdma_0/usr_irq_req]

  # Create address segments
  assign_bd_address -offset 0x00000000 -range 0x00010000 -target_address_space [get_bd_addr_spaces xdma_0/M_AXI_B] [get_bd_addr_segs axi_bram_ctrl_0/S_AXI/Mem0] -force
  assign_bd_address -offset 0x00000000 -range 0x008000000000 -target_address_space [get_bd_addr_spaces axi_datamover_0/Data] [get_bd_addr_segs xdma_0/S_AXI_B/BAR0] -force


  # Restore current instance
  current_bd_instance $oldCurInst

  validate_bd_design
  save_bd_design
}
# End of create_root_design()


##################################################################
# MAIN FLOW
##################################################################

create_root_design ""


