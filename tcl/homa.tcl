
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
xilinx.com:ip:axi_fifo_mm_s:4.3\
xilinx.com:ip:axis_dwidth_converter:1.1\
xilinx.com:ip:axi_clock_converter:2.1\
xilinx.com:ip:clk_wiz:6.0\
xilinx.com:ip:proc_sys_reset:5.0\
xilinx.com:ip:xdma:4.1\
xilinx.com:ip:util_ds_buf:2.2\
xilinx.com:ip:axi_datamover:5.1\
xilinx.com:hls:homa:1.0\
xilinx.com:ip:xlconstant:1.1\
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

  # Create instance: homa_sendmsg_fifo, and set properties
  set homa_sendmsg_fifo [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_fifo_mm_s:4.3 homa_sendmsg_fifo ]
  set_property -dict [list \
    CONFIG.C_AXIS_TUSER_WIDTH {4} \
    CONFIG.C_DATA_INTERFACE_TYPE {1} \
    CONFIG.C_HAS_AXIS_TKEEP {false} \
    CONFIG.C_S_AXI4_DATA_WIDTH {32} \
    CONFIG.C_USE_TX_CTRL {0} \
  ] $homa_sendmsg_fifo


  # Create instance: homa_recvmsg_fifo, and set properties
  set homa_recvmsg_fifo [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_fifo_mm_s:4.3 homa_recvmsg_fifo ]
  set_property -dict [list \
    CONFIG.C_AXIS_TUSER_WIDTH {4} \
    CONFIG.C_DATA_INTERFACE_TYPE {1} \
    CONFIG.C_S_AXI4_DATA_WIDTH {32} \
    CONFIG.C_USE_TX_CTRL {0} \
  ] $homa_recvmsg_fifo


  # Create instance: sendmsg_32_to_512, and set properties
  set sendmsg_32_to_512 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_dwidth_converter:1.1 sendmsg_32_to_512 ]
  set_property CONFIG.M_TDATA_NUM_BYTES {64} $sendmsg_32_to_512


  # Create instance: sendmsg_512_32, and set properties
  set sendmsg_512_32 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_dwidth_converter:1.1 sendmsg_512_32 ]
  set_property -dict [list \
    CONFIG.M_TDATA_NUM_BYTES {4} \
    CONFIG.S_TDATA_NUM_BYTES {64} \
  ] $sendmsg_512_32


  # Create instance: recvmsg_32_512, and set properties
  set recvmsg_32_512 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_dwidth_converter:1.1 recvmsg_32_512 ]
  set_property CONFIG.M_TDATA_NUM_BYTES {65} $recvmsg_32_512


  # Create instance: recvmsg_512_32, and set properties
  set recvmsg_512_32 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_dwidth_converter:1.1 recvmsg_512_32 ]
  set_property CONFIG.M_TDATA_NUM_BYTES {4} $recvmsg_512_32


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
    CONFIG.axibar_num {1} \
    CONFIG.en_axi_slave_if {true} \
    CONFIG.en_transceiver_status_ports {false} \
    CONFIG.functional_mode {AXI_Bridge} \
    CONFIG.mode_selection {Advanced} \
    CONFIG.pf0_bar0_scale {Megabytes} \
    CONFIG.pf0_bar0_size {2} \
    CONFIG.pf0_bar1_enabled {false} \
    CONFIG.xdma_axi_intf_mm {AXI_Memory_Mapped} \
    CONFIG.xdma_rnum_chnl {4} \
    CONFIG.xdma_wnum_chnl {4} \
  ] $xdma_0


  # Create instance: util_ds_buf_1, and set properties
  set util_ds_buf_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_ds_buf:2.2 util_ds_buf_1 ]
  set_property -dict [list \
    CONFIG.DIFF_CLK_IN_BOARD_INTERFACE {pcie_refclk} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $util_ds_buf_1


  # Create instance: axi_datamover_0, and set properties
  set axi_datamover_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_datamover:5.1 axi_datamover_0 ]
  set_property -dict [list \
    CONFIG.c_dummy {1} \
    CONFIG.c_enable_mm2s_adv_sig {0} \
    CONFIG.c_enable_s2mm_adv_sig {0} \
    CONFIG.c_include_mm2s_dre {true} \
    CONFIG.c_include_s2mm_dre {true} \
    CONFIG.c_m_axi_mm2s_data_width {512} \
    CONFIG.c_m_axi_s2mm_data_width {512} \
    CONFIG.c_m_axis_mm2s_tdata_width {512} \
    CONFIG.c_mm2s_btt_used {23} \
    CONFIG.c_mm2s_include_sf {false} \
    CONFIG.c_s2mm_btt_used {23} \
    CONFIG.c_s2mm_burst_size {2} \
    CONFIG.c_s_axis_s2mm_tdata_width {512} \
  ] $axi_datamover_0


  # Create instance: homa, and set properties
  set homa [ create_bd_cell -type ip -vlnv xilinx.com:hls:homa:1.0 homa ]

  # Create instance: axi_interconnect_1, and set properties
  set axi_interconnect_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_1 ]
  set_property -dict [list \
    CONFIG.NUM_MI {1} \
    CONFIG.NUM_SI {2} \
  ] $axi_interconnect_1


  # Create instance: xlconstant_0, and set properties
  set xlconstant_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_0 ]
  set_property CONFIG.CONST_VAL {0} $xlconstant_0


  # Create instance: axi_interconnect_2, and set properties
  set axi_interconnect_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_2 ]
  set_property -dict [list \
    CONFIG.NUM_MI {2} \
    CONFIG.NUM_SI {1} \
  ] $axi_interconnect_2


  # Create instance: axi_interconnect_3, and set properties
  set axi_interconnect_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_3 ]
  set_property -dict [list \
    CONFIG.NUM_MI {4} \
    CONFIG.NUM_SI {1} \
  ] $axi_interconnect_3


  # Create interface connections
  connect_bd_intf_net -intf_net axi_clock_converter_0_M_AXI [get_bd_intf_pins axi_clock_converter_0/M_AXI] [get_bd_intf_pins axi_interconnect_3/S00_AXI]
  connect_bd_intf_net -intf_net axi_clock_converter_1_M_AXI [get_bd_intf_pins axi_clock_converter_1/M_AXI] [get_bd_intf_pins axi_interconnect_2/S00_AXI]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXIS_MM2S [get_bd_intf_pins axi_datamover_0/M_AXIS_MM2S] [get_bd_intf_pins homa/r_data_queue_i]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXIS_MM2S_STS [get_bd_intf_pins axi_datamover_0/M_AXIS_MM2S_STS] [get_bd_intf_pins homa/r_status_queue_i]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXIS_S2MM_STS [get_bd_intf_pins axi_datamover_0/M_AXIS_S2MM_STS] [get_bd_intf_pins homa/w_status_queue_i]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXI_MM2S [get_bd_intf_pins axi_datamover_0/M_AXI_MM2S] [get_bd_intf_pins axi_interconnect_1/S00_AXI]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXI_S2MM [get_bd_intf_pins axi_datamover_0/M_AXI_S2MM] [get_bd_intf_pins axi_interconnect_1/S01_AXI]
  connect_bd_intf_net -intf_net axi_fifo_mm_s_0_AXI_STR_TXD [get_bd_intf_pins homa_sendmsg_fifo/AXI_STR_TXD] [get_bd_intf_pins sendmsg_32_to_512/S_AXIS]
  connect_bd_intf_net -intf_net axi_fifo_mm_s_1_AXI_STR_TXD [get_bd_intf_pins recvmsg_32_512/S_AXIS] [get_bd_intf_pins homa_recvmsg_fifo/AXI_STR_TXD]
  connect_bd_intf_net -intf_net axi_interconnect_1_M00_AXI [get_bd_intf_pins axi_interconnect_1/M00_AXI] [get_bd_intf_pins axi_clock_converter_1/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_2_M00_AXI [get_bd_intf_pins axi_interconnect_2/M00_AXI] [get_bd_intf_pins xdma_0/S_AXI_B]
  connect_bd_intf_net -intf_net axi_interconnect_2_M01_AXI [get_bd_intf_pins axi_interconnect_2/M01_AXI] [get_bd_intf_pins xdma_0/S_AXI_LITE]
  connect_bd_intf_net -intf_net axi_interconnect_3_M00_AXI [get_bd_intf_pins axi_interconnect_3/M00_AXI] [get_bd_intf_pins homa_sendmsg_fifo/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_3_M01_AXI [get_bd_intf_pins axi_interconnect_3/M01_AXI] [get_bd_intf_pins homa_sendmsg_fifo/S_AXI_FULL]
  connect_bd_intf_net -intf_net axi_interconnect_3_M02_AXI [get_bd_intf_pins axi_interconnect_3/M02_AXI] [get_bd_intf_pins homa_recvmsg_fifo/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_3_M03_AXI [get_bd_intf_pins axi_interconnect_3/M03_AXI] [get_bd_intf_pins homa_recvmsg_fifo/S_AXI_FULL]
  connect_bd_intf_net -intf_net axis_dwidth_converter_0_M_AXIS [get_bd_intf_pins sendmsg_32_to_512/M_AXIS] [get_bd_intf_pins homa/msghdr_send_i]
  connect_bd_intf_net -intf_net axis_dwidth_converter_1_M_AXIS [get_bd_intf_pins sendmsg_512_32/M_AXIS] [get_bd_intf_pins homa_sendmsg_fifo/AXI_STR_RXD]
  connect_bd_intf_net -intf_net axis_dwidth_converter_2_M_AXIS [get_bd_intf_pins recvmsg_32_512/M_AXIS] [get_bd_intf_pins homa/msghdr_recv_i]
  connect_bd_intf_net -intf_net axis_dwidth_converter_3_M_AXIS [get_bd_intf_pins recvmsg_512_32/M_AXIS] [get_bd_intf_pins homa_recvmsg_fifo/AXI_STR_RXD]
  connect_bd_intf_net -intf_net default_300mhz_clk0_1 [get_bd_intf_ports default_300mhz_clk0] [get_bd_intf_pins mainClk/CLK_IN1_D]
  connect_bd_intf_net -intf_net homa_1_link_egress [get_bd_intf_pins homa/link_egress_o] [get_bd_intf_pins homa/link_ingress_i]
  connect_bd_intf_net -intf_net homa_1_msghdr_recv_o [get_bd_intf_pins homa/msghdr_recv_o] [get_bd_intf_pins recvmsg_512_32/S_AXIS]
  connect_bd_intf_net -intf_net homa_1_msghdr_send_o [get_bd_intf_pins homa/msghdr_send_o] [get_bd_intf_pins sendmsg_512_32/S_AXIS]
  connect_bd_intf_net -intf_net homa_1_r_cmd_queue_o [get_bd_intf_pins homa/r_cmd_queue_o] [get_bd_intf_pins axi_datamover_0/S_AXIS_MM2S_CMD]
  connect_bd_intf_net -intf_net homa_1_w_cmd_queue_o [get_bd_intf_pins homa/w_cmd_queue_o] [get_bd_intf_pins axi_datamover_0/S_AXIS_S2MM_CMD]
  connect_bd_intf_net -intf_net homa_1_w_data_queue_o [get_bd_intf_pins homa/w_data_queue_o] [get_bd_intf_pins axi_datamover_0/S_AXIS_S2MM]
  connect_bd_intf_net -intf_net pcie_refclk_1 [get_bd_intf_ports pcie_refclk] [get_bd_intf_pins util_ds_buf_1/CLK_IN_D]
  connect_bd_intf_net -intf_net xdma_0_M_AXI_B [get_bd_intf_pins xdma_0/M_AXI_B] [get_bd_intf_pins axi_clock_converter_0/S_AXI]
  connect_bd_intf_net -intf_net xdma_0_pcie_mgt [get_bd_intf_ports pci_express_x16] [get_bd_intf_pins xdma_0/pcie_mgt]

  # Create port connections
  connect_bd_net -net ARESETN_1 [get_bd_pins proc_sys_reset_0/interconnect_aresetn] [get_bd_pins axi_interconnect_3/ARESETN] [get_bd_pins axi_interconnect_1/ARESETN]
  connect_bd_net -net mainClk_clk_out1 [get_bd_pins mainClk/clk_out1] [get_bd_pins axi_clock_converter_0/m_axi_aclk] [get_bd_pins axi_clock_converter_1/s_axi_aclk] [get_bd_pins homa_sendmsg_fifo/s_axi_aclk] [get_bd_pins homa_recvmsg_fifo/s_axi_aclk] [get_bd_pins sendmsg_32_to_512/aclk] [get_bd_pins sendmsg_512_32/aclk] [get_bd_pins recvmsg_32_512/aclk] [get_bd_pins recvmsg_512_32/aclk] [get_bd_pins axi_interconnect_1/ACLK] [get_bd_pins axi_interconnect_1/S00_ACLK] [get_bd_pins axi_interconnect_1/M00_ACLK] [get_bd_pins axi_interconnect_1/S01_ACLK] [get_bd_pins axi_datamover_0/m_axi_mm2s_aclk] [get_bd_pins axi_datamover_0/m_axis_mm2s_cmdsts_aclk] [get_bd_pins axi_datamover_0/m_axi_s2mm_aclk] [get_bd_pins axi_datamover_0/m_axis_s2mm_cmdsts_awclk] [get_bd_pins axi_interconnect_3/ACLK] [get_bd_pins axi_interconnect_3/S00_ACLK] [get_bd_pins axi_interconnect_3/M00_ACLK] [get_bd_pins axi_interconnect_3/M01_ACLK] [get_bd_pins axi_interconnect_3/M02_ACLK] [get_bd_pins axi_interconnect_3/M03_ACLK] [get_bd_pins proc_sys_reset_0/slowest_sync_clk] [get_bd_pins homa/ap_clk]
  connect_bd_net -net mainClk_locked [get_bd_pins mainClk/locked] [get_bd_pins proc_sys_reset_0/dcm_locked]
  connect_bd_net -net pcie_perstn_1 [get_bd_ports pcie_perstn] [get_bd_pins xdma_0/sys_rst_n]
  connect_bd_net -net proc_sys_reset_0_peripheral_aresetn [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins recvmsg_512_32/aresetn] [get_bd_pins sendmsg_512_32/aresetn] [get_bd_pins homa_recvmsg_fifo/s_axi_aresetn] [get_bd_pins recvmsg_32_512/aresetn] [get_bd_pins sendmsg_32_to_512/aresetn] [get_bd_pins homa_sendmsg_fifo/s_axi_aresetn] [get_bd_pins axi_datamover_0/m_axis_s2mm_cmdsts_aresetn] [get_bd_pins axi_datamover_0/m_axi_s2mm_aresetn] [get_bd_pins axi_datamover_0/m_axis_mm2s_cmdsts_aresetn] [get_bd_pins axi_datamover_0/m_axi_mm2s_aresetn] [get_bd_pins axi_clock_converter_1/s_axi_aresetn] [get_bd_pins axi_interconnect_1/S01_ARESETN] [get_bd_pins axi_interconnect_1/M00_ARESETN] [get_bd_pins axi_interconnect_1/S00_ARESETN] [get_bd_pins axi_interconnect_3/S00_ARESETN] [get_bd_pins axi_interconnect_3/M00_ARESETN] [get_bd_pins axi_interconnect_3/M01_ARESETN] [get_bd_pins axi_interconnect_3/M02_ARESETN] [get_bd_pins axi_interconnect_3/M03_ARESETN] [get_bd_pins homa/ap_rst_n] [get_bd_pins axi_clock_converter_0/m_axi_aresetn]
  connect_bd_net -net resetn_2 [get_bd_ports resetn] [get_bd_pins proc_sys_reset_0/ext_reset_in] [get_bd_pins mainClk/resetn]
  connect_bd_net -net util_ds_buf_1_IBUF_DS_ODIV2 [get_bd_pins util_ds_buf_1/IBUF_DS_ODIV2] [get_bd_pins xdma_0/sys_clk]
  connect_bd_net -net util_ds_buf_1_IBUF_OUT [get_bd_pins util_ds_buf_1/IBUF_OUT] [get_bd_pins xdma_0/sys_clk_gt]
  connect_bd_net -net xdma_0_axi_aclk [get_bd_pins xdma_0/axi_aclk] [get_bd_pins axi_clock_converter_0/s_axi_aclk] [get_bd_pins axi_clock_converter_1/m_axi_aclk] [get_bd_pins axi_interconnect_2/ACLK] [get_bd_pins axi_interconnect_2/S00_ACLK] [get_bd_pins axi_interconnect_2/M00_ACLK] [get_bd_pins axi_interconnect_2/M01_ACLK]
  connect_bd_net -net xdma_0_axi_aresetn [get_bd_pins xdma_0/axi_aresetn] [get_bd_pins axi_clock_converter_0/s_axi_aresetn] [get_bd_pins axi_clock_converter_1/m_axi_aresetn] [get_bd_pins axi_interconnect_2/ARESETN] [get_bd_pins axi_interconnect_2/S00_ARESETN] [get_bd_pins axi_interconnect_2/M00_ARESETN] [get_bd_pins axi_interconnect_2/M01_ARESETN]
  connect_bd_net -net xlconstant_0_dout [get_bd_pins xlconstant_0/dout] [get_bd_pins xdma_0/usr_irq_req]

  # Create address segments
  assign_bd_address -offset 0x00106000 -range 0x00001000 -target_address_space [get_bd_addr_spaces xdma_0/M_AXI_B] [get_bd_addr_segs homa_recvmsg_fifo/S_AXI/Mem0] -force
  assign_bd_address -offset 0x00102000 -range 0x00002000 -target_address_space [get_bd_addr_spaces xdma_0/M_AXI_B] [get_bd_addr_segs homa_recvmsg_fifo/S_AXI_FULL/Mem1] -force
  assign_bd_address -offset 0x00101000 -range 0x00001000 -target_address_space [get_bd_addr_spaces xdma_0/M_AXI_B] [get_bd_addr_segs homa_sendmsg_fifo/S_AXI/Mem0] -force
  assign_bd_address -offset 0x00104000 -range 0x00002000 -target_address_space [get_bd_addr_spaces xdma_0/M_AXI_B] [get_bd_addr_segs homa_sendmsg_fifo/S_AXI_FULL/Mem1] -force
  assign_bd_address -offset 0x00000000 -range 0x00100000 -target_address_space [get_bd_addr_spaces axi_datamover_0/Data_MM2S] [get_bd_addr_segs xdma_0/S_AXI_B/BAR0] -force
  assign_bd_address -offset 0x00100000 -range 0x00001000 -target_address_space [get_bd_addr_spaces axi_datamover_0/Data_MM2S] [get_bd_addr_segs xdma_0/S_AXI_LITE/CTL0] -force
  assign_bd_address -offset 0x00000000 -range 0x00100000 -target_address_space [get_bd_addr_spaces axi_datamover_0/Data_S2MM] [get_bd_addr_segs xdma_0/S_AXI_B/BAR0] -force
  assign_bd_address -offset 0x00100000 -range 0x00001000 -target_address_space [get_bd_addr_spaces axi_datamover_0/Data_S2MM] [get_bd_addr_segs xdma_0/S_AXI_LITE/CTL0] -force


  # Restore current instance
  current_bd_instance $oldCurInst

  save_bd_design
}
# End of create_root_design()


##################################################################
# MAIN FLOW
##################################################################

create_root_design ""


common::send_gid_msg -ssname BD::TCL -id 2053 -severity "WARNING" "This Tcl script was generated from a block design that has not been validated. It is possible that design <$design_name> may result in errors during validation."

