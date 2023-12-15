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

HOMA_SYNTH_FLAGS = $(PART) $(SYNTH) 

# TODO can generalize this
c2h_dma_synth:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(HOMA_SYNTH_FLAGS) \
		c2h_dma \
		src/dma/dma.cc 

h2c_dma_synth:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(HOMA_SYNTH_FLAGS) \
		h2c_cache \
		src/dma/dma.cc

cache_ctrl_synth:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(HOMA_SYNTH_FLAGS) \
		cache_ctrl \
		src/cache/databuff.cc 

addr_map_synth:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(HOMA_SYNTH_FLAGS) \
		addr_map \
		src/dma/dma.cc 

intf_synth:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(HOMA_SYNTH_FLAGS) \
		interface \
		src/intf/interface.cc


homa_synth:
	$(VITIS) tcl/homa_hls.tcl -tclargs \
		$(HOMA_SYNTH_FLAGS) \
		$(C_TB_DIR)/single_message_tester.cc \
		homa \
		"RTT_BYTES=1000000"


############ Verilog Synthesis ############ 

data_synth: srpt_data_queue.synth
grant_synth: srpt_grant_queue.synth

%.synth: ./src/%.v
	$(VIVADO) -mode tcl -source tcl/ooc_synth.tcl -tclargs $(PART) $^ $(notdir $(basename $(^))) ./xdc/clocks.xdc  $(notdir $(basename $(^)))

############ Verilog Simulation ############ 

fetch_test: srpt_queue_fetch_tb.xsim
send_test:  srpt_queue_send_tb.xsim
grant_test: srpt_grant_pkts.xsim

fetch_waves: srpt_fetch_queue.waves
data_waves:  srpt_queue.waves
grant_waves: srpt_grant_pkts.waves

%.waves: %.xsim
	$(VS) --gui $(basename $(^)).snapshot.wdb

%.xsim: %.xelab
	$(VS) $(basename $(^)).snapshot --tclbatch ./tcl/xsim_cfg.tcl

%.xelab: %.xvlog
	$(VE) -debug all -top $(basename $(^)) --snapshot $(basename $(^)).snapshot

%.xvlog: ./src/pq/%.v
	$(VC) $(basename $(^)).v 

############ Verilog Linting ############ 

grant_lint: srpt_grant_queue.lint
data_lint:  srpt_data_queue.lint

%.lint: ./src/pq/%.v
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

