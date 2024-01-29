
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
set scripts_vivado_version 2023.2
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
   puts ""
   if { [string compare $scripts_vivado_version $current_vivado_version] > 0 } {
      catch {common::send_gid_msg -ssname BD::TCL -id 2042 -severity "ERROR" " This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Sourcing the script failed since it was created with a future version of Vivado."}

   } else {
     catch {common::send_gid_msg -ssname BD::TCL -id 2041 -severity "ERROR" "This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_bd_tcl to create an updated script."}

   }

   return 1
}

################################################################
# START
################################################################

# To test this script, run the following commands from Vivado Tcl console:
# source homa_script.tcl


# The design that will be created by this Tcl script contains the following 
# module references:
# dma_psdpram, pcie, dma_client_axis_source, dma_client_axis_sink, dma_psdpram, srpt_queue, srpt_queue, axi2axis, picorv32_axi

# Please add the sources of those modules before sourcing this Tcl script.

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
xilinx.com:ip:proc_sys_reset:5.0\
xilinx.com:ip:clk_wiz:6.0\
xilinx.com:ip:axi_bram_ctrl:4.1\
xilinx.com:ip:blk_mem_gen:8.4\
xilinx.com:ip:system_ila:1.1\
xilinx.com:hls:addr_map:1.0\
xilinx.com:hls:c2h_dma:1.0\
xilinx.com:hls:h2c_dma:1.0\
xilinx.com:ip:axi_clock_converter:2.1\
xilinx.com:ip:axis_clock_converter:1.1\
xilinx.com:ip:util_vector_logic:2.0\
xilinx.com:ip:xlconstant:1.1\
xilinx.com:hls:interface:1.0\
xilinx.com:hls:user:1.0\
xilinx.com:hls:pkt_ctor:1.0\
xilinx.com:hls:pkt_dtor:1.0\
xilinx.com:ip:fifo_generator:13.2\
xilinx.com:ip:ila:6.2\
xilinx.com:ip:cmac_usplus:3.1\
xilinx.com:ip:xlslice:1.0\
xilinx.com:ip:axi_gpio:2.0\
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

##################################################################
# CHECK Modules
##################################################################
set bCheckModules 1
if { $bCheckModules == 1 } {
   set list_check_mods "\ 
dma_psdpram\
pcie\
dma_client_axis_source\
dma_client_axis_sink\
dma_psdpram\
srpt_queue\
srpt_queue\
axi2axis\
picorv32_axi\
"

   set list_mods_missing ""
   common::send_gid_msg -ssname BD::TCL -id 2020 -severity "INFO" "Checking if the following modules exist in the project's sources: $list_check_mods ."

   foreach mod_vlnv $list_check_mods {
      if { [can_resolve_reference $mod_vlnv] == 0 } {
         lappend list_mods_missing $mod_vlnv
      }
   }

   if { $list_mods_missing ne "" } {
      catch {common::send_gid_msg -ssname BD::TCL -id 2021 -severity "ERROR" "The following module(s) are not found in the project: $list_mods_missing" }
      common::send_gid_msg -ssname BD::TCL -id 2022 -severity "INFO" "Please add source files for the missing module(s) above."
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


# Hierarchical cell: softproc
proc create_hier_cell_softproc { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_softproc() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI1


  # Create pins
  create_bd_pin -dir I -type rst s_axi_aresetn
  create_bd_pin -dir I -type clk s_axi_aclk
  create_bd_pin -dir I -type clk slowest_sync_clk
  create_bd_pin -dir I dcm_locked
  create_bd_pin -dir I -type rst resetn

  # Create instance: cpu_bram, and set properties
  set cpu_bram [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 cpu_bram ]
  set_property -dict [list \
    CONFIG.Enable_32bit_Address {true} \
    CONFIG.Enable_B {Use_ENB_Pin} \
    CONFIG.Memory_Type {True_Dual_Port_RAM} \
    CONFIG.Port_B_Clock {100} \
    CONFIG.Port_B_Enable_Rate {100} \
    CONFIG.Port_B_Write_Rate {50} \
    CONFIG.Register_PortA_Output_of_Memory_Primitives {false} \
    CONFIG.Register_PortB_Output_of_Memory_Primitives {false} \
    CONFIG.Use_RSTA_Pin {true} \
    CONFIG.Use_RSTB_Pin {true} \
    CONFIG.use_bram_block {BRAM_Controller} \
  ] $cpu_bram


  # Create instance: axi_bram_ctrl_1, and set properties
  set axi_bram_ctrl_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_1 ]
  set_property CONFIG.SINGLE_PORT_BRAM {1} $axi_bram_ctrl_1


  # Create instance: system_ila_0, and set properties
  set system_ila_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 system_ila_0 ]
  set_property CONFIG.C_MON_TYPE {INTERFACE} $system_ila_0


  # Create instance: axi_bram_ctrl_2, and set properties
  set axi_bram_ctrl_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_2 ]
  set_property -dict [list \
    CONFIG.PROTOCOL {AXI4LITE} \
    CONFIG.SINGLE_PORT_BRAM {1} \
  ] $axi_bram_ctrl_2


  # Create instance: picorv32_axi_0, and set properties
  set block_name picorv32_axi
  set block_cell_name picorv32_axi_0
  if { [catch {set picorv32_axi_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $picorv32_axi_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
    set_property -dict [list \
    CONFIG.ENABLE_DIV {1} \
    CONFIG.ENABLE_FAST_MUL {1} \
    CONFIG.ENABLE_MUL {1} \
    CONFIG.ENABLE_PCPI {0} \
  ] $picorv32_axi_0


  # Create instance: axi_interconnect_3, and set properties
  set axi_interconnect_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_3 ]

  # Create instance: util_vector_logic_1, and set properties
  set util_vector_logic_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 util_vector_logic_1 ]
  set_property -dict [list \
    CONFIG.C_OPERATION {not} \
    CONFIG.C_SIZE {1} \
  ] $util_vector_logic_1


  # Create instance: rst_mainClk_200M, and set properties
  set rst_mainClk_200M [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_mainClk_200M ]

  # Create instance: xlslice_1, and set properties
  set xlslice_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlslice:1.0 xlslice_1 ]
  set_property -dict [list \
    CONFIG.DIN_FROM {0} \
    CONFIG.DIN_TO {0} \
  ] $xlslice_1


  # Create instance: axi_gpio_0, and set properties
  set axi_gpio_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_0 ]
  set_property -dict [list \
    CONFIG.C_ALL_OUTPUTS {1} \
    CONFIG.C_DOUT_DEFAULT {0x00000000} \
    CONFIG.C_GPIO_WIDTH {1} \
  ] $axi_gpio_0


  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins axi_bram_ctrl_1/S_AXI] [get_bd_intf_pins S_AXI]
  connect_bd_intf_net -intf_net Conn2 [get_bd_intf_pins axi_bram_ctrl_2/S_AXI] [get_bd_intf_pins axi_interconnect_3/M00_AXI]
  connect_bd_intf_net -intf_net [get_bd_intf_nets Conn2] [get_bd_intf_pins axi_bram_ctrl_2/S_AXI] [get_bd_intf_pins system_ila_0/SLOT_0_AXI]
  connect_bd_intf_net -intf_net Conn3 [get_bd_intf_pins axi_gpio_0/S_AXI] [get_bd_intf_pins S_AXI1]
  connect_bd_intf_net -intf_net axi_bram_ctrl_1_BRAM_PORTA [get_bd_intf_pins cpu_bram/BRAM_PORTA] [get_bd_intf_pins axi_bram_ctrl_1/BRAM_PORTA]
  connect_bd_intf_net -intf_net axi_bram_ctrl_2_BRAM_PORTA [get_bd_intf_pins cpu_bram/BRAM_PORTB] [get_bd_intf_pins axi_bram_ctrl_2/BRAM_PORTA]
  connect_bd_intf_net -intf_net picorv32_axi_0_mem_axi [get_bd_intf_pins picorv32_axi_0/mem_axi] [get_bd_intf_pins axi_interconnect_3/S00_AXI]

  # Create port connections
  connect_bd_net -net axi_gpio_0_gpio_io_o [get_bd_pins axi_gpio_0/gpio_io_o] [get_bd_pins xlslice_1/Din]
  connect_bd_net -net dcm_locked_1 [get_bd_pins dcm_locked] [get_bd_pins rst_mainClk_200M/dcm_locked]
  connect_bd_net -net resetn_1 [get_bd_pins resetn] [get_bd_pins rst_mainClk_200M/ext_reset_in]
  connect_bd_net -net rst_mainClk_200M_mb_reset [get_bd_pins rst_mainClk_200M/mb_reset] [get_bd_pins util_vector_logic_1/Op1]
  connect_bd_net -net s_axi_aclk_1 [get_bd_pins s_axi_aclk] [get_bd_pins picorv32_axi_0/clk] [get_bd_pins axi_bram_ctrl_1/s_axi_aclk] [get_bd_pins axi_bram_ctrl_2/s_axi_aclk] [get_bd_pins axi_interconnect_3/ACLK] [get_bd_pins axi_interconnect_3/S00_ACLK] [get_bd_pins axi_interconnect_3/M00_ACLK] [get_bd_pins axi_interconnect_3/M01_ACLK] [get_bd_pins system_ila_0/clk] [get_bd_pins axi_gpio_0/s_axi_aclk]
  connect_bd_net -net s_axi_aresetn_1 [get_bd_pins s_axi_aresetn] [get_bd_pins axi_bram_ctrl_1/s_axi_aresetn] [get_bd_pins axi_bram_ctrl_2/s_axi_aresetn] [get_bd_pins axi_interconnect_3/ARESETN] [get_bd_pins axi_interconnect_3/S00_ARESETN] [get_bd_pins axi_interconnect_3/M00_ARESETN] [get_bd_pins axi_interconnect_3/M01_ARESETN] [get_bd_pins system_ila_0/resetn] [get_bd_pins axi_gpio_0/s_axi_aresetn]
  connect_bd_net -net slowest_sync_clk_1 [get_bd_pins slowest_sync_clk] [get_bd_pins rst_mainClk_200M/slowest_sync_clk]
  connect_bd_net -net util_vector_logic_1_Res [get_bd_pins util_vector_logic_1/Res] [get_bd_pins picorv32_axi_0/resetn]
  connect_bd_net -net xlslice_1_Dout [get_bd_pins xlslice_1/Dout] [get_bd_pins rst_mainClk_200M/aux_reset_in]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: link
proc create_hier_cell_link { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_link() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 M_AXIS

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 S_AXIS

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:gt_rtl:1.0 qsfp1_4x

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 qsfp1_156mhz


  # Create pins
  create_bd_pin -dir I -type rst m_axi_aresetn
  create_bd_pin -dir I -type clk m_aclk
  create_bd_pin -dir I -type clk m_axi_aclk
  create_bd_pin -dir I -type rst s_axi_aresetn
  create_bd_pin -dir I -type clk s_axi_aclk
  create_bd_pin -dir I -type rst sys_reset

  # Create instance: fifo_generator_1, and set properties
  set fifo_generator_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:fifo_generator:13.2 fifo_generator_1 ]
  set_property -dict [list \
    CONFIG.Clock_Type_AXI {Independent_Clock} \
    CONFIG.Enable_TLAST {true} \
    CONFIG.FIFO_Application_Type_axis {Data_FIFO} \
    CONFIG.FIFO_Implementation_axis {Independent_Clocks_Block_RAM} \
    CONFIG.FIFO_Implementation_rach {Independent_Clocks_Distributed_RAM} \
    CONFIG.FIFO_Implementation_rdch {Independent_Clocks_Builtin_FIFO} \
    CONFIG.FIFO_Implementation_wach {Independent_Clocks_Distributed_RAM} \
    CONFIG.FIFO_Implementation_wdch {Independent_Clocks_Builtin_FIFO} \
    CONFIG.FIFO_Implementation_wrch {Independent_Clocks_Distributed_RAM} \
    CONFIG.HAS_TKEEP {true} \
    CONFIG.HAS_TSTRB {false} \
    CONFIG.INTERFACE_TYPE {AXI_STREAM} \
    CONFIG.Input_Depth_axis {32} \
    CONFIG.TDATA_NUM_BYTES {64} \
  ] $fifo_generator_1


  # Create instance: ila_0, and set properties
  set ila_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_0 ]
  set_property -dict [list \
    CONFIG.C_ENABLE_ILA_AXI_MON {true} \
    CONFIG.C_MONITOR_TYPE {AXI} \
    CONFIG.C_NUM_OF_PROBES {9} \
    CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} \
  ] $ila_0


  # Create instance: util_vector_logic_0, and set properties
  set util_vector_logic_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 util_vector_logic_0 ]
  set_property -dict [list \
    CONFIG.C_OPERATION {not} \
    CONFIG.C_SIZE {1} \
  ] $util_vector_logic_0


  # Create instance: xlconstant_1, and set properties
  set xlconstant_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_1 ]
  set_property CONFIG.CONST_VAL {0} $xlconstant_1


  # Create instance: fifo_generator_0, and set properties
  set fifo_generator_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:fifo_generator:13.2 fifo_generator_0 ]
  set_property -dict [list \
    CONFIG.Clock_Type_AXI {Independent_Clock} \
    CONFIG.Enable_TLAST {true} \
    CONFIG.FIFO_Application_Type_axis {Packet_FIFO} \
    CONFIG.FIFO_Implementation_axis {Independent_Clocks_Block_RAM} \
    CONFIG.FIFO_Implementation_rach {Independent_Clocks_Distributed_RAM} \
    CONFIG.FIFO_Implementation_rdch {Independent_Clocks_Builtin_FIFO} \
    CONFIG.FIFO_Implementation_wach {Independent_Clocks_Distributed_RAM} \
    CONFIG.FIFO_Implementation_wdch {Independent_Clocks_Builtin_FIFO} \
    CONFIG.FIFO_Implementation_wrch {Independent_Clocks_Distributed_RAM} \
    CONFIG.HAS_TKEEP {true} \
    CONFIG.INTERFACE_TYPE {AXI_STREAM} \
    CONFIG.Input_Depth_axis {32} \
    CONFIG.TDATA_NUM_BYTES {64} \
  ] $fifo_generator_0


  # Create instance: ila_1, and set properties
  set ila_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_1 ]
  set_property -dict [list \
    CONFIG.C_ENABLE_ILA_AXI_MON {true} \
    CONFIG.C_MONITOR_TYPE {AXI} \
    CONFIG.C_NUM_OF_PROBES {9} \
    CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} \
  ] $ila_1


  # Create instance: axi_clock_converter_3, and set properties
  set axi_clock_converter_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_clock_converter:2.1 axi_clock_converter_3 ]

  # Create instance: util_vector_logic_1, and set properties
  set util_vector_logic_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 util_vector_logic_1 ]
  set_property -dict [list \
    CONFIG.C_OPERATION {not} \
    CONFIG.C_SIZE {1} \
  ] $util_vector_logic_1


  # Create instance: cmac_usplus_1, and set properties
  set cmac_usplus_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:cmac_usplus:3.1 cmac_usplus_1 ]
  set_property -dict [list \
    CONFIG.DIFFCLK_BOARD_INTERFACE {qsfp1_156mhz} \
    CONFIG.ENABLE_AXI_INTERFACE {1} \
    CONFIG.ETHERNET_BOARD_INTERFACE {qsfp1_4x} \
    CONFIG.GT_DRP_CLK {100.00} \
    CONFIG.INCLUDE_AUTO_NEG_LT_LOGIC {0} \
    CONFIG.INCLUDE_RS_FEC {0} \
    CONFIG.INCLUDE_STATISTICS_COUNTERS {0} \
    CONFIG.INS_LOSS_NYQ {12} \
    CONFIG.NUM_LANES {4x25} \
    CONFIG.OPERATING_MODE {Duplex} \
    CONFIG.RX_FLOW_CONTROL {0} \
    CONFIG.TX_FLOW_CONTROL {0} \
    CONFIG.TX_OTN_INTERFACE {0} \
    CONFIG.USER_INTERFACE {AXIS} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $cmac_usplus_1


  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins axi_clock_converter_3/S_AXI] [get_bd_intf_pins S_AXI]
  connect_bd_intf_net -intf_net Conn2 [get_bd_intf_pins cmac_usplus_1/gt_serial_port] [get_bd_intf_pins qsfp1_4x]
  connect_bd_intf_net -intf_net Conn3 [get_bd_intf_pins cmac_usplus_1/gt_ref_clk] [get_bd_intf_pins qsfp1_156mhz]
  connect_bd_intf_net -intf_net Conn4 [get_bd_intf_pins fifo_generator_1/M_AXIS] [get_bd_intf_pins M_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets Conn4] [get_bd_intf_pins fifo_generator_1/M_AXIS] [get_bd_intf_pins ila_0/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net Conn5 [get_bd_intf_pins fifo_generator_0/S_AXIS] [get_bd_intf_pins S_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets Conn5] [get_bd_intf_pins fifo_generator_0/S_AXIS] [get_bd_intf_pins ila_1/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net axi_clock_converter_3_M_AXI [get_bd_intf_pins axi_clock_converter_3/M_AXI] [get_bd_intf_pins cmac_usplus_1/s_axi]
  connect_bd_intf_net -intf_net cmac_usplus_1_axis_rx [get_bd_intf_pins fifo_generator_1/S_AXIS] [get_bd_intf_pins cmac_usplus_1/axis_rx]
  connect_bd_intf_net -intf_net fifo_generator_0_M_AXIS [get_bd_intf_pins fifo_generator_0/M_AXIS] [get_bd_intf_pins cmac_usplus_1/axis_tx]

  # Create port connections
  connect_bd_net -net Net [get_bd_pins cmac_usplus_1/gt_rxusrclk2] [get_bd_pins fifo_generator_1/s_aclk] [get_bd_pins cmac_usplus_1/rx_clk]
  connect_bd_net -net cmac_usplus_1_gt_txusrclk2 [get_bd_pins cmac_usplus_1/gt_txusrclk2] [get_bd_pins fifo_generator_0/m_aclk]
  connect_bd_net -net cmac_usplus_1_usr_rx_reset [get_bd_pins cmac_usplus_1/usr_rx_reset] [get_bd_pins util_vector_logic_0/Op1]
  connect_bd_net -net m_aclk_1 [get_bd_pins m_aclk] [get_bd_pins fifo_generator_0/s_aclk] [get_bd_pins fifo_generator_1/m_aclk] [get_bd_pins ila_0/clk] [get_bd_pins ila_1/clk] [get_bd_pins cmac_usplus_1/init_clk]
  connect_bd_net -net m_axi_aclk_1 [get_bd_pins m_axi_aclk] [get_bd_pins axi_clock_converter_3/m_axi_aclk] [get_bd_pins cmac_usplus_1/s_axi_aclk]
  connect_bd_net -net m_axi_aresetn_1 [get_bd_pins m_axi_aresetn] [get_bd_pins fifo_generator_0/s_aresetn] [get_bd_pins axi_clock_converter_3/m_axi_aresetn] [get_bd_pins util_vector_logic_1/Op1]
  connect_bd_net -net s_axi_aclk_1 [get_bd_pins s_axi_aclk] [get_bd_pins axi_clock_converter_3/s_axi_aclk]
  connect_bd_net -net s_axi_aresetn_1 [get_bd_pins s_axi_aresetn] [get_bd_pins axi_clock_converter_3/s_axi_aresetn]
  connect_bd_net -net sys_reset_1 [get_bd_pins sys_reset] [get_bd_pins cmac_usplus_1/sys_reset]
  connect_bd_net -net util_vector_logic_0_Res [get_bd_pins util_vector_logic_0/Res] [get_bd_pins fifo_generator_1/s_aresetn]
  connect_bd_net -net util_vector_logic_1_Res [get_bd_pins util_vector_logic_1/Res] [get_bd_pins cmac_usplus_1/s_axi_sreset]
  connect_bd_net -net xlconstant_1_dout [get_bd_pins xlconstant_1/dout] [get_bd_pins cmac_usplus_1/gtwiz_reset_rx_datapath] [get_bd_pins cmac_usplus_1/core_rx_reset] [get_bd_pins cmac_usplus_1/core_tx_reset] [get_bd_pins cmac_usplus_1/core_drp_reset] [get_bd_pins cmac_usplus_1/drp_clk] [get_bd_pins cmac_usplus_1/pm_tick] [get_bd_pins cmac_usplus_1/gtwiz_reset_tx_datapath]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: packet
proc create_hier_cell_packet { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_packet() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 header_out_i

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 chunk_out_o

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 link_ingress

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:bram_rtl:1.0 recv_cbs_PORTA

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 dma_w_req_o

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:bram_rtl:1.0 send_cbs_PORTA

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 ram_cmd_o

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 ram_data_i

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 ram_status_i


  # Create pins
  create_bd_pin -dir I -type clk ap_clk
  create_bd_pin -dir I -type rst ap_rst_n

  # Create instance: pkt_ctor, and set properties
  set pkt_ctor [ create_bd_cell -type ip -vlnv xilinx.com:hls:pkt_ctor:1.0 pkt_ctor ]

  # Create instance: pkt_dtor, and set properties
  set pkt_dtor [ create_bd_cell -type ip -vlnv xilinx.com:hls:pkt_dtor:1.0 pkt_dtor ]

  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins pkt_dtor/link_ingress] [get_bd_intf_pins link_ingress]
  connect_bd_intf_net -intf_net Conn2 [get_bd_intf_pins pkt_ctor/send_cbs_PORTA] [get_bd_intf_pins send_cbs_PORTA]
  connect_bd_intf_net -intf_net header_out_i_1 [get_bd_intf_pins header_out_i] [get_bd_intf_pins pkt_ctor/header_out_i]
  connect_bd_intf_net -intf_net pkt_ctor_link_egress_o [get_bd_intf_pins chunk_out_o] [get_bd_intf_pins pkt_ctor/link_egress_o]
  connect_bd_intf_net -intf_net pkt_ctor_ram_cmd_o [get_bd_intf_pins ram_cmd_o] [get_bd_intf_pins pkt_ctor/ram_cmd_o]
  connect_bd_intf_net -intf_net pkt_dtor_0_dma_w_req_o [get_bd_intf_pins dma_w_req_o] [get_bd_intf_pins pkt_dtor/dma_w_req_o]
  connect_bd_intf_net -intf_net pkt_dtor_0_recv_cbs_PORTA [get_bd_intf_pins recv_cbs_PORTA] [get_bd_intf_pins pkt_dtor/recv_cbs_PORTA]
  connect_bd_intf_net -intf_net ram_data_i_1 [get_bd_intf_pins ram_data_i] [get_bd_intf_pins pkt_ctor/ram_data_i]
  connect_bd_intf_net -intf_net ram_status_i_1 [get_bd_intf_pins ram_status_i] [get_bd_intf_pins pkt_ctor/ram_status_i]

  # Create port connections
  connect_bd_net -net ap_clk_1 [get_bd_pins ap_clk] [get_bd_pins pkt_dtor/ap_clk] [get_bd_pins pkt_ctor/ap_clk]
  connect_bd_net -net ap_rst_n_1 [get_bd_pins ap_rst_n] [get_bd_pins pkt_dtor/ap_rst_n] [get_bd_pins pkt_ctor/ap_rst_n]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: control
proc create_hier_cell_control { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_control() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 new_sendmsg_o

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 new_fetch_o

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 sendmsg_o

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:bram_rtl:1.0 BRAM_PORTB

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:bram_rtl:1.0 BRAM_PORTA

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI


  # Create pins
  create_bd_pin -dir I -type rst ap_rst_n
  create_bd_pin -dir I -type clk ap_clk

  # Create instance: send_cbs, and set properties
  set send_cbs [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 send_cbs ]
  set_property -dict [list \
    CONFIG.Enable_32bit_Address {false} \
    CONFIG.Enable_B {Use_ENB_Pin} \
    CONFIG.Memory_Type {True_Dual_Port_RAM} \
    CONFIG.Port_B_Clock {100} \
    CONFIG.Port_B_Enable_Rate {100} \
    CONFIG.Port_B_Write_Rate {50} \
    CONFIG.Read_Width_A {512} \
    CONFIG.Read_Width_B {512} \
    CONFIG.Register_PortA_Output_of_Memory_Primitives {true} \
    CONFIG.Register_PortB_Output_of_Memory_Primitives {true} \
    CONFIG.Use_Byte_Write_Enable {false} \
    CONFIG.Use_RSTA_Pin {false} \
    CONFIG.Write_Width_A {512} \
    CONFIG.Write_Width_B {512} \
    CONFIG.use_bram_block {Stand_Alone} \
  ] $send_cbs


  # Create instance: recv_cbs, and set properties
  set recv_cbs [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 recv_cbs ]
  set_property -dict [list \
    CONFIG.Enable_32bit_Address {false} \
    CONFIG.Enable_B {Use_ENB_Pin} \
    CONFIG.Memory_Type {True_Dual_Port_RAM} \
    CONFIG.Port_B_Clock {100} \
    CONFIG.Port_B_Enable_Rate {100} \
    CONFIG.Port_B_Write_Rate {50} \
    CONFIG.Read_Width_A {512} \
    CONFIG.Read_Width_B {512} \
    CONFIG.Register_PortA_Output_of_Memory_Primitives {true} \
    CONFIG.Register_PortB_Output_of_Memory_Primitives {true} \
    CONFIG.Use_Byte_Write_Enable {false} \
    CONFIG.Use_RSTA_Pin {false} \
    CONFIG.Write_Width_A {512} \
    CONFIG.Write_Width_B {512} \
    CONFIG.use_bram_block {Stand_Alone} \
  ] $recv_cbs


  # Create instance: axi2axis_0, and set properties
  set block_name axi2axis
  set block_cell_name axi2axis_0
  if { [catch {set axi2axis_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $axi2axis_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
    set_property CONFIG.C_S_AXI_ID_WIDTH {8} $axi2axis_0


  set_property -dict [ list \
   CONFIG.FREQ_HZ {200000000} \
 ] [get_bd_intf_pins /control/axi2axis_0/M_AXIS]

  # Create instance: system_ila_4, and set properties
  set system_ila_4 [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 system_ila_4 ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $system_ila_4


  # Create instance: intf, and set properties
  set intf [ create_bd_cell -type ip -vlnv xilinx.com:hls:interface:1.0 intf ]

  # Create instance: user, and set properties
  set user [ create_bd_cell -type ip -vlnv xilinx.com:hls:user:1.0 user ]

  # Create interface connections
  connect_bd_intf_net -intf_net BRAM_PORTA_1 [get_bd_intf_pins BRAM_PORTA] [get_bd_intf_pins recv_cbs/BRAM_PORTA]
  connect_bd_intf_net -intf_net BRAM_PORTB_1 [get_bd_intf_pins BRAM_PORTB] [get_bd_intf_pins send_cbs/BRAM_PORTB]
  connect_bd_intf_net -intf_net Conn [get_bd_intf_pins axi2axis_0/S_AXIS] [get_bd_intf_pins system_ila_4/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net Conn2 [get_bd_intf_pins axi2axis_0/S_AXI] [get_bd_intf_pins S_AXI]
  connect_bd_intf_net -intf_net axi2axis_0_M_AXIS [get_bd_intf_pins intf/addr_in] [get_bd_intf_pins axi2axis_0/M_AXIS]
  connect_bd_intf_net -intf_net intf_recvmsg [get_bd_intf_pins intf/recvmsg] [get_bd_intf_pins user/recvmsg_i]
  connect_bd_intf_net -intf_net intf_sendmsg [get_bd_intf_pins intf/sendmsg] [get_bd_intf_pins user/sendmsg_i]
  connect_bd_intf_net -intf_net user_0_cbs_PORTA [get_bd_intf_pins user/cbs_PORTA] [get_bd_intf_pins send_cbs/BRAM_PORTA]
  connect_bd_intf_net -intf_net user_0_new_fetch_o [get_bd_intf_pins new_fetch_o] [get_bd_intf_pins user/new_fetch_o]
  connect_bd_intf_net -intf_net user_0_new_sendmsg_o [get_bd_intf_pins new_sendmsg_o] [get_bd_intf_pins user/new_sendmsg_o]
  connect_bd_intf_net -intf_net user_0_sendmsg_o [get_bd_intf_pins sendmsg_o] [get_bd_intf_pins user/sendmsg_o]

  # Create port connections
  connect_bd_net -net ap_clk_1 [get_bd_pins ap_clk] [get_bd_pins intf/ap_clk] [get_bd_pins axi2axis_0/S_AXI_ACLK] [get_bd_pins system_ila_4/clk] [get_bd_pins user/ap_clk]
  connect_bd_net -net ap_rst_n_1 [get_bd_pins ap_rst_n] [get_bd_pins intf/ap_rst_n] [get_bd_pins axi2axis_0/S_AXI_ARESETN] [get_bd_pins system_ila_4/resetn] [get_bd_pins user/ap_rst_n]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: pqs
proc create_hier_cell_pqs { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_pqs() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 S00_AXIS

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 M_AXIS

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 S02_AXIS

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 S_AXIS

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 M_AXIS1


  # Create pins
  create_bd_pin -dir I -type clk ap_clk
  create_bd_pin -dir I -type rst ap_rst

  # Create instance: axis_interconnect_0, and set properties
  set axis_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_interconnect:2.1 axis_interconnect_0 ]
  set_property -dict [list \
    CONFIG.NUM_MI {1} \
    CONFIG.NUM_SI {3} \
  ] $axis_interconnect_0


  # Create instance: fetch_in, and set properties
  set fetch_in [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 fetch_in ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $fetch_in


  # Create instance: fetch_out, and set properties
  set fetch_out [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 fetch_out ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $fetch_out


  # Create instance: sendmsg_in, and set properties
  set sendmsg_in [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 sendmsg_in ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $sendmsg_in


  # Create instance: sendmsg_out, and set properties
  set sendmsg_out [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 sendmsg_out ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $sendmsg_out


  # Create instance: sendmsg_queue, and set properties
  set block_name srpt_queue
  set block_cell_name sendmsg_queue
  if { [catch {set sendmsg_queue [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $sendmsg_queue eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  set_property -dict [ list \
   CONFIG.FREQ_HZ {200000000} \
 ] [get_bd_intf_pins /pqs/sendmsg_queue/M_AXIS]

  # Create instance: datafetch_queue, and set properties
  set block_name srpt_queue
  set block_cell_name datafetch_queue
  if { [catch {set datafetch_queue [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $datafetch_queue eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
    set_property CONFIG.TYPE {fetch} $datafetch_queue


  set_property -dict [ list \
   CONFIG.FREQ_HZ {200000000} \
 ] [get_bd_intf_pins /pqs/datafetch_queue/M_AXIS]

  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins axis_interconnect_0/S00_AXIS] [get_bd_intf_pins S00_AXIS]
  connect_bd_intf_net -intf_net S02_AXIS_1 [get_bd_intf_pins S02_AXIS] [get_bd_intf_pins axis_interconnect_0/S02_AXIS]
  connect_bd_intf_net -intf_net S_AXIS_1 [get_bd_intf_pins S_AXIS] [get_bd_intf_pins datafetch_queue/S_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets S_AXIS_1] [get_bd_intf_pins S_AXIS] [get_bd_intf_pins fetch_in/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net axis_interconnect_0_M00_AXIS [get_bd_intf_pins sendmsg_queue/S_AXIS] [get_bd_intf_pins axis_interconnect_0/M00_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets axis_interconnect_0_M00_AXIS] [get_bd_intf_pins sendmsg_queue/S_AXIS] [get_bd_intf_pins sendmsg_in/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net datafetch_queue_M_AXIS [get_bd_intf_pins M_AXIS] [get_bd_intf_pins datafetch_queue/M_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets datafetch_queue_M_AXIS] [get_bd_intf_pins M_AXIS] [get_bd_intf_pins fetch_out/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net sendmsg_queue_M_AXIS [get_bd_intf_pins M_AXIS1] [get_bd_intf_pins sendmsg_queue/M_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets sendmsg_queue_M_AXIS] [get_bd_intf_pins M_AXIS1] [get_bd_intf_pins sendmsg_out/SLOT_0_AXIS]

  # Create port connections
  connect_bd_net -net Net [get_bd_pins ap_clk] [get_bd_pins axis_interconnect_0/ACLK] [get_bd_pins axis_interconnect_0/S00_AXIS_ACLK] [get_bd_pins axis_interconnect_0/M00_AXIS_ACLK] [get_bd_pins axis_interconnect_0/S01_AXIS_ACLK] [get_bd_pins axis_interconnect_0/S02_AXIS_ACLK] [get_bd_pins fetch_out/clk] [get_bd_pins sendmsg_in/clk] [get_bd_pins sendmsg_out/clk] [get_bd_pins fetch_in/clk] [get_bd_pins sendmsg_queue/ap_clk] [get_bd_pins datafetch_queue/ap_clk]
  connect_bd_net -net ap_rst_1 [get_bd_pins ap_rst] [get_bd_pins axis_interconnect_0/ARESETN] [get_bd_pins axis_interconnect_0/S00_AXIS_ARESETN] [get_bd_pins axis_interconnect_0/M00_AXIS_ARESETN] [get_bd_pins axis_interconnect_0/S01_AXIS_ARESETN] [get_bd_pins axis_interconnect_0/S02_AXIS_ARESETN] [get_bd_pins fetch_out/resetn] [get_bd_pins sendmsg_in/resetn] [get_bd_pins sendmsg_out/resetn] [get_bd_pins fetch_in/resetn] [get_bd_pins sendmsg_queue/ap_rst_n] [get_bd_pins datafetch_queue/ap_rst_n]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: dma
proc create_hier_cell_dma { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_dma() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dma_r_req_i

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dma_w_sendmsg_i

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dma_w_data_i

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M01_AXI

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 dbuff_notif_o

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 S_AXIS

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 M_AXIS

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 M_AXIS1


  # Create pins
  create_bd_pin -dir O -from 0 -to 0 -type rst axi_aresetn
  create_bd_pin -dir O -type clk axi_aclk
  create_bd_pin -dir I -type clk ap_clk
  create_bd_pin -dir I -type rst ap_rst_n
  create_bd_pin -dir I pcie_reset_n
  create_bd_pin -dir O -from 15 -to 0 pcie_tx_p
  create_bd_pin -dir I pcie_refclk_n
  create_bd_pin -dir I pcie_refclk_p
  create_bd_pin -dir I -from 15 -to 0 pcie_rx_n
  create_bd_pin -dir I -from 15 -to 0 pcie_rx_p
  create_bd_pin -dir O -from 15 -to 0 pcie_tx_n

  # Create instance: metadata_maps_ctrl, and set properties
  set metadata_maps_ctrl [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 metadata_maps_ctrl ]
  set_property -dict [list \
    CONFIG.DATA_WIDTH {64} \
    CONFIG.SINGLE_PORT_BRAM {1} \
  ] $metadata_maps_ctrl


  # Create instance: metadata_maps, and set properties
  set metadata_maps [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 metadata_maps ]
  set_property -dict [list \
    CONFIG.Enable_B {Use_ENB_Pin} \
    CONFIG.Memory_Type {True_Dual_Port_RAM} \
    CONFIG.Port_B_Clock {100} \
    CONFIG.Port_B_Enable_Rate {100} \
    CONFIG.Port_B_Write_Rate {50} \
    CONFIG.Use_RSTB_Pin {true} \
  ] $metadata_maps


  # Create instance: h2c_data_maps_ctrl, and set properties
  set h2c_data_maps_ctrl [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 h2c_data_maps_ctrl ]
  set_property -dict [list \
    CONFIG.DATA_WIDTH {64} \
    CONFIG.SINGLE_PORT_BRAM {1} \
  ] $h2c_data_maps_ctrl


  # Create instance: h2c_data_maps, and set properties
  set h2c_data_maps [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 h2c_data_maps ]
  set_property -dict [list \
    CONFIG.Enable_B {Use_ENB_Pin} \
    CONFIG.Memory_Type {True_Dual_Port_RAM} \
    CONFIG.Port_B_Clock {100} \
    CONFIG.Port_B_Enable_Rate {100} \
    CONFIG.Port_B_Write_Rate {50} \
    CONFIG.Use_RSTB_Pin {true} \
  ] $h2c_data_maps


  # Create instance: c2h_data_maps_ctrl, and set properties
  set c2h_data_maps_ctrl [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 c2h_data_maps_ctrl ]
  set_property -dict [list \
    CONFIG.DATA_WIDTH {64} \
    CONFIG.SINGLE_PORT_BRAM {1} \
  ] $c2h_data_maps_ctrl


  # Create instance: c2h_data_maps, and set properties
  set c2h_data_maps [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 c2h_data_maps ]
  set_property -dict [list \
    CONFIG.Enable_B {Use_ENB_Pin} \
    CONFIG.Memory_Type {True_Dual_Port_RAM} \
    CONFIG.Port_B_Clock {100} \
    CONFIG.Port_B_Enable_Rate {100} \
    CONFIG.Port_B_Write_Rate {50} \
    CONFIG.Use_RSTB_Pin {true} \
  ] $c2h_data_maps


  # Create instance: axi_interconnect_4, and set properties
  set axi_interconnect_4 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_4 ]
  set_property CONFIG.NUM_MI {3} $axi_interconnect_4


  # Create instance: axi_interconnect_1, and set properties
  set axi_interconnect_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_1 ]
  set_property CONFIG.NUM_MI {2} $axi_interconnect_1


  # Create instance: dma_w_req, and set properties
  set dma_w_req [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 dma_w_req ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $dma_w_req


  # Create instance: c2h, and set properties
  set c2h [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 c2h ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_MAX_RD_BURSTS {64} \
    CONFIG.C_SLOT_0_MAX_WR_BURSTS {64} \
  ] $c2h


  # Create instance: addr_map, and set properties
  set addr_map [ create_bd_cell -type ip -vlnv xilinx.com:hls:addr_map:1.0 addr_map ]

  # Create instance: c2h_dma, and set properties
  set c2h_dma [ create_bd_cell -type ip -vlnv xilinx.com:hls:c2h_dma:1.0 c2h_dma ]

  # Create instance: cmd_queue_write, and set properties
  set cmd_queue_write [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 cmd_queue_write ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $cmd_queue_write


  # Create instance: dma_r_req, and set properties
  set dma_r_req [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 dma_r_req ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $dma_r_req


  # Create instance: h2c_dma, and set properties
  set h2c_dma [ create_bd_cell -type ip -vlnv xilinx.com:hls:h2c_dma:1.0 h2c_dma ]

  # Create instance: cmd_queue_read, and set properties
  set cmd_queue_read [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 cmd_queue_read ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $cmd_queue_read


  # Create instance: blk_mem_gen_0, and set properties
  set blk_mem_gen_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 blk_mem_gen_0 ]

  # Create instance: axi_bram_ctrl_0, and set properties
  set axi_bram_ctrl_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_0 ]
  set_property -dict [list \
    CONFIG.DATA_WIDTH {512} \
    CONFIG.SINGLE_PORT_BRAM {1} \
  ] $axi_bram_ctrl_0


  # Create instance: axi_clock_converter_0, and set properties
  set axi_clock_converter_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_clock_converter:2.1 axi_clock_converter_0 ]

  # Create instance: axi_clock_converter_1, and set properties
  set axi_clock_converter_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_clock_converter:2.1 axi_clock_converter_1 ]

  # Create instance: axis_clock_converter_1, and set properties
  set axis_clock_converter_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_1 ]

  # Create instance: h2c, and set properties
  set h2c [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 h2c ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_MAX_RD_BURSTS {64} \
    CONFIG.C_SLOT_0_MAX_WR_BURSTS {64} \
  ] $h2c


  # Create instance: util_vector_logic_0, and set properties
  set util_vector_logic_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 util_vector_logic_0 ]
  set_property -dict [list \
    CONFIG.C_OPERATION {not} \
    CONFIG.C_SIZE {1} \
  ] $util_vector_logic_0


  # Create instance: h2c_buffer, and set properties
  set block_name dma_psdpram
  set block_cell_name h2c_buffer
  if { [catch {set h2c_buffer [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $h2c_buffer eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
    set_property -dict [list \
    CONFIG.SEG_ADDR_WIDTH {11} \
    CONFIG.SEG_BE_WIDTH {64} \
    CONFIG.SEG_COUNT {2} \
    CONFIG.SEG_DATA_WIDTH {512} \
    CONFIG.SIZE {262144} \
  ] $h2c_buffer


  # Create instance: pcie_0, and set properties
  set block_name pcie
  set block_cell_name pcie_0
  if { [catch {set pcie_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $pcie_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
    set_property -dict [list \
    CONFIG.AXIS_PCIE_DATA_WIDTH {512} \
    CONFIG.AXIS_PCIE_KEEP_WIDTH {16} \
    CONFIG.AXI_ADDR_WIDTH {26} \
    CONFIG.CC_STRADDLE {true} \
    CONFIG.CHECK_BUS_NUMBER {0} \
    CONFIG.CQ_STRADDLE {true} \
    CONFIG.RAM_ADDR_WIDTH {18} \
    CONFIG.RAM_SEG_ADDR_WIDTH {11} \
    CONFIG.RAM_SEG_BE_WIDTH {64} \
    CONFIG.RAM_SEG_DATA_WIDTH {512} \
    CONFIG.RC_STRADDLE {true} \
    CONFIG.RQ_STRADDLE {true} \
    CONFIG.TLP_DATA_WIDTH {512} \
    CONFIG.TLP_FORCE_64_BIT_ADDR {1} \
    CONFIG.TLP_SEG_COUNT {1} \
  ] $pcie_0


  set_property -dict [ list \
   CONFIG.FREQ_HZ {250000000} \
 ] [get_bd_intf_pins /dma/pcie_0/m_axi]

  set_property -dict [ list \
   CONFIG.FREQ_HZ {250000000} \
 ] [get_bd_intf_pins /dma/pcie_0/pcie_dma_read_desc_status]

  set_property -dict [ list \
   CONFIG.FREQ_HZ {250000000} \
 ] [get_bd_intf_pins /dma/pcie_0/pcie_dma_write_desc_status]

  set_property -dict [ list \
   CONFIG.FREQ_HZ {250000000} \
 ] [get_bd_pins /dma/pcie_0/pcie_user_clk]

  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_HIGH} \
 ] [get_bd_pins /dma/pcie_0/pcie_user_reset]

  # Create instance: dma_client_axis_sour_0, and set properties
  set block_name dma_client_axis_source
  set block_cell_name dma_client_axis_sour_0
  if { [catch {set dma_client_axis_sour_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $dma_client_axis_sour_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
    set_property -dict [list \
    CONFIG.AXIS_DATA_WIDTH {512} \
    CONFIG.AXIS_KEEP_ENABLE {true} \
    CONFIG.AXIS_KEEP_WIDTH {64} \
    CONFIG.LEN_WIDTH {64} \
    CONFIG.RAM_ADDR_WIDTH {18} \
    CONFIG.SEG_ADDR_WIDTH {11} \
    CONFIG.SEG_BE_WIDTH {64} \
    CONFIG.SEG_DATA_WIDTH {512} \
  ] $dma_client_axis_sour_0


  set_property -dict [ list \
   CONFIG.FREQ_HZ {250000000} \
 ] [get_bd_intf_pins /dma/dma_client_axis_sour_0/m_axis_read_data]

  set_property -dict [ list \
   CONFIG.FREQ_HZ {250000000} \
 ] [get_bd_intf_pins /dma/dma_client_axis_sour_0/m_axis_read_desc_status]

  set_property -dict [ list \
   CONFIG.FREQ_HZ {250000000} \
 ] [get_bd_pins /dma/dma_client_axis_sour_0/clk]

  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_HIGH} \
 ] [get_bd_pins /dma/dma_client_axis_sour_0/rst]

  # Create instance: xlconstant_1, and set properties
  set xlconstant_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_1 ]

  # Create instance: read_status, and set properties
  set read_status [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 read_status ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $read_status


  # Create instance: axis_clock_converter_2, and set properties
  set axis_clock_converter_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_2 ]
  set_property CONFIG.TDATA_NUM_BYTES {2} $axis_clock_converter_2


  # Create instance: dbuff_notif_o, and set properties
  set dbuff_notif_o [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 dbuff_notif_o ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $dbuff_notif_o


  # Create instance: axis_clock_converter_3, and set properties
  set axis_clock_converter_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_3 ]

  # Create instance: axis_clock_converter_4, and set properties
  set axis_clock_converter_4 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_4 ]

  # Create instance: axis_clock_converter_5, and set properties
  set axis_clock_converter_5 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_5 ]
  set_property CONFIG.TDATA_NUM_BYTES {2} $axis_clock_converter_5


  # Create instance: client_read_status, and set properties
  set client_read_status [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 client_read_status ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $client_read_status


  # Create instance: client_read_desc, and set properties
  set client_read_desc [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 client_read_desc ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $client_read_desc


  # Create instance: client_read_data, and set properties
  set client_read_data [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 client_read_data ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $client_read_data


  # Create instance: client_read, and set properties
  set client_read [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 client_read ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {NATIVE} \
    CONFIG.C_NUM_OF_PROBES {6} \
  ] $client_read


  # Create instance: dma_client_axis_sink_0, and set properties
  set block_name dma_client_axis_sink
  set block_cell_name dma_client_axis_sink_0
  if { [catch {set dma_client_axis_sink_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $dma_client_axis_sink_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
    set_property -dict [list \
    CONFIG.AXIS_DATA_WIDTH {512} \
    CONFIG.AXIS_KEEP_WIDTH {64} \
    CONFIG.LEN_WIDTH {64} \
    CONFIG.RAM_ADDR_WIDTH {18} \
    CONFIG.SEG_ADDR_WIDTH {11} \
    CONFIG.SEG_BE_WIDTH {64} \
    CONFIG.SEG_DATA_WIDTH {512} \
  ] $dma_client_axis_sink_0


  # Create instance: c2h_buffer, and set properties
  set block_name dma_psdpram
  set block_cell_name c2h_buffer
  if { [catch {set c2h_buffer [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $c2h_buffer eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
    set_property -dict [list \
    CONFIG.SEG_ADDR_WIDTH {11} \
    CONFIG.SEG_BE_WIDTH {64} \
    CONFIG.SEG_COUNT {2} \
    CONFIG.SEG_DATA_WIDTH {512} \
    CONFIG.SIZE {262144} \
  ] $c2h_buffer


  # Create instance: axis_clock_converter_6, and set properties
  set axis_clock_converter_6 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_6 ]

  # Create instance: axis_clock_converter_7, and set properties
  set axis_clock_converter_7 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_7 ]

  # Create instance: axis_clock_converter_8, and set properties
  set axis_clock_converter_8 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_8 ]

  # Create instance: ram_cmd_o, and set properties
  set ram_cmd_o [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 ram_cmd_o ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $ram_cmd_o


  # Create instance: ram_status_i, and set properties
  set ram_status_i [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 ram_status_i ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $ram_status_i


  # Create instance: ram_data_o, and set properties
  set ram_data_o [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 ram_data_o ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $ram_data_o


  # Create instance: axis_clock_converter_9, and set properties
  set axis_clock_converter_9 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_9 ]

  # Create instance: write_status, and set properties
  set write_status [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 write_status ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {INTERFACE} \
    CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} \
  ] $write_status


  # Create instance: xlconstant_0, and set properties
  set xlconstant_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_0 ]

  # Create instance: axis_clock_converter_10, and set properties
  set axis_clock_converter_10 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_10 ]

  # Create instance: ram_wr, and set properties
  set ram_wr [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 ram_wr ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {NATIVE} \
    CONFIG.C_NUM_OF_PROBES {6} \
  ] $ram_wr


  # Create instance: pcie_wr, and set properties
  set pcie_wr [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 pcie_wr ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {NATIVE} \
    CONFIG.C_NUM_OF_PROBES {6} \
  ] $pcie_wr


  # Create instance: internal, and set properties
  set internal [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 internal ]
  set_property -dict [list \
    CONFIG.C_MON_TYPE {NATIVE} \
    CONFIG.C_NUM_OF_PROBES {50} \
  ] $internal


  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins h2c_dma/dbuff_notif_o] [get_bd_intf_pins dbuff_notif_o]
  connect_bd_intf_net -intf_net [get_bd_intf_nets Conn1] [get_bd_intf_pins h2c_dma/dbuff_notif_o] [get_bd_intf_pins dbuff_notif_o/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net Conn2 [get_bd_intf_pins axi_interconnect_1/M01_AXI] [get_bd_intf_pins M01_AXI]
  connect_bd_intf_net -intf_net S_AXIS_1 [get_bd_intf_pins S_AXIS] [get_bd_intf_pins axis_clock_converter_3/S_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets S_AXIS_1] [get_bd_intf_pins S_AXIS] [get_bd_intf_pins client_read_desc/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net S_AXI_1 [get_bd_intf_pins S_AXI] [get_bd_intf_pins axi_interconnect_4/S00_AXI]
  connect_bd_intf_net -intf_net addr_map_c2h_data_map_PORTA [get_bd_intf_pins addr_map/c2h_data_map_PORTA] [get_bd_intf_pins c2h_data_maps/BRAM_PORTB]
  connect_bd_intf_net -intf_net addr_map_dma_w_req_o [get_bd_intf_pins c2h_dma/dma_w_req_i] [get_bd_intf_pins addr_map/dma_w_req_o]
  connect_bd_intf_net -intf_net [get_bd_intf_nets addr_map_dma_w_req_o] [get_bd_intf_pins c2h_dma/dma_w_req_i] [get_bd_intf_pins dma_w_req/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net addr_map_h2c_data_map_PORTA [get_bd_intf_pins addr_map/h2c_data_map_PORTA] [get_bd_intf_pins h2c_data_maps/BRAM_PORTB]
  connect_bd_intf_net -intf_net addr_map_metadata_map_PORTA [get_bd_intf_pins addr_map/metadata_map_PORTA] [get_bd_intf_pins metadata_maps/BRAM_PORTB]
  connect_bd_intf_net -intf_net axi_bram_ctrl_0_BRAM_PORTA [get_bd_intf_pins axi_bram_ctrl_0/BRAM_PORTA] [get_bd_intf_pins blk_mem_gen_0/BRAM_PORTA]
  connect_bd_intf_net -intf_net axi_clock_converter_0_M_AXI [get_bd_intf_pins M_AXI] [get_bd_intf_pins axi_clock_converter_0/M_AXI]
  connect_bd_intf_net -intf_net [get_bd_intf_nets axi_clock_converter_0_M_AXI] [get_bd_intf_pins M_AXI] [get_bd_intf_pins h2c/SLOT_0_AXI]
  connect_bd_intf_net -intf_net axi_clock_converter_1_M_AXI [get_bd_intf_pins axi_clock_converter_1/M_AXI] [get_bd_intf_pins axi_bram_ctrl_0/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_1_M00_AXI [get_bd_intf_pins axi_interconnect_1/M00_AXI] [get_bd_intf_pins axi_clock_converter_0/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_4_M00_AXI [get_bd_intf_pins metadata_maps_ctrl/S_AXI] [get_bd_intf_pins axi_interconnect_4/M00_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_4_M01_AXI [get_bd_intf_pins c2h_data_maps_ctrl/S_AXI] [get_bd_intf_pins axi_interconnect_4/M01_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_4_M02_AXI [get_bd_intf_pins axi_interconnect_4/M02_AXI] [get_bd_intf_pins h2c_data_maps_ctrl/S_AXI]
  connect_bd_intf_net -intf_net axis_clock_converter_10_M_AXIS [get_bd_intf_pins axis_clock_converter_10/M_AXIS] [get_bd_intf_pins pcie_0/pcie_dma_write_desc]
  connect_bd_intf_net -intf_net axis_clock_converter_1_M_AXIS [get_bd_intf_pins addr_map/dma_r_req_o] [get_bd_intf_pins h2c_dma/dma_r_req_i]
  connect_bd_intf_net -intf_net [get_bd_intf_nets axis_clock_converter_1_M_AXIS] [get_bd_intf_pins addr_map/dma_r_req_o] [get_bd_intf_pins dma_r_req/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net axis_clock_converter_1_M_AXIS1 [get_bd_intf_pins axis_clock_converter_1/M_AXIS] [get_bd_intf_pins pcie_0/pcie_dma_read_desc]
  connect_bd_intf_net -intf_net axis_clock_converter_2_M_AXIS [get_bd_intf_pins axis_clock_converter_2/M_AXIS] [get_bd_intf_pins h2c_dma/status_queue_i]
  connect_bd_intf_net -intf_net [get_bd_intf_nets axis_clock_converter_2_M_AXIS] [get_bd_intf_pins axis_clock_converter_2/M_AXIS] [get_bd_intf_pins read_status/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net axis_clock_converter_3_M_AXIS [get_bd_intf_pins axis_clock_converter_3/M_AXIS] [get_bd_intf_pins dma_client_axis_sour_0/s_axis_read_desc]
  connect_bd_intf_net -intf_net axis_clock_converter_4_M_AXIS [get_bd_intf_pins M_AXIS] [get_bd_intf_pins axis_clock_converter_4/M_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets axis_clock_converter_4_M_AXIS] [get_bd_intf_pins M_AXIS] [get_bd_intf_pins client_read_data/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net axis_clock_converter_5_M_AXIS [get_bd_intf_pins M_AXIS1] [get_bd_intf_pins axis_clock_converter_5/M_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets axis_clock_converter_5_M_AXIS] [get_bd_intf_pins M_AXIS1] [get_bd_intf_pins client_read_status/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net axis_clock_converter_6_M_AXIS [get_bd_intf_pins axis_clock_converter_6/M_AXIS] [get_bd_intf_pins dma_client_axis_sink_0/s_axis_write_data]
  connect_bd_intf_net -intf_net axis_clock_converter_7_M_AXIS [get_bd_intf_pins axis_clock_converter_7/M_AXIS] [get_bd_intf_pins dma_client_axis_sink_0/s_axis_write_desc]
  connect_bd_intf_net -intf_net axis_clock_converter_8_M_AXIS [get_bd_intf_pins axis_clock_converter_8/M_AXIS] [get_bd_intf_pins c2h_dma/ram_status_i]
  connect_bd_intf_net -intf_net [get_bd_intf_nets axis_clock_converter_8_M_AXIS] [get_bd_intf_pins axis_clock_converter_8/M_AXIS] [get_bd_intf_pins ram_status_i/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net axis_clock_converter_9_M_AXIS [get_bd_intf_pins axis_clock_converter_9/M_AXIS] [get_bd_intf_pins write_status/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net c2h_data_maps_ctrl_BRAM_PORTA [get_bd_intf_pins c2h_data_maps_ctrl/BRAM_PORTA] [get_bd_intf_pins c2h_data_maps/BRAM_PORTA]
  connect_bd_intf_net -intf_net c2h_dma_pcie_cmd_o [get_bd_intf_pins c2h_dma/pcie_cmd_o] [get_bd_intf_pins axis_clock_converter_10/S_AXIS]
  connect_bd_intf_net -intf_net c2h_dma_ram_cmd_o [get_bd_intf_pins c2h_dma/ram_cmd_o] [get_bd_intf_pins axis_clock_converter_7/S_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets c2h_dma_ram_cmd_o] [get_bd_intf_pins c2h_dma/ram_cmd_o] [get_bd_intf_pins ram_cmd_o/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net c2h_dma_ram_data_o [get_bd_intf_pins c2h_dma/ram_data_o] [get_bd_intf_pins axis_clock_converter_6/S_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets c2h_dma_ram_data_o] [get_bd_intf_pins c2h_dma/ram_data_o] [get_bd_intf_pins ram_data_o/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net dma_client_axis_sink_0_m_axis_write_desc_status [get_bd_intf_pins dma_client_axis_sink_0/m_axis_write_desc_status] [get_bd_intf_pins axis_clock_converter_8/S_AXIS]
  connect_bd_intf_net -intf_net dma_client_axis_sour_0_m_axis_read_data [get_bd_intf_pins axis_clock_converter_4/S_AXIS] [get_bd_intf_pins dma_client_axis_sour_0/m_axis_read_data]
  connect_bd_intf_net -intf_net dma_client_axis_sour_0_m_axis_read_desc_status [get_bd_intf_pins dma_client_axis_sour_0/m_axis_read_desc_status] [get_bd_intf_pins axis_clock_converter_5/S_AXIS]
  connect_bd_intf_net -intf_net dma_r_req_i_1 [get_bd_intf_pins dma_r_req_i] [get_bd_intf_pins addr_map/dma_r_req_i]
  connect_bd_intf_net -intf_net dma_w_data_i_1 [get_bd_intf_pins dma_w_data_i] [get_bd_intf_pins addr_map/dma_w_data_i]
  connect_bd_intf_net -intf_net dma_w_sendmsg_i_1 [get_bd_intf_pins dma_w_sendmsg_i] [get_bd_intf_pins addr_map/dma_w_sendmsg_i]
  connect_bd_intf_net -intf_net h2c_data_maps_ctrl_BRAM_PORTA [get_bd_intf_pins h2c_data_maps_ctrl/BRAM_PORTA] [get_bd_intf_pins h2c_data_maps/BRAM_PORTA]
  connect_bd_intf_net -intf_net h2c_dma_cmd_queue_o [get_bd_intf_pins h2c_dma/cmd_queue_o] [get_bd_intf_pins axis_clock_converter_1/S_AXIS]
  connect_bd_intf_net -intf_net [get_bd_intf_nets h2c_dma_cmd_queue_o] [get_bd_intf_pins h2c_dma/cmd_queue_o] [get_bd_intf_pins cmd_queue_read/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net hostsharedmemctrl_BRAM_PORTA [get_bd_intf_pins metadata_maps_ctrl/BRAM_PORTA] [get_bd_intf_pins metadata_maps/BRAM_PORTA]
  connect_bd_intf_net -intf_net pcie_0_pcie_dma_read_desc_status [get_bd_intf_pins pcie_0/pcie_dma_read_desc_status] [get_bd_intf_pins axis_clock_converter_2/S_AXIS]
  connect_bd_intf_net -intf_net pcie_0_pcie_dma_write_desc_status [get_bd_intf_pins axis_clock_converter_9/S_AXIS] [get_bd_intf_pins pcie_0/pcie_dma_write_desc_status]
  connect_bd_intf_net -intf_net pcie_m_axi_h2c [get_bd_intf_pins pcie_0/m_axi] [get_bd_intf_pins axi_interconnect_1/S00_AXI]
  connect_bd_intf_net -intf_net pcie_upgraded_ipi_m_axi_c2h [get_bd_intf_pins axi_clock_converter_1/S_AXI] [get_bd_intf_pins c2h/SLOT_0_AXI]

  # Create port connections
  connect_bd_net -net ap_clk_1 [get_bd_pins ap_clk] [get_bd_pins metadata_maps_ctrl/s_axi_aclk] [get_bd_pins axi_interconnect_4/ACLK] [get_bd_pins axi_interconnect_4/S00_ACLK] [get_bd_pins axi_interconnect_4/M00_ACLK] [get_bd_pins axi_interconnect_4/M01_ACLK] [get_bd_pins axi_interconnect_4/M02_ACLK] [get_bd_pins dma_r_req/clk] [get_bd_pins c2h_data_maps_ctrl/s_axi_aclk] [get_bd_pins h2c_data_maps_ctrl/s_axi_aclk] [get_bd_pins cmd_queue_read/clk] [get_bd_pins axi_clock_converter_0/m_axi_aclk] [get_bd_pins axi_bram_ctrl_0/s_axi_aclk] [get_bd_pins axi_clock_converter_1/m_axi_aclk] [get_bd_pins axis_clock_converter_1/s_axis_aclk] [get_bd_pins h2c/clk] [get_bd_pins read_status/clk] [get_bd_pins axis_clock_converter_2/m_axis_aclk] [get_bd_pins dbuff_notif_o/clk] [get_bd_pins axis_clock_converter_5/m_axis_aclk] [get_bd_pins axis_clock_converter_3/s_axis_aclk] [get_bd_pins axis_clock_converter_4/m_axis_aclk] [get_bd_pins client_read_data/clk] [get_bd_pins client_read_status/clk] [get_bd_pins client_read_desc/clk] [get_bd_pins axis_clock_converter_7/s_axis_aclk] [get_bd_pins axis_clock_converter_6/s_axis_aclk] [get_bd_pins ram_cmd_o/clk] [get_bd_pins ram_data_o/clk] [get_bd_pins ram_status_i/clk] [get_bd_pins axis_clock_converter_9/m_axis_aclk] [get_bd_pins write_status/clk] [get_bd_pins dma_w_req/clk] [get_bd_pins axis_clock_converter_10/s_axis_aclk] [get_bd_pins axis_clock_converter_8/m_axis_aclk] [get_bd_pins c2h_dma/ap_clk] [get_bd_pins h2c_dma/ap_clk] [get_bd_pins addr_map/ap_clk]
  connect_bd_net -net ap_rst_n_1 [get_bd_pins ap_rst_n] [get_bd_pins metadata_maps_ctrl/s_axi_aresetn] [get_bd_pins axi_interconnect_4/ARESETN] [get_bd_pins axi_interconnect_4/S00_ARESETN] [get_bd_pins axi_interconnect_4/M00_ARESETN] [get_bd_pins axi_interconnect_4/M01_ARESETN] [get_bd_pins axi_interconnect_4/M02_ARESETN] [get_bd_pins dma_r_req/resetn] [get_bd_pins c2h_data_maps_ctrl/s_axi_aresetn] [get_bd_pins h2c_data_maps_ctrl/s_axi_aresetn] [get_bd_pins cmd_queue_read/resetn] [get_bd_pins axi_clock_converter_0/m_axi_aresetn] [get_bd_pins axi_bram_ctrl_0/s_axi_aresetn] [get_bd_pins axi_clock_converter_1/m_axi_aresetn] [get_bd_pins axis_clock_converter_1/s_axis_aresetn] [get_bd_pins h2c/resetn] [get_bd_pins read_status/resetn] [get_bd_pins axis_clock_converter_2/m_axis_aresetn] [get_bd_pins dbuff_notif_o/resetn] [get_bd_pins axis_clock_converter_5/m_axis_aresetn] [get_bd_pins axis_clock_converter_4/m_axis_aresetn] [get_bd_pins client_read_data/resetn] [get_bd_pins client_read_status/resetn] [get_bd_pins axis_clock_converter_3/s_axis_aresetn] [get_bd_pins client_read_desc/resetn] [get_bd_pins axis_clock_converter_7/s_axis_aresetn] [get_bd_pins axis_clock_converter_6/s_axis_aresetn] [get_bd_pins axis_clock_converter_8/m_axis_aresetn] [get_bd_pins ram_cmd_o/resetn] [get_bd_pins ram_data_o/resetn] [get_bd_pins ram_status_i/resetn] [get_bd_pins axis_clock_converter_9/m_axis_aresetn] [get_bd_pins write_status/resetn] [get_bd_pins dma_w_req/resetn] [get_bd_pins axis_clock_converter_10/s_axis_aresetn] [get_bd_pins c2h_dma/ap_rst_n] [get_bd_pins h2c_dma/ap_rst_n] [get_bd_pins addr_map/ap_rst_n]
  connect_bd_net -net c2h_buffer_rd_cmd_ready [get_bd_pins c2h_buffer/rd_cmd_ready] [get_bd_pins pcie_0/ram_rd_cmd_ready]
  connect_bd_net -net c2h_buffer_rd_resp_data [get_bd_pins c2h_buffer/rd_resp_data] [get_bd_pins pcie_0/ram_rd_resp_data]
  connect_bd_net -net c2h_buffer_rd_resp_valid [get_bd_pins c2h_buffer/rd_resp_valid] [get_bd_pins pcie_0/ram_rd_resp_valid]
  connect_bd_net -net c2h_buffer_wr_cmd_ready [get_bd_pins c2h_buffer/wr_cmd_ready] [get_bd_pins ram_wr/probe4] [get_bd_pins dma_client_axis_sink_0/ram_wr_cmd_ready]
  connect_bd_net -net c2h_buffer_wr_done [get_bd_pins c2h_buffer/wr_done] [get_bd_pins ram_wr/probe5] [get_bd_pins dma_client_axis_sink_0/ram_wr_done]
  connect_bd_net -net dma_client_axis_sink_0_ram_wr_cmd_addr [get_bd_pins dma_client_axis_sink_0/ram_wr_cmd_addr] [get_bd_pins c2h_buffer/wr_cmd_addr] [get_bd_pins ram_wr/probe1]
  connect_bd_net -net dma_client_axis_sink_0_ram_wr_cmd_be [get_bd_pins dma_client_axis_sink_0/ram_wr_cmd_be] [get_bd_pins c2h_buffer/wr_cmd_be] [get_bd_pins ram_wr/probe0]
  connect_bd_net -net dma_client_axis_sink_0_ram_wr_cmd_data [get_bd_pins dma_client_axis_sink_0/ram_wr_cmd_data] [get_bd_pins c2h_buffer/wr_cmd_data] [get_bd_pins ram_wr/probe2]
  connect_bd_net -net dma_client_axis_sink_0_ram_wr_cmd_valid [get_bd_pins dma_client_axis_sink_0/ram_wr_cmd_valid] [get_bd_pins c2h_buffer/wr_cmd_valid] [get_bd_pins ram_wr/probe3]
  connect_bd_net -net dma_client_axis_sour_0_ram_rd_cmd_addr [get_bd_pins dma_client_axis_sour_0/ram_rd_cmd_addr] [get_bd_pins h2c_buffer/rd_cmd_addr] [get_bd_pins client_read/probe0]
  connect_bd_net -net dma_client_axis_sour_0_ram_rd_cmd_valid [get_bd_pins dma_client_axis_sour_0/ram_rd_cmd_valid] [get_bd_pins h2c_buffer/rd_cmd_valid] [get_bd_pins client_read/probe4]
  connect_bd_net -net dma_client_axis_sour_0_ram_rd_resp_ready [get_bd_pins dma_client_axis_sour_0/ram_rd_resp_ready] [get_bd_pins h2c_buffer/rd_resp_ready] [get_bd_pins client_read/probe3]
  connect_bd_net -net dma_psdpram_0_rd_cmd_ready [get_bd_pins h2c_buffer/rd_cmd_ready] [get_bd_pins dma_client_axis_sour_0/ram_rd_cmd_ready]
  connect_bd_net -net dma_psdpram_0_rd_resp_data [get_bd_pins h2c_buffer/rd_resp_data] [get_bd_pins client_read/probe1] [get_bd_pins dma_client_axis_sour_0/ram_rd_resp_data]
  connect_bd_net -net dma_psdpram_0_rd_resp_valid [get_bd_pins h2c_buffer/rd_resp_valid] [get_bd_pins client_read/probe2] [get_bd_pins dma_client_axis_sour_0/ram_rd_resp_valid]
  connect_bd_net -net dma_psdpram_0_wr_cmd_ready [get_bd_pins h2c_buffer/wr_cmd_ready] [get_bd_pins pcie_wr/probe3] [get_bd_pins pcie_0/ram_wr_cmd_ready]
  connect_bd_net -net dma_psdpram_0_wr_done [get_bd_pins h2c_buffer/wr_done] [get_bd_pins pcie_wr/probe4] [get_bd_pins client_read/probe5] [get_bd_pins pcie_0/ram_wr_done]
  connect_bd_net -net pcie_0_pcie_tx_n [get_bd_pins pcie_0/pcie_tx_n] [get_bd_pins pcie_tx_n]
  connect_bd_net -net pcie_0_pcie_tx_p [get_bd_pins pcie_0/pcie_tx_p] [get_bd_pins pcie_tx_p]
  connect_bd_net -net pcie_0_pcie_user_reset [get_bd_pins pcie_0/pcie_user_reset] [get_bd_pins util_vector_logic_0/Op1] [get_bd_pins h2c_buffer/rst] [get_bd_pins c2h_buffer/rst] [get_bd_pins dma_client_axis_sink_0/rst] [get_bd_pins dma_client_axis_sour_0/rst]
  connect_bd_net -net pcie_0_ram_rd_cmd_addr [get_bd_pins pcie_0/ram_rd_cmd_addr] [get_bd_pins c2h_buffer/rd_cmd_addr]
  connect_bd_net -net pcie_0_ram_rd_cmd_valid [get_bd_pins pcie_0/ram_rd_cmd_valid] [get_bd_pins c2h_buffer/rd_cmd_valid]
  connect_bd_net -net pcie_0_ram_rd_resp_ready [get_bd_pins pcie_0/ram_rd_resp_ready] [get_bd_pins c2h_buffer/rd_resp_ready]
  connect_bd_net -net pcie_0_rx_cpl_tlp_data [get_bd_pins pcie_0/rx_cpl_tlp_data] [get_bd_pins internal/probe0]
  connect_bd_net -net pcie_0_rx_cpl_tlp_eop [get_bd_pins pcie_0/rx_cpl_tlp_eop] [get_bd_pins internal/probe6]
  connect_bd_net -net pcie_0_rx_cpl_tlp_error [get_bd_pins pcie_0/rx_cpl_tlp_error] [get_bd_pins internal/probe3]
  connect_bd_net -net pcie_0_rx_cpl_tlp_hdr [get_bd_pins pcie_0/rx_cpl_tlp_hdr] [get_bd_pins internal/probe2]
  connect_bd_net -net pcie_0_rx_cpl_tlp_ready [get_bd_pins pcie_0/rx_cpl_tlp_ready] [get_bd_pins internal/probe7]
  connect_bd_net -net pcie_0_rx_cpl_tlp_sop [get_bd_pins pcie_0/rx_cpl_tlp_sop] [get_bd_pins internal/probe5]
  connect_bd_net -net pcie_0_rx_cpl_tlp_strb [get_bd_pins pcie_0/rx_cpl_tlp_strb] [get_bd_pins internal/probe1]
  connect_bd_net -net pcie_0_rx_cpl_tlp_valid [get_bd_pins pcie_0/rx_cpl_tlp_valid] [get_bd_pins internal/probe4]
  connect_bd_net -net pcie_0_rx_req_tlp_bar_id [get_bd_pins pcie_0/rx_req_tlp_bar_id] [get_bd_pins internal/probe11]
  connect_bd_net -net pcie_0_rx_req_tlp_data [get_bd_pins pcie_0/rx_req_tlp_data] [get_bd_pins internal/probe8]
  connect_bd_net -net pcie_0_rx_req_tlp_eop [get_bd_pins pcie_0/rx_req_tlp_eop] [get_bd_pins internal/probe15]
  connect_bd_net -net pcie_0_rx_req_tlp_func_num [get_bd_pins pcie_0/rx_req_tlp_func_num] [get_bd_pins internal/probe12]
  connect_bd_net -net pcie_0_rx_req_tlp_hdr [get_bd_pins pcie_0/rx_req_tlp_hdr] [get_bd_pins internal/probe10]
  connect_bd_net -net pcie_0_rx_req_tlp_sop [get_bd_pins pcie_0/rx_req_tlp_sop] [get_bd_pins internal/probe14]
  connect_bd_net -net pcie_0_rx_req_tlp_strb [get_bd_pins pcie_0/rx_req_tlp_strb] [get_bd_pins internal/probe9]
  connect_bd_net -net pcie_0_rx_req_tlp_valid [get_bd_pins pcie_0/rx_req_tlp_valid] [get_bd_pins internal/probe13]
  connect_bd_net -net pcie_0_tx_cpl_tlp_data [get_bd_pins pcie_0/tx_cpl_tlp_data] [get_bd_pins internal/probe30]
  connect_bd_net -net pcie_0_tx_cpl_tlp_eop [get_bd_pins pcie_0/tx_cpl_tlp_eop] [get_bd_pins internal/probe35]
  connect_bd_net -net pcie_0_tx_cpl_tlp_hdr [get_bd_pins pcie_0/tx_cpl_tlp_hdr] [get_bd_pins internal/probe32]
  connect_bd_net -net pcie_0_tx_cpl_tlp_ready [get_bd_pins pcie_0/tx_cpl_tlp_ready] [get_bd_pins internal/probe36]
  connect_bd_net -net pcie_0_tx_cpl_tlp_sop [get_bd_pins pcie_0/tx_cpl_tlp_sop] [get_bd_pins internal/probe34]
  connect_bd_net -net pcie_0_tx_cpl_tlp_strb [get_bd_pins pcie_0/tx_cpl_tlp_strb] [get_bd_pins internal/probe31]
  connect_bd_net -net pcie_0_tx_cpl_tlp_valid [get_bd_pins pcie_0/tx_cpl_tlp_valid] [get_bd_pins internal/probe33]
  connect_bd_net -net pcie_0_tx_rd_req_tlp_eop [get_bd_pins pcie_0/tx_rd_req_tlp_eop] [get_bd_pins internal/probe20]
  connect_bd_net -net pcie_0_tx_rd_req_tlp_hdr [get_bd_pins pcie_0/tx_rd_req_tlp_hdr] [get_bd_pins internal/probe16]
  connect_bd_net -net pcie_0_tx_rd_req_tlp_ready [get_bd_pins pcie_0/tx_rd_req_tlp_ready] [get_bd_pins internal/probe21]
  connect_bd_net -net pcie_0_tx_rd_req_tlp_seq [get_bd_pins pcie_0/tx_rd_req_tlp_seq] [get_bd_pins internal/probe17]
  connect_bd_net -net pcie_0_tx_rd_req_tlp_sop [get_bd_pins pcie_0/tx_rd_req_tlp_sop] [get_bd_pins internal/probe19]
  connect_bd_net -net pcie_0_tx_rd_req_tlp_valid [get_bd_pins pcie_0/tx_rd_req_tlp_valid] [get_bd_pins internal/probe18]
  connect_bd_net -net pcie_0_tx_wr_req_tlp_data [get_bd_pins pcie_0/tx_wr_req_tlp_data] [get_bd_pins internal/probe22]
  connect_bd_net -net pcie_0_tx_wr_req_tlp_eop [get_bd_pins pcie_0/tx_wr_req_tlp_eop] [get_bd_pins internal/probe28]
  connect_bd_net -net pcie_0_tx_wr_req_tlp_hdr [get_bd_pins pcie_0/tx_wr_req_tlp_hdr] [get_bd_pins internal/probe24]
  connect_bd_net -net pcie_0_tx_wr_req_tlp_ready [get_bd_pins pcie_0/tx_wr_req_tlp_ready] [get_bd_pins internal/probe29]
  connect_bd_net -net pcie_0_tx_wr_req_tlp_seq [get_bd_pins pcie_0/tx_wr_req_tlp_seq] [get_bd_pins internal/probe25]
  connect_bd_net -net pcie_0_tx_wr_req_tlp_sop [get_bd_pins pcie_0/tx_wr_req_tlp_sop] [get_bd_pins internal/probe27]
  connect_bd_net -net pcie_0_tx_wr_req_tlp_strb [get_bd_pins pcie_0/tx_wr_req_tlp_strb] [get_bd_pins internal/probe23]
  connect_bd_net -net pcie_0_tx_wr_req_tlp_valid [get_bd_pins pcie_0/tx_wr_req_tlp_valid] [get_bd_pins internal/probe26]
  connect_bd_net -net pcie_ram_wr_cmd_addr [get_bd_pins pcie_0/ram_wr_cmd_addr] [get_bd_pins h2c_buffer/wr_cmd_addr] [get_bd_pins pcie_wr/probe1]
  connect_bd_net -net pcie_ram_wr_cmd_be [get_bd_pins pcie_0/ram_wr_cmd_be] [get_bd_pins h2c_buffer/wr_cmd_be] [get_bd_pins pcie_wr/probe0]
  connect_bd_net -net pcie_ram_wr_cmd_data [get_bd_pins pcie_0/ram_wr_cmd_data] [get_bd_pins h2c_buffer/wr_cmd_data] [get_bd_pins pcie_wr/probe2]
  connect_bd_net -net pcie_ram_wr_cmd_valid [get_bd_pins pcie_0/ram_wr_cmd_valid] [get_bd_pins h2c_buffer/wr_cmd_valid] [get_bd_pins pcie_wr/probe5]
  connect_bd_net -net pcie_refclk_n_0_1 [get_bd_pins pcie_refclk_n] [get_bd_pins pcie_0/pcie_refclk_n]
  connect_bd_net -net pcie_refclk_p_0_1 [get_bd_pins pcie_refclk_p] [get_bd_pins pcie_0/pcie_refclk_p]
  connect_bd_net -net pcie_reset_n_0_1 [get_bd_pins pcie_reset_n] [get_bd_pins pcie_0/pcie_reset_n]
  connect_bd_net -net pcie_rx_n_0_1 [get_bd_pins pcie_rx_n] [get_bd_pins pcie_0/pcie_rx_n]
  connect_bd_net -net pcie_rx_p_0_1 [get_bd_pins pcie_rx_p] [get_bd_pins pcie_0/pcie_rx_p]
  connect_bd_net -net pcie_sys_clk_clk_out1 [get_bd_pins pcie_0/pcie_user_clk] [get_bd_pins axi_aclk] [get_bd_pins axi_interconnect_1/ACLK] [get_bd_pins axi_interconnect_1/S00_ACLK] [get_bd_pins axi_interconnect_1/M00_ACLK] [get_bd_pins axi_interconnect_1/M01_ACLK] [get_bd_pins cmd_queue_write/clk] [get_bd_pins axi_clock_converter_0/s_axi_aclk] [get_bd_pins axi_clock_converter_1/s_axi_aclk] [get_bd_pins axis_clock_converter_1/m_axis_aclk] [get_bd_pins c2h/clk] [get_bd_pins axis_clock_converter_2/s_axis_aclk] [get_bd_pins h2c_buffer/clk] [get_bd_pins axis_clock_converter_5/s_axis_aclk] [get_bd_pins axis_clock_converter_3/m_axis_aclk] [get_bd_pins axis_clock_converter_4/s_axis_aclk] [get_bd_pins client_read/clk] [get_bd_pins c2h_buffer/clk] [get_bd_pins axis_clock_converter_7/m_axis_aclk] [get_bd_pins axis_clock_converter_6/m_axis_aclk] [get_bd_pins axis_clock_converter_8/s_axis_aclk] [get_bd_pins axis_clock_converter_9/s_axis_aclk] [get_bd_pins axis_clock_converter_10/m_axis_aclk] [get_bd_pins ram_wr/clk] [get_bd_pins pcie_wr/clk] [get_bd_pins dma_client_axis_sink_0/clk] [get_bd_pins dma_client_axis_sour_0/clk] [get_bd_pins internal/clk]
  connect_bd_net -net xdma_0_axi_aresetn [get_bd_pins util_vector_logic_0/Res] [get_bd_pins axi_aresetn] [get_bd_pins axi_interconnect_1/ARESETN] [get_bd_pins axi_interconnect_1/S00_ARESETN] [get_bd_pins axi_interconnect_1/M00_ARESETN] [get_bd_pins axi_interconnect_1/M01_ARESETN] [get_bd_pins cmd_queue_write/resetn] [get_bd_pins axi_clock_converter_0/s_axi_aresetn] [get_bd_pins axi_clock_converter_1/s_axi_aresetn] [get_bd_pins axis_clock_converter_1/m_axis_aresetn] [get_bd_pins c2h/resetn] [get_bd_pins axis_clock_converter_2/s_axis_aresetn] [get_bd_pins axis_clock_converter_5/s_axis_aresetn] [get_bd_pins axis_clock_converter_3/m_axis_aresetn] [get_bd_pins axis_clock_converter_4/s_axis_aresetn] [get_bd_pins axis_clock_converter_7/m_axis_aresetn] [get_bd_pins axis_clock_converter_6/m_axis_aresetn] [get_bd_pins axis_clock_converter_8/s_axis_aresetn] [get_bd_pins axis_clock_converter_9/s_axis_aresetn] [get_bd_pins axis_clock_converter_10/m_axis_aresetn]
  connect_bd_net -net xlconstant_0_dout [get_bd_pins xlconstant_0/dout] [get_bd_pins axis_clock_converter_9/m_axis_tready]
  connect_bd_net -net xlconstant_1_dout [get_bd_pins xlconstant_1/dout] [get_bd_pins dma_client_axis_sink_0/enable] [get_bd_pins dma_client_axis_sour_0/enable]

  # Restore current instance
  current_bd_instance $oldCurInst
}


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
  set qsfp1_4x [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gt_rtl:1.0 qsfp1_4x ]

  set default_300mhz_clk0 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 default_300mhz_clk0 ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {300000000} \
   ] $default_300mhz_clk0

  set qsfp1_156mhz [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 qsfp1_156mhz ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {156250000} \
   ] $qsfp1_156mhz


  # Create ports
  set resetn [ create_bd_port -dir I -type rst resetn ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_LOW} \
 ] $resetn
  set pcie_reset_n [ create_bd_port -dir I pcie_reset_n ]
  set pcie_tx_p [ create_bd_port -dir O -from 15 -to 0 pcie_tx_p ]
  set pcie_refclk_n [ create_bd_port -dir I pcie_refclk_n ]
  set pcie_refclk_p [ create_bd_port -dir I pcie_refclk_p ]
  set pcie_rx_n [ create_bd_port -dir I -from 15 -to 0 pcie_rx_n ]
  set pcie_rx_p [ create_bd_port -dir I -from 15 -to 0 pcie_rx_p ]
  set pcie_tx_n [ create_bd_port -dir O -from 15 -to 0 pcie_tx_n ]

  # Create instance: proc_sys_reset_0, and set properties
  set proc_sys_reset_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_0 ]

  # Create instance: axi_interconnect_0, and set properties
  set axi_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_0 ]
  set_property CONFIG.NUM_MI {5} $axi_interconnect_0


  # Create instance: dma
  create_hier_cell_dma [current_bd_instance .] dma

  # Create instance: pqs
  create_hier_cell_pqs [current_bd_instance .] pqs

  # Create instance: control
  create_hier_cell_control [current_bd_instance .] control

  # Create instance: packet
  create_hier_cell_packet [current_bd_instance .] packet

  # Create instance: link
  create_hier_cell_link [current_bd_instance .] link

  # Create instance: softproc
  create_hier_cell_softproc [current_bd_instance .] softproc

  # Create instance: mainClk, and set properties
  set mainClk [ create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:6.0 mainClk ]
  set_property -dict [list \
    CONFIG.CLKIN1_JITTER_PS {33.330000000000005} \
    CONFIG.CLKOUT1_JITTER {88.577} \
    CONFIG.CLKOUT1_PHASE_ERROR {77.836} \
    CONFIG.CLKOUT1_REQUESTED_OUT_FREQ {200.000} \
    CONFIG.CLKOUT2_JITTER {101.475} \
    CONFIG.CLKOUT2_PHASE_ERROR {77.836} \
    CONFIG.CLKOUT2_USED {true} \
    CONFIG.CLK_IN1_BOARD_INTERFACE {default_300mhz_clk0} \
    CONFIG.MMCM_CLKFBOUT_MULT_F {4.000} \
    CONFIG.MMCM_CLKIN1_PERIOD {3.333} \
    CONFIG.MMCM_CLKIN2_PERIOD {10.0} \
    CONFIG.MMCM_CLKOUT0_DIVIDE_F {6.000} \
    CONFIG.MMCM_CLKOUT1_DIVIDE {12} \
    CONFIG.NUM_OUT_CLKS {2} \
    CONFIG.PRIM_SOURCE {Differential_clock_capable_pin} \
    CONFIG.RESET_BOARD_INTERFACE {resetn} \
    CONFIG.RESET_PORT {resetn} \
    CONFIG.RESET_TYPE {ACTIVE_LOW} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $mainClk


  # Create interface connections
  connect_bd_intf_net -intf_net BRAM_PORTB_1 [get_bd_intf_pins control/BRAM_PORTB] [get_bd_intf_pins packet/send_cbs_PORTA]
  connect_bd_intf_net -intf_net S00_AXIS_1 [get_bd_intf_pins pqs/S00_AXIS] [get_bd_intf_pins dma/dbuff_notif_o]
  connect_bd_intf_net -intf_net S00_AXI_1 [get_bd_intf_pins axi_interconnect_0/S00_AXI] [get_bd_intf_pins dma/M_AXI]
  connect_bd_intf_net -intf_net S_AXI_1 [get_bd_intf_pins link/S_AXI] [get_bd_intf_pins dma/M01_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M00_AXI [get_bd_intf_pins axi_interconnect_0/M00_AXI] [get_bd_intf_pins dma/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M02_AXI [get_bd_intf_pins axi_interconnect_0/M02_AXI] [get_bd_intf_pins control/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M03_AXI [get_bd_intf_pins axi_interconnect_0/M03_AXI] [get_bd_intf_pins softproc/S_AXI1]
  connect_bd_intf_net -intf_net axi_interconnect_0_M04_AXI [get_bd_intf_pins axi_interconnect_0/M04_AXI] [get_bd_intf_pins softproc/S_AXI]
  connect_bd_intf_net -intf_net control_new_fetch_o [get_bd_intf_pins control/new_fetch_o] [get_bd_intf_pins pqs/S_AXIS]
  connect_bd_intf_net -intf_net control_new_sendmsg_o [get_bd_intf_pins control/new_sendmsg_o] [get_bd_intf_pins pqs/S02_AXIS]
  connect_bd_intf_net -intf_net control_sendmsg_o [get_bd_intf_pins control/sendmsg_o] [get_bd_intf_pins dma/dma_w_sendmsg_i]
  connect_bd_intf_net -intf_net default_300mhz_clk0_1 [get_bd_intf_ports default_300mhz_clk0] [get_bd_intf_pins mainClk/CLK_IN1_D]
  connect_bd_intf_net -intf_net dma_M_AXIS [get_bd_intf_pins dma/M_AXIS] [get_bd_intf_pins packet/ram_data_i]
  connect_bd_intf_net -intf_net fifo_generator_1_M_AXIS [get_bd_intf_pins link/M_AXIS] [get_bd_intf_pins packet/link_ingress]
  connect_bd_intf_net -intf_net homa_link_egress_o [get_bd_intf_pins link/S_AXIS] [get_bd_intf_pins packet/chunk_out_o]
  connect_bd_intf_net -intf_net link_qsfp1_4x [get_bd_intf_ports qsfp1_4x] [get_bd_intf_pins link/qsfp1_4x]
  connect_bd_intf_net -intf_net packet_dma_w_req_o [get_bd_intf_pins packet/dma_w_req_o] [get_bd_intf_pins dma/dma_w_data_i]
  connect_bd_intf_net -intf_net packet_ram_cmd_o [get_bd_intf_pins packet/ram_cmd_o] [get_bd_intf_pins dma/S_AXIS]
  connect_bd_intf_net -intf_net packet_recv_cbs_PORTA [get_bd_intf_pins packet/recv_cbs_PORTA] [get_bd_intf_pins control/BRAM_PORTA]
  connect_bd_intf_net -intf_net pqs_M_AXIS [get_bd_intf_pins pqs/M_AXIS] [get_bd_intf_pins dma/dma_r_req_i]
  connect_bd_intf_net -intf_net pqs_M_AXIS1 [get_bd_intf_pins pqs/M_AXIS1] [get_bd_intf_pins packet/header_out_i]
  connect_bd_intf_net -intf_net qsfp1_156mhz_1 [get_bd_intf_ports qsfp1_156mhz] [get_bd_intf_pins link/qsfp1_156mhz]
  connect_bd_intf_net -intf_net ram_status_i_1 [get_bd_intf_pins packet/ram_status_i] [get_bd_intf_pins dma/M_AXIS1]

  # Create port connections
  connect_bd_net -net dma_pcie_tx_n_0 [get_bd_pins dma/pcie_tx_n] [get_bd_ports pcie_tx_n]
  connect_bd_net -net dma_pcie_tx_p_0 [get_bd_pins dma/pcie_tx_p] [get_bd_ports pcie_tx_p]
  connect_bd_net -net mainClk_clk_out2 [get_bd_pins mainClk/clk_out2] [get_bd_pins softproc/slowest_sync_clk] [get_bd_pins proc_sys_reset_0/slowest_sync_clk] [get_bd_pins link/m_axi_aclk]
  connect_bd_net -net mainClk_locked [get_bd_pins mainClk/locked] [get_bd_pins softproc/dcm_locked] [get_bd_pins proc_sys_reset_0/dcm_locked]
  connect_bd_net -net microblaze_0_Clk [get_bd_pins mainClk/clk_out1] [get_bd_pins control/ap_clk] [get_bd_pins packet/ap_clk] [get_bd_pins dma/ap_clk] [get_bd_pins pqs/ap_clk] [get_bd_pins link/m_aclk] [get_bd_pins softproc/s_axi_aclk] [get_bd_pins axi_interconnect_0/ACLK] [get_bd_pins axi_interconnect_0/S00_ACLK] [get_bd_pins axi_interconnect_0/M00_ACLK] [get_bd_pins axi_interconnect_0/M01_ACLK] [get_bd_pins axi_interconnect_0/M02_ACLK] [get_bd_pins axi_interconnect_0/M03_ACLK] [get_bd_pins axi_interconnect_0/M04_ACLK]
  connect_bd_net -net pcie_refclk_n_0_1 [get_bd_ports pcie_refclk_n] [get_bd_pins dma/pcie_refclk_n]
  connect_bd_net -net pcie_refclk_p_0_1 [get_bd_ports pcie_refclk_p] [get_bd_pins dma/pcie_refclk_p]
  connect_bd_net -net pcie_reset_n_0_1 [get_bd_ports pcie_reset_n] [get_bd_pins dma/pcie_reset_n]
  connect_bd_net -net pcie_rx_n_0_1 [get_bd_ports pcie_rx_n] [get_bd_pins dma/pcie_rx_n]
  connect_bd_net -net pcie_rx_p_0_1 [get_bd_ports pcie_rx_p] [get_bd_pins dma/pcie_rx_p]
  connect_bd_net -net proc_sys_reset_0_peripheral_aresetn [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins control/ap_rst_n] [get_bd_pins packet/ap_rst_n] [get_bd_pins dma/ap_rst_n] [get_bd_pins pqs/ap_rst] [get_bd_pins link/m_axi_aresetn] [get_bd_pins softproc/s_axi_aresetn] [get_bd_pins axi_interconnect_0/ARESETN] [get_bd_pins axi_interconnect_0/S00_ARESETN] [get_bd_pins axi_interconnect_0/M00_ARESETN] [get_bd_pins axi_interconnect_0/M01_ARESETN] [get_bd_pins axi_interconnect_0/M02_ARESETN] [get_bd_pins axi_interconnect_0/M03_ARESETN] [get_bd_pins axi_interconnect_0/M04_ARESETN]
  connect_bd_net -net proc_sys_reset_0_peripheral_reset [get_bd_pins proc_sys_reset_0/peripheral_reset] [get_bd_pins link/sys_reset]
  connect_bd_net -net resetn_1 [get_bd_ports resetn] [get_bd_pins mainClk/resetn] [get_bd_pins softproc/resetn] [get_bd_pins proc_sys_reset_0/ext_reset_in]
  connect_bd_net -net xdma_0_axi_aclk [get_bd_pins dma/axi_aclk] [get_bd_pins link/s_axi_aclk]
  connect_bd_net -net xdma_0_axi_aresetn [get_bd_pins dma/axi_aresetn] [get_bd_pins link/s_axi_aresetn]

  # Create address segments
  assign_bd_address -offset 0x00000000 -range 0x00010000 -target_address_space [get_bd_addr_spaces dma/pcie_0/m_axi] [get_bd_addr_segs control/axi2axis_0/S_AXI/reg0] -force
  assign_bd_address -offset 0x00070000 -range 0x00010000 -target_address_space [get_bd_addr_spaces dma/pcie_0/m_axi] [get_bd_addr_segs dma/c2h_data_maps_ctrl/S_AXI/Mem0] -force
  assign_bd_address -offset 0x00030000 -range 0x00010000 -target_address_space [get_bd_addr_spaces dma/pcie_0/m_axi] [get_bd_addr_segs link/cmac_usplus_1/s_axi/Reg] -force
  assign_bd_address -offset 0x00080000 -range 0x00010000 -target_address_space [get_bd_addr_spaces dma/pcie_0/m_axi] [get_bd_addr_segs dma/h2c_data_maps_ctrl/S_AXI/Mem0] -force
  assign_bd_address -offset 0x00060000 -range 0x00010000 -target_address_space [get_bd_addr_spaces dma/pcie_0/m_axi] [get_bd_addr_segs dma/metadata_maps_ctrl/S_AXI/Mem0] -force


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


