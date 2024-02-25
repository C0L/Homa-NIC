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

PART = xcu250-figd2104-2L-e

CSIM = 0
SYNTH = 1
COSIM = 2

############ Bitstream  Gen ############

chisel: clean
	sbt run

homa: 
	$(VIVADO) -mode batch -source tcl/compile.tcl

############ Verilog Synthesis ############ 

data_synth: srpt_data_queue.synth
grant_synth: srpt_grant_queue.synth

%.synth: ./src/%.v
	$(VIVADO) -mode tcl -source tcl/ooc_synth.tcl -tclargs $(PART) $^ $(notdir $(basename $(^))) ./xdc/clocks.xdc  $(notdir $(basename $(^)))

############ Verilog Simulation ############ 

fetch_test: srpt_queue_fetch_tb.xsim
send_test:  srpt_queue_send_tb.xsim
grant_test: srpt_grant_pkts.xsim

fetch_waves: srpt_queue_fetch_tb.waves
data_waves:  srpt_queue_send_tb.waves
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
	rm -f gen/*.sv
	rm -f gen/*.tcl
	# rm -f vitis_hls.log
	# rm -rf homa
	# rm -rf xsim.dir
	# rm -f *.log
	# rm -f *.jou
	# rm -f *.log
	# rm -f xelab*
	# rm -f xvlog*
	# rm -f *.wdb
	# rm -rf srpt_grant_pkts
	# rm -rf srpt_data_pkts
	# rm *.str
	# rm traces/*

