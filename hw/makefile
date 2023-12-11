# Verilog linter (2001 version) -- unroll count needed for larege loops
LINTER = verilator -lint-only +1364-2001ext+v --unroll-count 1024

# XILINX_HLS includes 
# XILINX_HLS = /home/chdrewes/Vitis_HLS/2023.1/include/

# C synthesis tool (2023.1 currently tested)
VITIS  = vitis_hls

# Synthesis, Placement, Routing, Bitstream Generation (2023.1 current tested)
VIVADO = vivado

# Verilog Simulator
VS    = xsim

# Verilog Elaboration 
VE    = xelab

# Verilog Compiler for Simulator
VC    = xvlog 

C_SRC_DIR = ./src
C_TB_DIR  = ./src
V_SRC_DIR = ./src
V_TB_DIR  = ./src
TCL_DIR   = ./tcl

SRC_V = \
        $(V_SRC_DIR)/srpt_grant_queue.v

# TB_V = \
#         $(V_SRC_DIR)/srpt_grant_pkts.v


SRC_JSON = \
        $(V_SRC_DIR)/srpt_grant_pkts.json \
        $(V_SRC_DIR)/srpt_data_queue.json \
        $(V_SRC_DIR)/srpt_fetch_queue.json

CSRC_INF =                     \
    $(C_SRC_DIR)/interface.cc  \
    $(C_SRC_DIR)/interface.hh  \
    $(C_SRC_DIR)/homa.hh       


SRC_C =                       \
    $(C_SRC_DIR)/databuff.cc  \
    $(C_SRC_DIR)/databuff.hh  \
    $(C_SRC_DIR)/user.cc      \
    $(C_SRC_DIR)/user.hh      \
    $(C_SRC_DIR)/map.cc       \
    $(C_SRC_DIR)/map.hh       \
    $(C_SRC_DIR)/homa.cc      \
    $(C_SRC_DIR)/homa.hh      \
    $(C_SRC_DIR)/link.cc      \
    $(C_SRC_DIR)/link.hh      \
    $(C_SRC_DIR)/peer.cc      \
    $(C_SRC_DIR)/peer.hh      \
    $(C_SRC_DIR)/rpcmgmt.cc   \
    $(C_SRC_DIR)/rpcmgmt.hh   \
    $(C_SRC_DIR)/srptmgmt.cc  \
    $(C_SRC_DIR)/srptmgmt.hh  \
    $(C_SRC_DIR)/stack.hh     \
    $(C_SRC_DIR)/fifo.hh     \
    $(C_SRC_DIR)/packetmap.hh \
    $(C_SRC_DIR)/packetmap.cc \
    $(C_SRC_DIR)/dma.cc \
    $(C_SRC_DIR)/dma.hh \
    $(C_SRC_DIR)/logger.cc \
    $(C_SRC_DIR)/logger.hh \

PART = xcu250-figd2104-2L-e

CSIM = 0
SYNTH = 1
COSIM = 2

############ Bitstream  Gen ############

homa: 
	$(VIVADO) -mode batch -source tcl/compile.tcl

############ Vitis C Synth ############

HOMA_SYNTH_FLAGS = $(PART) $(SYNTH) "$(SRC_C)" "$(SRC_JSON)"

homa_synth:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(HOMA_SYNTH_FLAGS) \
		$(C_TB_DIR)/single_message_tester.cc \
		homa \
		"RTT_BYTES=1000000"

INF_SYNTH_FLAGS = $(PART) $(SYNTH) "$(CSRC_INF)" "null"

inf_synth:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(INF_SYNTH_FLAGS) \
		$(C_TB_DIR)/interface.cc \
		interface

############ Vitis C Simulation ############

CSIM_FLAGS = $(PART) $(CSIM) "$(SRC_C)" "$(SRC_JSON)"

message_tester_rtt_5000_bin:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(CSIM_FLAGS) \
		tb/message_tester.cc \
		homa \
		"RTT_BYTES=1000000"
	./homa_kernel/solution/csim/build/csim.exe 

	# "RTT_BYTES=5000"

#srpt_fetch_csim_shim:
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(CSIM_FLAGS) \
#		$(C_TB_DIR)/cosim_shim_tester.cc \
#		fetch_shim \
#		"RTT_BYTES=5000"
#	./homa_kernel/solution/csim/build/csim.exe	 



#hashtable_no_cam_csim:
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(CSIM_FLAGS) \
#		tb/hashtable_no_cam.cc \
#		rpc_state \
#		"RTT_BYTES=5000"
#	./homa_kernel/solution/csim/build/csim.exe

#TODO can just make this csim.exe path to avoid recomp
#single_message_tester_rtt_5000_bin:
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(CSIM_FLAGS) \
#		$(C_TB_DIR)/single_message_tester.cc \
#		homa \
#		"RTT_BYTES=5000" \
#
#single_message_csim: single_message_tester_rtt_5000_bin
#	./homa_kernel/solution/csim/build/csim.exe -i test/garbage -l 2000
#	./homa_kernel/solution/csim/build/csim.exe -i test/garbage -l 128
#	./homa_kernel/solution/csim/build/csim.exe -i test/garbage -l 64
#	./homa_kernel/solution/csim/build/csim.exe -i test/garbage -l 1


#srpt_fetch_csim_shim:
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(CSIM_FLAGS) \
#		$(C_TB_DIR)/cosim_shim_tester.cc \
#		fetch_shim \
#		"RTT_BYTES=5000"
#	./homa_kernel/solution/csim/build/csim.exe	 

############# Vitis Cosim ############

COSIM_FLAGS = $(PART) $(COSIM) "$(SRC_C)" 

#single_message_cosim:
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(COSIM_FLAGS) $(SRC_JSON)
#		$(C_TB_DIR)/single_message_tester.cc \
#		"RTT_BYTES=5000" \

srpt_fetch_cosim_shim:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(COSIM_FLAGS) $(V_SRC_DIR)/srpt_fetch_queue.json \
		$(C_TB_DIR)/cosim_shim_tester.cc \
		fetch_shim \
		"RTT_BYTES=5000"

############ Verilog Synthesis ############ 

data_synth: srpt_data_queue.synth
grant_synth: srpt_grant_queue.synth

%.synth: ./src/%.v
	$(VIVADO) -mode tcl -source tcl/ooc_synth.tcl -tclargs $(PART) $^ $(notdir $(basename $(^))) ./xdc/clocks.xdc  $(notdir $(basename $(^)))

############ Verilog Simulation ############ 

fetch_test: srpt_fetch_queue.xsim
data_test:  srpt_data_queue.xsim
grant_test: srpt_grant_pkts.xsim

fetch_waves: srpt_fetch_queue.waves
data_waves:  srpt_data_queue.waves
grant_waves: srpt_grant_pkts.waves

%.waves: %.xsim
	$(VS) --gui $(basename $(^)).snapshot.wdb

%.xsim: %.xelab
	$(VS) $(basename $(^)).snapshot --tclbatch ./tcl/xsim_cfg.tcl

%.xelab: %.xvlog
	$(VE) -debug all -top $(basename $(^))_tb --snapshot $(basename $(^)).snapshot

%.xvlog: ./src/%.v
	$(VC) $(basename $(^)).v 

############ Verilog Linting ############ 

grant_lint: srpt_grant_queue.lint
data_lint:  srpt_data_queue.lint

%.lint: ./src/%.v
	$(LINTER) $^

clean:
	rm -f vitis_hls.log
	rm -rf homa
	rm -rf xsim.dir
	rm -f *.log
	rm -f *.jou
	rm -f *.log
	rm -f xelab*
	rm -f xvlog*
	rm -f *.wdb
	rm -rf srpt_grant_pkts
	rm -rf srpt_data_pkts
	rm *.str
	rm traces/*

