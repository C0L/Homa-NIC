# Verilog linter (2001 version) -- unroll count needed for larege loops
LINTER = verilator -lint-only +1364-2001ext+v --unroll-count 1024 

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
        $(V_SRC_DIR)/srpt_grant_pkts.v

# TB_V = \
#         $(V_SRC_DIR)/srpt_grant_pkts.v


SRC_JSON = \
        $(V_SRC_DIR)/srpt_grant_pkts.json \
        $(V_SRC_DIR)/srpt_data_pkts.json

SRC_C =                       \
    $(C_SRC_DIR)/databuff.cc  \
    $(C_SRC_DIR)/databuff.hh  \
    $(C_SRC_DIR)/user.cc      \
    $(C_SRC_DIR)/user.hh      \
    $(C_SRC_DIR)/hashmap.hh   \
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
    $(C_SRC_DIR)/test.hh

PART = xcu250-figd2104-2L-e

CSIM = 0
SYNTH = 1
COSIM = 2

############ Bitstream  Gen ############

# homa: synth
homa: 
	$(VIVADO) -mode batch -source tcl/compile.tcl

############ Vitis C Synth ############

CSYNTH_FLAGS = $(PART) $(SYNTH) "$(SRC_C)" "$(SRC_JSON)"

synth:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(CSYNTH_FLAGS) \
		$(C_TB_DIR)/single_message_tester.cc \
		"RTT_BYTES=5000"

############ Vitis C Simulation ############

CSIM_FLAGS = $(PART) $(CSIM) "$(SRC_C)" "$(SRC_JSON)"

single_packet_msg_16384_5000_100.trace:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(CSIM_FLAGS) \
		$(C_TB_DIR)/single_message_tester.cc \
		"OFILE=single_packet_msg_16384_5000_100.trace" \
		"DMA_SIZE=16384" \
		"RTT_BYTES=5000" \
		"MSG_SIZE=128"

#multi_packet_msg_16384_5000_100.trace:
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(CSIM_FLAGS) \
#		$(C_TB_DIR)/single_message_tester.cc \
#		"OFILE=multi_packet_msg_16384_5000_100.trace" \
#		"DMA_SIZE=16384" \
#		"RTT_BYTES=5000" \
#		"MSG_SIZE=4000"
#
#csim_single_packet_msg_1_byte_grant_16384_1_1000.trace:
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(CSIM_FLAGS) \
#		$(C_TB_DIR)/single_message_tester.cc \
#		"OFILE=csim_single_packet_msg_1_byte_grant_16384_1_1000.trace" \
#		"DMA_SIZE=16384" \
#		"RTT_BYTES=1" \
#		"MSG_SIZE=1000"
#
#csim_multi_packet_require_grant_16384_1000_2500.trace:
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(CSIM_FLAGS) \
#		$(C_TB_DIR)/single_message_tester.cc \
#		"OFILE=csim_multi_packet_require_grant_16384_1000_2500.trace" \
#		"DMA_SIZE=16384" \
#		"RTT_BYTES=1000" \
#		"MSG_SIZE=2500"

############# Vitis Cosim ############

COSIM_FLAGS = $(PART) $(COSIM) "$(SRC_C)" "$(SRC_JSON)"

#cosim_single_packet_msg:
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(COSIM_FLAGS) \
#		$(C_TB_DIR)/single_message_tester.cc \
#		"OFILE=single_packet_msg_16384_5000_100.trace" \
#		"DMA_SIZE=16384" \
#		"RTT_BYTES=5000" \
#		"MSG_SIZE=128"
#
#cosim_multi_packet_msg: 
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(COSIM_FLAGS) \
#		$(C_TB_DIR)/single_message_tester.cc \
#		"OFILE=multi_packet_msg_16384_5000_100.trace" \
#		"DMA_SIZE=16384" \
#		"RTT_BYTES=5000" \
#		"MSG_SIZE=4000"
#
#cosim_single_packet_msg_1_byte_grant: csim_single_packet_msg_1_byte_grant_16384_1_1000.trace
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(COSIM_FLAGS) \
#		$(C_TB_DIR)/single_message_tester.cc \
#		"OFILE=csim_single_packet_msg_1_byte_grant_16384_1_1000.trace" \
#		"DMA_SIZE=16384" \
#		"RTT_BYTES=1" \
#		"MSG_SIZE=1000"
#
#cosim_multi_packet_require_grant: csim_multi_packet_require_grant_16384_1000_2500.trace
#	$(VITIS) tcl/homa_hls.tcl -tclargs \
#		$(COSIM_FLAGS) \
#		$(C_TB_DIR)/single_message_tester.cc \
#		"OFILE=csim_multi_packet_require_grant_16384_1000_2500.trace" \
#		"DMA_SIZE=16384" \
#		"RTT_BYTES=1000" \
#		"MSG_SIZE=2500"
#

############ Verilog Synthesis ############ 

data_synth: srpt_data_pkts.synth
grant_synth: srpt_grant_pkts.synth

%.synth: ./src/%.v
	$(VIVADO) -mode tcl -source tcl/ooc_synth.tcl -tclargs $(PART) $^ $(notdir $(basename $(^))) ./xdc/clocks.xdc  $(notdir $(basename $(^)))

############ Verilog Simulation ############ 

data_test: srpt_data_pkts.xsim
grant_test: srpt_grant_pkts.xsim

data_waves: srpt_data_pkts.waves
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

grant_lint: srpt_grant_pkts.lint
data_lint: srpt_data_pkts.lint

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

