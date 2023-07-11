LINTER = verilator -lint-only +1364-2001ext+v --unroll-count 1024 
VITIS = vitis_hls
VIVADO = vivado

C_SRC_DIR = ./src
C_TB_DIR  = ./src
# C_TB_DIR  = ./tb
V_SRC_DIR = ./src
V_TB_DIR  = ./src
TCL_DIR   = ./tcl
XDC_DIR   = ./xdc

SRC_V = \
        $(V_SRC_DIR)/srpt_grant_pkts.v

TB_V = \
        $(V_SRC_DIR)/srpt_grant_pkts.v

SRC_JSON = \
        $(V_SRC_DIR)/srpt_grant_pkts.json

SRC_C =                      \
    $(C_SRC_DIR)/databuff.cc \
    $(C_SRC_DIR)/databuff.hh \
    $(C_SRC_DIR)/dma.cc      \
    $(C_SRC_DIR)/dma.hh      \
    $(C_SRC_DIR)/hashmap.hh  \
    $(C_SRC_DIR)/homa.cc     \
    $(C_SRC_DIR)/homa.hh     \
    $(C_SRC_DIR)/link.cc     \
    $(C_SRC_DIR)/link.hh     \
    $(C_SRC_DIR)/peer.cc     \
    $(C_SRC_DIR)/peer.hh     \
    $(C_SRC_DIR)/rpcmgmt.cc  \
    $(C_SRC_DIR)/rpcmgmt.hh  \
    $(C_SRC_DIR)/srptmgmt.cc \
    $(C_SRC_DIR)/srptmgmt.hh \
    $(C_SRC_DIR)/stack.hh    \
    
#$(C_TB_DIR)/client_test.hh \

TB_C = $(C_TB_DIR)/client_test.cc

XDC = \
   $(XDC_DIR)/clocks.xdc

PART = xcvu9p-flgb2104-2-i

CSIM = 0
SYNTH = 1
COSIM = 2

# .PHONY: all vlint vtest clean

# all: vlint xsim synth

#homa:
#       vitis_hls tcl/synth.tcl
#
#homa_test:
#
#homa_synth:
#
#srpt_queue_synth:
#       vivado -mode tcl -source tcl/srpt_grant_synth.tcl
#
#srpt_queue_test:
#       vivado -mode tcl -source tcl/srpt_grant_test.tcl
#
#
#unit_test:
#       vitis_hls tcl/unit_tests.tcl
#
#srptmgmt_test:
#       vitis_hls tcl/srptmgmt_test.tcl
#
#link_test:
#       vitis_hls tcl/link_test.tcl
#
#
############ Vitis C Synth ############ 

synth:
	$(VITIS) tcl/homa.tcl -tclargs $(PART) $(SYNTH) "$(SRC_C)" "$(SRC_JSON)" $(TB_C)

############ Vitis C Simulation ############ 

csim:
	$(VITIS) tcl/homa.tcl -tclargs $(PART) $(CSIM) "$(SRC_C)" "$(SRC_JSON)" $(TB_C)

############ Vitis Cosim ############ 

cosim:
	$(VITIS) tcl/homa.tcl -tclargs $(PART) $(COSIM) "$(SRC_C)" "$(SRC_JSON)" $(TB_C)

############ Verilog Synthesis ############ 

vsynth:
	# TODO generalize this tcl script
	$(VIVADO) -mode tcl -source tcl/srpt_grant_synth.tcl

############ Verilog Simulation ############ 

#TODO generalize this section

waves: xsim
	xsim --gui srpt_grant_queue_tb_snapshot.wdb

xsim: xsim_elaborate
	xsim srpt_grant_queue_tb_snapshot --tclbatch ./tcl/xsim_cfg.tcl

xsim_elaborate: xsim_compile
	xelab -debug all -top srpt_grant_pkst--snapshot srpt_grant_queue_tb_snapshot

xsim_compile: 
	xvlog $(SRC_V) $(TB_V)

vlint:
	$(LINTER) $(SRC_V) $(TB_V)

clean:
	rm vitis_hls.log
	rm -r homa
	rm -rf xsim.dir
	rm *.log
	rm *.jou
	rm *.log
	rm xelab*
