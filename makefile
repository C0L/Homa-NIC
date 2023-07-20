LINTER = verilator -lint-only +1364-2001ext+v --unroll-count 1024 
VITIS = vitis_hls
VIVADO = vivado

C_SRC_DIR = ./src
C_TB_DIR  = ./src
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
    $(C_SRC_DIR)/user.cc     \
    $(C_SRC_DIR)/user.hh     \
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
    
# TB_C = $(C_TB_DIR)/client_test.cc

XDC = \
   $(XDC_DIR)/clocks.xdc

PART = xcvu9p-flgb2104-2-i

CSIM = 0
SYNTH = 1
COSIM = 2

############ Vitis C Synth ############ 

synth:
	$(VITIS) tcl/homa.tcl -tclargs $(PART) $(SYNTH) "$(SRC_C)" "$(SRC_JSON)" $(C_TB_DIR)/test_unscheduled_exchange.cc

############ Vitis C Simulation ############ 

csim_unscheduled_exchange:
	$(VITIS) tcl/homa.tcl -tclargs $(PART) $(CSIM) "$(SRC_C)" "$(SRC_JSON)" $(C_TB_DIR)/test_unscheduled_exchange.cc

csim_scheduled_exchange:
	$(VITIS) tcl/homa.tcl -tclargs $(PART) $(CSIM) "$(SRC_C)" "$(SRC_JSON)" $(C_TB_DIR)/test_scheduled_exchange.cc

############ Vitis Cosim ############ 

cosim_unscheduled_exchange:
	$(VITIS) tcl/homa.tcl -tclargs $(PART) $(COSIM) "$(SRC_C)" "$(SRC_JSON)" $(C_TB_DIR)/test_unscheduled_exchange.cc

cosim_scheduled_exchange:
	$(VITIS) tcl/homa.tcl -tclargs $(PART) $(COSIM) "$(SRC_C)" "$(SRC_JSON)" $(C_TB_DIR)/test_scheduled_exchange.cc

############ Verilog Synthesis ############ 

vsynth:
	# TODO generalize this tcl script
	$(VIVADO) -mode tcl -source tcl/srpt_grant_synth.tcl

############ Verilog Simulation ############ 

waves: xsim
	xsim --gui srpt_grant_queue_tb_snapshot.wdb

xsim: xsim_elaborate
	xsim srpt_grant_queue_tb_snapshot --tclbatch ./tcl/xsim_cfg.tcl

xsim_elaborate: xsim_compile
	xelab -debug all -top srpt_grant_queue_tb --snapshot srpt_grant_queue_tb_snapshot

xsim_compile: 
	xvlog ./src/srpt_grant_pkts.v ./src/srpt_grant_queue_tb.v

############ Verilog Linting ############ 

lint_srpt_grant_pkts:
	$(LINTER) ./src/srpt_grant_pkts.v ./src/srpt_grant_queue_tb.v

lint_srpt_data_pkts:
	$(LINTER) ./src/srpt_data_pkts.v ./src/srpt_data_queue_tb.v

clean:
	rm -f vitis_hls.log
	rm -rf homa
	rm -rf xsim.dir
	rm -f *.log
	rm -f *.jou
	rm -f *.log
	rm -f xelab*
	rm -f xvlog*
	rm -f srpt_grant_queue_tb_snapshot.wdb
