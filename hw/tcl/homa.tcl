
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


# The design that will be created by this Tcl script contains the following 
# module references:
# picorv32_axi, axi2axis, srpt_queue, srpt_queue

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
xilinx.com:ip:axi_clock_converter:2.1\
xilinx.com:ip:clk_wiz:6.0\
xilinx.com:ip:proc_sys_reset:5.0\
xilinx.com:ip:fifo_generator:13.2\
xilinx.com:ip:util_vector_logic:2.0\
xilinx.com:ip:ila:6.2\
xilinx.com:ip:xlconstant:1.1\
xilinx.com:ip:cmac_usplus:3.1\
xilinx.com:ip:axi_gpio:2.0\
xilinx.com:ip:xlslice:1.0\
xilinx.com:ip:blk_mem_gen:8.4\
xilinx.com:ip:axi_bram_ctrl:4.1\
xilinx.com:ip:system_ila:1.1\
xilinx.com:hls:homa:1.0\
xilinx.com:hls:dma_write:1.0\
xilinx.com:ip:axi_datamover:5.1\
xilinx.com:hls:dma_read:1.0\
xilinx.com:ip:xdma:4.1\
xilinx.com:ip:util_ds_buf:2.2\
xilinx.com:hls:cache_ctrl:1.0\
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

##################################################################
# CHECK Modules
##################################################################
set bCheckModules 1
if { $bCheckModules == 1 } {
   set list_check_mods "\ 
picorv32_axi\
axi2axis\
srpt_queue\
srpt_queue\
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
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 addr_in


  # Create pins
  create_bd_pin -dir I -type rst ap_rst_n
  create_bd_pin -dir I -type clk ap_clk

  # Create instance: intf, and set properties
  set intf [ create_bd_cell -type ip -vlnv xilinx.com:hls:interface:1.0 intf ]

  # Create interface connections
  connect_bd_intf_net -intf_net Conn2 [get_bd_intf_pins intf/addr_in] [get_bd_intf_pins addr_in]

  # Create port connections
  connect_bd_net -net ap_clk_1 [get_bd_pins ap_clk] [get_bd_pins intf/ap_clk]
  connect_bd_net -net ap_rst_n_1 [get_bd_pins ap_rst_n] [get_bd_pins intf/ap_rst_n]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: cache
proc create_hier_cell_cache { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_cache() - Empty argument(s)!"}
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
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 dbuff_notif_o

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 new_entry_i


  # Create pins
  create_bd_pin -dir I -type clk ap_clk
  create_bd_pin -dir I -type rst ap_rst_n

  # Create instance: data, and set properties
  set data [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 data ]
  set_property -dict [list \
    CONFIG.Enable_32bit_Address {true} \
    CONFIG.Memory_Type {True_Dual_Port_RAM} \
    CONFIG.Write_Width_A {512} \
    CONFIG.Write_Width_B {1024} \
    CONFIG.use_bram_block {Stand_Alone} \
  ] $data


  # Create instance: cache_ctrl_0, and set properties
  set cache_ctrl_0 [ create_bd_cell -type ip -vlnv xilinx.com:hls:cache_ctrl:1.0 cache_ctrl_0 ]

  # Create interface connections
  connect_bd_intf_net -intf_net cache_ctrl_0_cache_PORTA [get_bd_intf_pins cache_ctrl_0/cache_PORTA] [get_bd_intf_pins data/BRAM_PORTA]
  connect_bd_intf_net -intf_net cache_ctrl_0_dbuff_notif_o [get_bd_intf_pins dbuff_notif_o] [get_bd_intf_pins cache_ctrl_0/dbuff_notif_o]
  connect_bd_intf_net -intf_net new_entry_i_1 [get_bd_intf_pins new_entry_i] [get_bd_intf_pins cache_ctrl_0/new_entry_i]

  # Create port connections
  connect_bd_net -net ap_clk_1 [get_bd_pins ap_clk] [get_bd_pins cache_ctrl_0/ap_clk]
  connect_bd_net -net ap_rst_n_1 [get_bd_pins ap_rst_n] [get_bd_pins cache_ctrl_0/ap_rst_n]

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


  # Create pins
  create_bd_pin -dir I -type clk ap_clk
  create_bd_pin -dir I -type rst ap_rst

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
  
  # Create instance: axis_interconnect_0, and set properties
  set axis_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_interconnect:2.1 axis_interconnect_0 ]
  set_property -dict [list \
    CONFIG.NUM_MI {1} \
    CONFIG.NUM_SI {3} \
  ] $axis_interconnect_0


  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins axis_interconnect_0/S00_AXIS] [get_bd_intf_pins S00_AXIS]
  connect_bd_intf_net -intf_net axis_interconnect_0_M00_AXIS [get_bd_intf_pins sendmsg_queue/S_AXIS] [get_bd_intf_pins axis_interconnect_0/M00_AXIS]

  # Create port connections
  connect_bd_net -net Net [get_bd_pins ap_clk] [get_bd_pins sendmsg_queue/ap_clk] [get_bd_pins datafetch_queue/ap_clk] [get_bd_pins axis_interconnect_0/ACLK] [get_bd_pins axis_interconnect_0/S00_AXIS_ACLK] [get_bd_pins axis_interconnect_0/M00_AXIS_ACLK] [get_bd_pins axis_interconnect_0/S01_AXIS_ACLK] [get_bd_pins axis_interconnect_0/S02_AXIS_ACLK]
  connect_bd_net -net ap_rst_1 [get_bd_pins ap_rst] [get_bd_pins sendmsg_queue/ap_rst] [get_bd_pins datafetch_queue/ap_rst] [get_bd_pins axis_interconnect_0/ARESETN] [get_bd_pins axis_interconnect_0/S00_AXIS_ARESETN] [get_bd_pins axis_interconnect_0/M00_AXIS_ARESETN] [get_bd_pins axis_interconnect_0/S01_AXIS_ARESETN] [get_bd_pins axis_interconnect_0/S02_AXIS_ARESETN]

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
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI_B

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:pcie_7x_mgt_rtl:1.0 pci_express_x16

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_B

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 pcie_refclk

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 dma_r_req_o

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI


  # Create pins
  create_bd_pin -dir I -type rst m_axis_s2mm_cmdsts_aresetn
  create_bd_pin -dir I -type clk m_axis_mm2s_cmdsts_aclk
  create_bd_pin -dir O -type rst axi_aresetn
  create_bd_pin -dir O -type clk axi_aclk
  create_bd_pin -dir I -type rst pcie_perstn
  create_bd_pin -dir I -type clk ap_clk
  create_bd_pin -dir I -type rst ap_rst_n

  # Create instance: dma_write_0, and set properties
  set dma_write_0 [ create_bd_cell -type ip -vlnv xilinx.com:hls:dma_write:1.0 dma_write_0 ]

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


  # Create instance: dma_read_0, and set properties
  set dma_read_0 [ create_bd_cell -type ip -vlnv xilinx.com:hls:dma_read:1.0 dma_read_0 ]

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


  # Create instance: util_ds_buf, and set properties
  set util_ds_buf [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_ds_buf:2.2 util_ds_buf ]
  set_property -dict [list \
    CONFIG.DIFF_CLK_IN_BOARD_INTERFACE {pcie_refclk} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $util_ds_buf


  # Create instance: xlconstant_0, and set properties
  set xlconstant_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_0 ]
  set_property CONFIG.CONST_VAL {0} $xlconstant_0


  # Create instance: meteadata_maps_ctrl, and set properties
  set meteadata_maps_ctrl [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 meteadata_maps_ctrl ]
  set_property -dict [list \
    CONFIG.DATA_WIDTH {128} \
    CONFIG.SINGLE_PORT_BRAM {1} \
  ] $meteadata_maps_ctrl


  # Create instance: metadata_maps, and set properties
  set metadata_maps [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 metadata_maps ]
  set_property CONFIG.Memory_Type {True_Dual_Port_RAM} $metadata_maps


  # Create instance: h2c_data_maps_ctrl, and set properties
  set h2c_data_maps_ctrl [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 h2c_data_maps_ctrl ]
  set_property -dict [list \
    CONFIG.DATA_WIDTH {128} \
    CONFIG.SINGLE_PORT_BRAM {1} \
  ] $h2c_data_maps_ctrl


  # Create instance: h2c_data_maps, and set properties
  set h2c_data_maps [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 h2c_data_maps ]
  set_property CONFIG.Memory_Type {True_Dual_Port_RAM} $h2c_data_maps


  # Create instance: c2h_data_maps_ctrl, and set properties
  set c2h_data_maps_ctrl [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 c2h_data_maps_ctrl ]
  set_property -dict [list \
    CONFIG.DATA_WIDTH {128} \
    CONFIG.SINGLE_PORT_BRAM {1} \
  ] $c2h_data_maps_ctrl


  # Create instance: c2h_data_maps, and set properties
  set c2h_data_maps [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 c2h_data_maps ]
  set_property CONFIG.Memory_Type {True_Dual_Port_RAM} $c2h_data_maps


  # Create instance: axi_interconnect_4, and set properties
  set axi_interconnect_4 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_4 ]
  set_property CONFIG.NUM_MI {3} $axi_interconnect_4


  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins axi_datamover_0/M_AXI] [get_bd_intf_pins M_AXI]
  connect_bd_intf_net -intf_net Conn2 [get_bd_intf_pins xdma_0/S_AXI_B] [get_bd_intf_pins S_AXI_B]
  connect_bd_intf_net -intf_net Conn3 [get_bd_intf_pins xdma_0/pcie_mgt] [get_bd_intf_pins pci_express_x16]
  connect_bd_intf_net -intf_net Conn4 [get_bd_intf_pins xdma_0/M_AXI_B] [get_bd_intf_pins M_AXI_B]
  connect_bd_intf_net -intf_net Conn5 [get_bd_intf_pins util_ds_buf/CLK_IN_D] [get_bd_intf_pins pcie_refclk]
  connect_bd_intf_net -intf_net S_AXI_1 [get_bd_intf_pins S_AXI] [get_bd_intf_pins axi_interconnect_4/S00_AXI]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXIS_MM2S [get_bd_intf_pins axi_datamover_0/M_AXIS_MM2S] [get_bd_intf_pins dma_read_0/data_queue_i]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXIS_MM2S_STS [get_bd_intf_pins axi_datamover_0/M_AXIS_MM2S_STS] [get_bd_intf_pins dma_read_0/status_queue_i]
  connect_bd_intf_net -intf_net axi_datamover_0_M_AXIS_S2MM_STS [get_bd_intf_pins dma_write_0/status_queue_i] [get_bd_intf_pins axi_datamover_0/M_AXIS_S2MM_STS]
  connect_bd_intf_net -intf_net axi_interconnect_4_M00_AXI [get_bd_intf_pins meteadata_maps_ctrl/S_AXI] [get_bd_intf_pins axi_interconnect_4/M00_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_4_M01_AXI [get_bd_intf_pins c2h_data_maps_ctrl/S_AXI] [get_bd_intf_pins axi_interconnect_4/M01_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_4_M02_AXI [get_bd_intf_pins axi_interconnect_4/M02_AXI] [get_bd_intf_pins h2c_data_maps_ctrl/S_AXI]
  connect_bd_intf_net -intf_net c2h_data_maps_ctrl_BRAM_PORTA [get_bd_intf_pins c2h_data_maps_ctrl/BRAM_PORTA] [get_bd_intf_pins c2h_data_maps/BRAM_PORTA]
  connect_bd_intf_net -intf_net dma_read_0_cmd_queue_o [get_bd_intf_pins axi_datamover_0/S_AXIS_MM2S_CMD] [get_bd_intf_pins dma_read_0/cmd_queue_o]
  connect_bd_intf_net -intf_net dma_read_0_dma_r_req_o [get_bd_intf_pins dma_r_req_o] [get_bd_intf_pins dma_read_0/dma_r_req_o]
  connect_bd_intf_net -intf_net dma_write_0_cmd_queue_o [get_bd_intf_pins dma_write_0/cmd_queue_o] [get_bd_intf_pins axi_datamover_0/S_AXIS_S2MM_CMD]
  connect_bd_intf_net -intf_net dma_write_0_data_queue_o [get_bd_intf_pins dma_write_0/data_queue_o] [get_bd_intf_pins axi_datamover_0/S_AXIS_S2MM]
  connect_bd_intf_net -intf_net h2c_data_maps_ctrl_BRAM_PORTA [get_bd_intf_pins h2c_data_maps_ctrl/BRAM_PORTA] [get_bd_intf_pins h2c_data_maps/BRAM_PORTA]
  connect_bd_intf_net -intf_net hostsharedmemctrl_BRAM_PORTA [get_bd_intf_pins meteadata_maps_ctrl/BRAM_PORTA] [get_bd_intf_pins metadata_maps/BRAM_PORTA]

  # Create port connections
  connect_bd_net -net ACLK_1 [get_bd_pins axi_interconnect_4/ACLK] [get_bd_pins c2h_data_maps_ctrl/s_axi_aclk] [get_bd_pins axi_interconnect_4/S00_ACLK] [get_bd_pins axi_interconnect_4/M00_ACLK] [get_bd_pins axi_interconnect_4/M01_ACLK] [get_bd_pins axi_interconnect_4/M02_ACLK]
  connect_bd_net -net ARESETN_1 [get_bd_pins axi_interconnect_4/ARESETN] [get_bd_pins c2h_data_maps_ctrl/s_axi_aresetn] [get_bd_pins axi_interconnect_4/M00_ARESETN] [get_bd_pins axi_interconnect_4/S00_ARESETN] [get_bd_pins axi_interconnect_4/M01_ARESETN] [get_bd_pins axi_interconnect_4/M02_ARESETN]
  connect_bd_net -net ap_clk_1 [get_bd_pins ap_clk] [get_bd_pins dma_write_0/ap_clk] [get_bd_pins dma_read_0/ap_clk]
  connect_bd_net -net ap_rst_n_1 [get_bd_pins ap_rst_n] [get_bd_pins dma_write_0/ap_rst_n] [get_bd_pins dma_read_0/ap_rst_n]
  connect_bd_net -net m_axis_mm2s_cmdsts_aclk_1 [get_bd_pins m_axis_mm2s_cmdsts_aclk] [get_bd_pins axi_datamover_0/m_axis_mm2s_cmdsts_aclk] [get_bd_pins axi_datamover_0/m_axi_s2mm_aclk] [get_bd_pins axi_datamover_0/m_axis_s2mm_cmdsts_awclk] [get_bd_pins axi_datamover_0/m_axi_mm2s_aclk] [get_bd_pins meteadata_maps_ctrl/s_axi_aclk] [get_bd_pins h2c_data_maps_ctrl/s_axi_aclk]
  connect_bd_net -net m_axis_s2mm_cmdsts_aresetn_1 [get_bd_pins m_axis_s2mm_cmdsts_aresetn] [get_bd_pins axi_datamover_0/m_axis_s2mm_cmdsts_aresetn] [get_bd_pins axi_datamover_0/m_axi_s2mm_aresetn] [get_bd_pins axi_datamover_0/m_axis_mm2s_cmdsts_aresetn] [get_bd_pins axi_datamover_0/m_axi_mm2s_aresetn] [get_bd_pins meteadata_maps_ctrl/s_axi_aresetn] [get_bd_pins h2c_data_maps_ctrl/s_axi_aresetn]
  connect_bd_net -net pcie_perstn_1 [get_bd_pins pcie_perstn] [get_bd_pins xdma_0/sys_rst_n]
  connect_bd_net -net util_ds_buf_IBUF_DS_ODIV2 [get_bd_pins util_ds_buf/IBUF_DS_ODIV2] [get_bd_pins xdma_0/sys_clk]
  connect_bd_net -net util_ds_buf_IBUF_OUT [get_bd_pins util_ds_buf/IBUF_OUT] [get_bd_pins xdma_0/sys_clk_gt]
  connect_bd_net -net xdma_0_axi_aclk [get_bd_pins xdma_0/axi_aclk] [get_bd_pins axi_aclk]
  connect_bd_net -net xdma_0_axi_aresetn [get_bd_pins xdma_0/axi_aresetn] [get_bd_pins axi_aresetn]
  connect_bd_net -net xlconstant_0_dout [get_bd_pins xlconstant_0/dout] [get_bd_pins xdma_0/usr_irq_req]

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
  set default_300mhz_clk0 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 default_300mhz_clk0 ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {300000000} \
   ] $default_300mhz_clk0

  set pci_express_x16 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:pcie_7x_mgt_rtl:1.0 pci_express_x16 ]

  set pcie_refclk [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 pcie_refclk ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {100000000} \
   ] $pcie_refclk

  set qsfp1_156mhz [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 qsfp1_156mhz ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {156250000} \
   ] $qsfp1_156mhz

  set qsfp1_4x [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gt_rtl:1.0 qsfp1_4x ]


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
    CONFIG.CLKOUT2_JITTER {101.475} \
    CONFIG.CLKOUT2_PHASE_ERROR {77.836} \
    CONFIG.CLKOUT2_USED {true} \
    CONFIG.CLKOUT3_JITTER {109.006} \
    CONFIG.CLKOUT3_PHASE_ERROR {98.575} \
    CONFIG.CLKOUT3_REQUESTED_OUT_FREQ {100.000} \
    CONFIG.CLKOUT3_USED {false} \
    CONFIG.CLK_IN1_BOARD_INTERFACE {default_300mhz_clk0} \
    CONFIG.MMCM_CLKFBOUT_MULT_F {4.000} \
    CONFIG.MMCM_CLKIN1_PERIOD {3.333} \
    CONFIG.MMCM_CLKIN2_PERIOD {10.0} \
    CONFIG.MMCM_CLKOUT0_DIVIDE_F {6.000} \
    CONFIG.MMCM_CLKOUT1_DIVIDE {12} \
    CONFIG.MMCM_CLKOUT2_DIVIDE {1} \
    CONFIG.MMCM_DIVCLK_DIVIDE {1} \
    CONFIG.NUM_OUT_CLKS {2} \
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


  # Create instance: fifo_generator_0, and set properties
  set fifo_generator_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:fifo_generator:13.2 fifo_generator_0 ]
  set_property -dict [list \
    CONFIG.Clock_Type_AXI {Independent_Clock} \
    CONFIG.Enable_TLAST {true} \
    CONFIG.FIFO_Application_Type_axis {Packet_FIFO} \
    CONFIG.FIFO_Implementation_axis {Independent_Clocks_Block_RAM} \
    CONFIG.HAS_TKEEP {true} \
    CONFIG.INTERFACE_TYPE {AXI_STREAM} \
    CONFIG.Input_Depth_axis {32} \
    CONFIG.TDATA_NUM_BYTES {64} \
  ] $fifo_generator_0


  # Create instance: fifo_generator_1, and set properties
  set fifo_generator_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:fifo_generator:13.2 fifo_generator_1 ]
  set_property -dict [list \
    CONFIG.Clock_Type_AXI {Independent_Clock} \
    CONFIG.Enable_TLAST {true} \
    CONFIG.FIFO_Application_Type_axis {Data_FIFO} \
    CONFIG.FIFO_Implementation_axis {Independent_Clocks_Block_RAM} \
    CONFIG.HAS_TKEEP {true} \
    CONFIG.HAS_TSTRB {false} \
    CONFIG.INTERFACE_TYPE {AXI_STREAM} \
    CONFIG.Input_Depth_axis {32} \
    CONFIG.TDATA_NUM_BYTES {64} \
  ] $fifo_generator_1


  # Create instance: util_vector_logic_0, and set properties
  set util_vector_logic_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 util_vector_logic_0 ]
  set_property -dict [list \
    CONFIG.C_OPERATION {not} \
    CONFIG.C_SIZE {1} \
  ] $util_vector_logic_0


  # Create instance: ila_0, and set properties
  set ila_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_0 ]
  set_property CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} $ila_0


  # Create instance: ila_1, and set properties
  set ila_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_1 ]
  set_property CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} $ila_1


  # Create instance: xlconstant_1, and set properties
  set xlconstant_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_1 ]
  set_property CONFIG.CONST_VAL {0} $xlconstant_1


  # Create instance: cmac_usplus_1, and set properties
  set cmac_usplus_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:cmac_usplus:3.1 cmac_usplus_1 ]
  set_property -dict [list \
    CONFIG.DIFFCLK_BOARD_INTERFACE {qsfp1_156mhz} \
    CONFIG.ENABLE_AXI_INTERFACE {1} \
    CONFIG.ETHERNET_BOARD_INTERFACE {qsfp1_4x} \
    CONFIG.GT_DRP_CLK {100.00} \
    CONFIG.INCLUDE_AUTO_NEG_LT_LOGIC {0} \
    CONFIG.INCLUDE_RS_FEC {0} \
    CONFIG.INS_LOSS_NYQ {20} \
    CONFIG.OPERATING_MODE {Duplex} \
    CONFIG.RX_FLOW_CONTROL {0} \
    CONFIG.TX_FLOW_CONTROL {0} \
    CONFIG.TX_OTN_INTERFACE {0} \
    CONFIG.USER_INTERFACE {AXIS} \
    CONFIG.USE_BOARD_FLOW {true} \
  ] $cmac_usplus_1


  # Create instance: axi_gpio_0, and set properties
  set axi_gpio_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_0 ]
  set_property -dict [list \
    CONFIG.C_ALL_OUTPUTS {1} \
    CONFIG.C_DOUT_DEFAULT {0x00000000} \
    CONFIG.C_GPIO_WIDTH {1} \
  ] $axi_gpio_0


  # Create instance: axi_interconnect_0, and set properties
  set axi_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_0 ]
  set_property CONFIG.NUM_MI {5} $axi_interconnect_0


  # Create instance: axi_interconnect_1, and set properties
  set axi_interconnect_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_1 ]
  set_property CONFIG.NUM_MI {3} $axi_interconnect_1


  # Create instance: axi_clock_converter_3, and set properties
  set axi_clock_converter_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_clock_converter:2.1 axi_clock_converter_3 ]

  # Create instance: rst_mainClk_200M, and set properties
  set rst_mainClk_200M [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_mainClk_200M ]

  # Create instance: ila_2, and set properties
  set ila_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ila:6.2 ila_2 ]

  # Create instance: xlslice_1, and set properties
  set xlslice_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlslice:1.0 xlslice_1 ]
  set_property -dict [list \
    CONFIG.DIN_FROM {0} \
    CONFIG.DIN_TO {0} \
  ] $xlslice_1


  # Create instance: util_vector_logic_1, and set properties
  set util_vector_logic_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 util_vector_logic_1 ]
  set_property -dict [list \
    CONFIG.C_OPERATION {not} \
    CONFIG.C_SIZE {1} \
  ] $util_vector_logic_1


  # Create instance: cpu_bram, and set properties
  set cpu_bram [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.4 cpu_bram ]
  set_property -dict [list \
    CONFIG.Enable_32bit_Address {true} \
    CONFIG.Memory_Type {True_Dual_Port_RAM} \
    CONFIG.Register_PortA_Output_of_Memory_Primitives {false} \
    CONFIG.Register_PortB_Output_of_Memory_Primitives {false} \
    CONFIG.Use_RSTA_Pin {true} \
    CONFIG.Use_RSTB_Pin {true} \
    CONFIG.use_bram_block {BRAM_Controller} \
  ] $cpu_bram


  # Create instance: axi_bram_ctrl_1, and set properties
  set axi_bram_ctrl_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_1 ]
  set_property CONFIG.SINGLE_PORT_BRAM {1} $axi_bram_ctrl_1


  # Create instance: axi_bram_ctrl_2, and set properties
  set axi_bram_ctrl_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_2 ]
  set_property -dict [list \
    CONFIG.PROTOCOL {AXI4LITE} \
    CONFIG.SINGLE_PORT_BRAM {1} \
  ] $axi_bram_ctrl_2


  # Create instance: system_ila_0, and set properties
  set system_ila_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 system_ila_0 ]

  # Create instance: system_ila_1, and set properties
  set system_ila_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 system_ila_1 ]
  set_property CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:bram_rtl:1.0} $system_ila_1


  # Create instance: system_ila_2, and set properties
  set system_ila_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 system_ila_2 ]
  set_property CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:bram_rtl:1.0} $system_ila_2


  # Create instance: axi_interconnect_2, and set properties
  set axi_interconnect_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_2 ]

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

  # Create instance: system_ila_3, and set properties
  set system_ila_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 system_ila_3 ]
  set_property CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} $system_ila_3


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
    set_property CONFIG.C_S_AXI_ID_WIDTH {4} $axi2axis_0


  set_property -dict [ list \
   CONFIG.FREQ_HZ {200000000} \
 ] [get_bd_intf_pins /axi2axis_0/M_AXIS]

  set_property -dict [ list \
   CONFIG.FREQ_HZ {200000000} \
 ] [get_bd_intf_pins /axi2axis_0/S_AXIS]

  # Create instance: homa, and set properties
  set homa [ create_bd_cell -type ip -vlnv xilinx.com:hls:homa:1.0 homa ]

  # Create instance: system_ila_4, and set properties
  set system_ila_4 [ create_bd_cell -type ip -vlnv xilinx.com:ip:system_ila:1.1 system_ila_4 ]
  set_property CONFIG.C_SLOT_0_INTF_TYPE {xilinx.com:interface:axis_rtl:1.0} $system_ila_4


  # Create instance: dma
  create_hier_cell_dma [current_bd_instance .] dma

  # Create instance: pqs
  create_hier_cell_pqs [current_bd_instance .] pqs

  # Create instance: cache
  create_hier_cell_cache [current_bd_instance .] cache

  # Create instance: control
  create_hier_cell_control [current_bd_instance .] control

  # Create interface connections
  connect_bd_intf_net -intf_net S00_AXI_1 [get_bd_intf_pins axi_interconnect_1/S00_AXI] [get_bd_intf_pins dma/M_AXI_B]
  connect_bd_intf_net -intf_net S00_AXI_2 [get_bd_intf_pins axi_interconnect_2/S00_AXI] [get_bd_intf_pins axi_clock_converter_3/M_AXI]
  connect_bd_intf_net -intf_net S00_AXI_3 [get_bd_intf_pins axi_interconnect_3/S00_AXI] [get_bd_intf_pins picorv32_axi_0/mem_axi]
  connect_bd_intf_net -intf_net axi2axis_0_M_AXIS [get_bd_intf_pins axi2axis_0/M_AXIS] [get_bd_intf_pins control/addr_in]
connect_bd_intf_net -intf_net [get_bd_intf_nets axi2axis_0_M_AXIS] [get_bd_intf_pins axi2axis_0/M_AXIS] [get_bd_intf_pins system_ila_3/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net axi_bram_ctrl_1_BRAM_PORTA [get_bd_intf_pins axi_bram_ctrl_1/BRAM_PORTA] [get_bd_intf_pins cpu_bram/BRAM_PORTA]
connect_bd_intf_net -intf_net [get_bd_intf_nets axi_bram_ctrl_1_BRAM_PORTA] [get_bd_intf_pins axi_bram_ctrl_1/BRAM_PORTA] [get_bd_intf_pins system_ila_2/SLOT_0_BRAM]
  connect_bd_intf_net -intf_net axi_bram_ctrl_2_BRAM_PORTA [get_bd_intf_pins axi_bram_ctrl_2/BRAM_PORTA] [get_bd_intf_pins cpu_bram/BRAM_PORTB]
connect_bd_intf_net -intf_net [get_bd_intf_nets axi_bram_ctrl_2_BRAM_PORTA] [get_bd_intf_pins axi_bram_ctrl_2/BRAM_PORTA] [get_bd_intf_pins system_ila_1/SLOT_0_BRAM]
  connect_bd_intf_net -intf_net axi_clock_converter_0_M_AXI [get_bd_intf_pins axi_clock_converter_0/M_AXI] [get_bd_intf_pins axi_interconnect_0/S00_AXI]
  connect_bd_intf_net -intf_net axi_clock_converter_1_M_AXI [get_bd_intf_pins axi_clock_converter_1/M_AXI] [get_bd_intf_pins dma/S_AXI_B]
  connect_bd_intf_net -intf_net axi_interconnect_0_M00_AXI [get_bd_intf_pins axi_interconnect_0/M00_AXI] [get_bd_intf_pins dma/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M02_AXI [get_bd_intf_pins axi_interconnect_0/M02_AXI] [get_bd_intf_pins axi2axis_0/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M03_AXI [get_bd_intf_pins axi_interconnect_0/M03_AXI] [get_bd_intf_pins axi_gpio_0/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M04_AXI [get_bd_intf_pins axi_interconnect_0/M04_AXI] [get_bd_intf_pins axi_bram_ctrl_1/S_AXI]
connect_bd_intf_net -intf_net [get_bd_intf_nets axi_interconnect_0_M04_AXI] [get_bd_intf_pins axi_interconnect_0/M04_AXI] [get_bd_intf_pins ila_2/SLOT_0_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_1_M00_AXI [get_bd_intf_pins axi_interconnect_1/M00_AXI] [get_bd_intf_pins axi_clock_converter_0/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_1_M01_AXI [get_bd_intf_pins axi_interconnect_1/M01_AXI] [get_bd_intf_pins axi_clock_converter_3/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_2_M00_AXI [get_bd_intf_pins axi_interconnect_2/M00_AXI] [get_bd_intf_pins cmac_usplus_1/s_axi]
  connect_bd_intf_net -intf_net cache_dbuff_notif_o [get_bd_intf_pins cache/dbuff_notif_o] [get_bd_intf_pins pqs/S00_AXIS]
  connect_bd_intf_net -intf_net cmac_usplus_1_axis_rx [get_bd_intf_pins cmac_usplus_1/axis_rx] [get_bd_intf_pins fifo_generator_1/S_AXIS]
  connect_bd_intf_net -intf_net cmac_usplus_1_gt_serial_port [get_bd_intf_ports qsfp1_4x] [get_bd_intf_pins cmac_usplus_1/gt_serial_port]
  connect_bd_intf_net -intf_net default_300mhz_clk0_1 [get_bd_intf_ports default_300mhz_clk0] [get_bd_intf_pins mainClk/CLK_IN1_D]
  connect_bd_intf_net -intf_net dma_M_AXI [get_bd_intf_pins axi_clock_converter_1/S_AXI] [get_bd_intf_pins dma/M_AXI]
  connect_bd_intf_net -intf_net dma_dma_r_req_o [get_bd_intf_pins dma/dma_r_req_o] [get_bd_intf_pins cache/new_entry_i]
  connect_bd_intf_net -intf_net dma_pci_express_x16 [get_bd_intf_ports pci_express_x16] [get_bd_intf_pins dma/pci_express_x16]
  connect_bd_intf_net -intf_net fifo_generator_0_M_AXIS [get_bd_intf_pins fifo_generator_0/M_AXIS] [get_bd_intf_pins cmac_usplus_1/axis_tx]
  connect_bd_intf_net -intf_net fifo_generator_1_M_AXIS [get_bd_intf_pins fifo_generator_1/M_AXIS] [get_bd_intf_pins homa/link_ingress_i]
connect_bd_intf_net -intf_net [get_bd_intf_nets fifo_generator_1_M_AXIS] [get_bd_intf_pins fifo_generator_1/M_AXIS] [get_bd_intf_pins ila_0/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net homa_link_egress_o [get_bd_intf_pins fifo_generator_0/S_AXIS] [get_bd_intf_pins homa/link_egress_o]
connect_bd_intf_net -intf_net [get_bd_intf_nets homa_link_egress_o] [get_bd_intf_pins fifo_generator_0/S_AXIS] [get_bd_intf_pins ila_1/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net homa_log_out_o [get_bd_intf_pins homa/log_out_o] [get_bd_intf_pins axi2axis_0/S_AXIS]
connect_bd_intf_net -intf_net [get_bd_intf_nets homa_log_out_o] [get_bd_intf_pins homa/log_out_o] [get_bd_intf_pins system_ila_4/SLOT_0_AXIS]
  connect_bd_intf_net -intf_net pcie_refclk_1 [get_bd_intf_ports pcie_refclk] [get_bd_intf_pins dma/pcie_refclk]
  connect_bd_intf_net -intf_net picorv32_axi_0_mem_axi [get_bd_intf_pins axi_interconnect_3/M00_AXI] [get_bd_intf_pins axi_bram_ctrl_2/S_AXI]
connect_bd_intf_net -intf_net [get_bd_intf_nets picorv32_axi_0_mem_axi] [get_bd_intf_pins axi_interconnect_3/M00_AXI] [get_bd_intf_pins system_ila_0/SLOT_0_AXI]
  connect_bd_intf_net -intf_net qsfp1_156mhz_1 [get_bd_intf_ports qsfp1_156mhz] [get_bd_intf_pins cmac_usplus_1/gt_ref_clk]

  # Create port connections
  connect_bd_net -net ap_clk_1 [get_bd_pins dma/ap_clk] [get_bd_pins pqs/ap_clk] [get_bd_pins cache/ap_clk]
  connect_bd_net -net ap_rst_n_1 [get_bd_pins dma/ap_rst_n] [get_bd_pins pqs/ap_rst] [get_bd_pins cache/ap_rst_n]
  connect_bd_net -net axi_gpio_0_gpio_io_o [get_bd_pins axi_gpio_0/gpio_io_o] [get_bd_pins xlslice_1/Din]
  connect_bd_net -net cmac_usplus_1_gt_rxusrclk2 [get_bd_pins cmac_usplus_1/gt_rxusrclk2] [get_bd_pins cmac_usplus_1/rx_clk] [get_bd_pins fifo_generator_1/s_aclk]
  connect_bd_net -net cmac_usplus_1_gt_txusrclk2 [get_bd_pins cmac_usplus_1/gt_txusrclk2] [get_bd_pins fifo_generator_0/m_aclk]
  connect_bd_net -net cmac_usplus_1_usr_rx_reset [get_bd_pins cmac_usplus_1/usr_rx_reset] [get_bd_pins util_vector_logic_0/Op1]
  connect_bd_net -net mainClk_clk_out2 [get_bd_pins mainClk/clk_out2] [get_bd_pins proc_sys_reset_0/slowest_sync_clk] [get_bd_pins cmac_usplus_1/s_axi_aclk] [get_bd_pins axi_clock_converter_3/m_axi_aclk] [get_bd_pins rst_mainClk_200M/slowest_sync_clk] [get_bd_pins axi_interconnect_2/ACLK] [get_bd_pins axi_interconnect_2/S00_ACLK] [get_bd_pins axi_interconnect_2/M00_ACLK] [get_bd_pins axi_interconnect_2/M01_ACLK]
  connect_bd_net -net mainClk_locked [get_bd_pins mainClk/locked] [get_bd_pins proc_sys_reset_0/dcm_locked] [get_bd_pins rst_mainClk_200M/dcm_locked]
  connect_bd_net -net microblaze_0_Clk [get_bd_pins mainClk/clk_out1] [get_bd_pins axi_clock_converter_0/m_axi_aclk] [get_bd_pins axi_clock_converter_1/s_axi_aclk] [get_bd_pins fifo_generator_0/s_aclk] [get_bd_pins fifo_generator_1/m_aclk] [get_bd_pins ila_0/clk] [get_bd_pins ila_1/clk] [get_bd_pins cmac_usplus_1/init_clk] [get_bd_pins axi_gpio_0/s_axi_aclk] [get_bd_pins axi_interconnect_0/ACLK] [get_bd_pins axi_interconnect_0/S00_ACLK] [get_bd_pins axi_interconnect_0/M00_ACLK] [get_bd_pins axi_interconnect_0/M01_ACLK] [get_bd_pins axi_interconnect_0/M02_ACLK] [get_bd_pins axi_interconnect_0/M03_ACLK] [get_bd_pins axi_interconnect_0/M04_ACLK] [get_bd_pins ila_2/clk] [get_bd_pins axi_bram_ctrl_1/s_axi_aclk] [get_bd_pins axi_bram_ctrl_2/s_axi_aclk] [get_bd_pins system_ila_0/clk] [get_bd_pins system_ila_1/clk] [get_bd_pins system_ila_2/clk] [get_bd_pins picorv32_axi_0/clk] [get_bd_pins axi_interconnect_3/M01_ACLK] [get_bd_pins axi_interconnect_3/M00_ACLK] [get_bd_pins axi_interconnect_3/S00_ACLK] [get_bd_pins axi_interconnect_3/ACLK] [get_bd_pins system_ila_3/clk] [get_bd_pins axi2axis_0/S_AXI_ACLK] [get_bd_pins homa/ap_clk] [get_bd_pins system_ila_4/clk] [get_bd_pins dma/m_axis_mm2s_cmdsts_aclk] [get_bd_pins control/ap_clk]
  connect_bd_net -net pcie_perstn_1 [get_bd_ports pcie_perstn] [get_bd_pins dma/pcie_perstn]
  connect_bd_net -net proc_sys_reset_0_peripheral_aresetn [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins axi_clock_converter_1/s_axi_aresetn] [get_bd_pins axi_clock_converter_0/m_axi_aresetn] [get_bd_pins fifo_generator_0/s_aresetn] [get_bd_pins axi_gpio_0/s_axi_aresetn] [get_bd_pins axi_interconnect_0/ARESETN] [get_bd_pins axi_interconnect_0/M03_ARESETN] [get_bd_pins axi_interconnect_0/M02_ARESETN] [get_bd_pins axi_interconnect_0/M01_ARESETN] [get_bd_pins axi_interconnect_0/M00_ARESETN] [get_bd_pins axi_interconnect_0/S00_ARESETN] [get_bd_pins axi_clock_converter_3/m_axi_aresetn] [get_bd_pins axi_interconnect_0/M04_ARESETN] [get_bd_pins axi_bram_ctrl_1/s_axi_aresetn] [get_bd_pins axi_bram_ctrl_2/s_axi_aresetn] [get_bd_pins system_ila_0/resetn] [get_bd_pins axi_interconnect_2/ARESETN] [get_bd_pins axi_interconnect_2/S00_ARESETN] [get_bd_pins axi_interconnect_2/M00_ARESETN] [get_bd_pins axi_interconnect_2/M01_ARESETN] [get_bd_pins axi_interconnect_3/ARESETN] [get_bd_pins axi_interconnect_3/S00_ARESETN] [get_bd_pins axi_interconnect_3/M00_ARESETN] [get_bd_pins axi_interconnect_3/M01_ARESETN] [get_bd_pins system_ila_3/resetn] [get_bd_pins axi2axis_0/S_AXI_ARESETN] [get_bd_pins homa/ap_rst_n] [get_bd_pins system_ila_4/resetn] [get_bd_pins dma/m_axis_s2mm_cmdsts_aresetn] [get_bd_pins control/ap_rst_n]
  connect_bd_net -net proc_sys_reset_0_peripheral_reset [get_bd_pins proc_sys_reset_0/peripheral_reset] [get_bd_pins cmac_usplus_1/s_axi_sreset] [get_bd_pins cmac_usplus_1/sys_reset]
  connect_bd_net -net resetn_2 [get_bd_ports resetn] [get_bd_pins proc_sys_reset_0/ext_reset_in] [get_bd_pins mainClk/resetn] [get_bd_pins rst_mainClk_200M/ext_reset_in]
  connect_bd_net -net rst_mainClk_200M_mb_reset [get_bd_pins rst_mainClk_200M/mb_reset] [get_bd_pins util_vector_logic_1/Op1]
  connect_bd_net -net util_vector_logic_0_Res [get_bd_pins util_vector_logic_0/Res] [get_bd_pins fifo_generator_1/s_aresetn]
  connect_bd_net -net util_vector_logic_1_Res [get_bd_pins util_vector_logic_1/Res] [get_bd_pins picorv32_axi_0/resetn]
  connect_bd_net -net xdma_0_axi_aclk [get_bd_pins dma/axi_aclk] [get_bd_pins axi_clock_converter_0/s_axi_aclk] [get_bd_pins axi_clock_converter_1/m_axi_aclk] [get_bd_pins axi_interconnect_1/ACLK] [get_bd_pins axi_interconnect_1/S00_ACLK] [get_bd_pins axi_interconnect_1/M00_ACLK] [get_bd_pins axi_interconnect_1/M01_ACLK] [get_bd_pins axi_interconnect_1/M02_ACLK] [get_bd_pins axi_clock_converter_3/s_axi_aclk]
  connect_bd_net -net xdma_0_axi_aresetn [get_bd_pins dma/axi_aresetn] [get_bd_pins axi_clock_converter_0/s_axi_aresetn] [get_bd_pins axi_clock_converter_1/m_axi_aresetn] [get_bd_pins axi_interconnect_1/M01_ARESETN] [get_bd_pins axi_interconnect_1/M00_ARESETN] [get_bd_pins axi_interconnect_1/S00_ARESETN] [get_bd_pins axi_interconnect_1/ARESETN] [get_bd_pins axi_interconnect_1/M02_ARESETN] [get_bd_pins axi_clock_converter_3/s_axi_aresetn]
  connect_bd_net -net xlconstant_1_dout1 [get_bd_pins xlconstant_1/dout] [get_bd_pins cmac_usplus_1/gtwiz_reset_tx_datapath] [get_bd_pins cmac_usplus_1/gtwiz_reset_rx_datapath] [get_bd_pins cmac_usplus_1/core_rx_reset] [get_bd_pins cmac_usplus_1/core_tx_reset] [get_bd_pins cmac_usplus_1/drp_clk] [get_bd_pins cmac_usplus_1/core_drp_reset] [get_bd_pins cmac_usplus_1/pm_tick]
  connect_bd_net -net xlslice_1_Dout [get_bd_pins xlslice_1/Dout] [get_bd_pins rst_mainClk_200M/aux_reset_in]

  # Create address segments
  assign_bd_address -offset 0x00000000 -range 0x00002000 -target_address_space [get_bd_addr_spaces picorv32_axi_0/mem_axi] [get_bd_addr_segs axi_bram_ctrl_2/S_AXI/Mem0] -force
  assign_bd_address -offset 0x00000000 -range 0x008000000000 -target_address_space [get_bd_addr_spaces dma/axi_datamover_0/Data] [get_bd_addr_segs dma/xdma_0/S_AXI_B/BAR0] -force
  assign_bd_address -offset 0x00000000 -range 0x00001000 -target_address_space [get_bd_addr_spaces dma/xdma_0/M_AXI_B] [get_bd_addr_segs axi2axis_0/S_AXI/reg0] -force
  assign_bd_address -offset 0x00040000 -range 0x00002000 -target_address_space [get_bd_addr_spaces dma/xdma_0/M_AXI_B] [get_bd_addr_segs axi_bram_ctrl_1/S_AXI/Mem0] -force
  assign_bd_address -offset 0x00050000 -range 0x00010000 -target_address_space [get_bd_addr_spaces dma/xdma_0/M_AXI_B] [get_bd_addr_segs axi_gpio_0/S_AXI/Reg] -force
  assign_bd_address -offset 0x00030000 -range 0x00010000 -target_address_space [get_bd_addr_spaces dma/xdma_0/M_AXI_B] [get_bd_addr_segs cmac_usplus_1/s_axi/Reg] -force


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

