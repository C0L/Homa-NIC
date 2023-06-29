LINTER = verilator --lint-only +1364-2001ext+v --unroll-count 1024 
VITIS = vitis-hls
VIVADO = vivado

C_SRC_DIR = ./src
C_TB_DIR  = ./tb
V_SRC_DIR = ./src
V_TB_DIR  = ./src
TCL_DIR   = ./tcl
XDC_DIR   = ./xdc

SRC_V = \
        $(V_SRC_DIR)/srpt_grant_queue.v

TB_V = \
        $(V_SRC_DIR)/srpt_grant_queue_tb.v

SOURCES_JSON = \
        $(V_SRC_DIR)/srpt_grant_queue.json

SOURCES_C =                      \
        $(C_SRC_DIR)/cam.hh      \
        $(C_SRC_DIR)/databuff.cc \
        $(C_SRC_DIR)/databuff.hh \
        $(C_SRC_DIR)/dma.cc      \
        $(C_SRC_DIR)/dma.hh      \
        $(C_SRC_DIR)/hashmap.hh  \
        $(C_SRC_DIR)/homa.cc     \
        $(C_SRC_DIR)/homa.hh     \
        $(C_SRC_DIR)/link.cc     \
        $(C_SRC_DIR)/link.hh     \
        $(C_SRC_DIR)/net.hh      \
        $(C_SRC_DIR)/peer.cc     \
        $(C_SRC_DIR)/peer.hh     \
        $(C_SRC_DIR)/rpcmgmt.cc  \
        $(C_SRC_DIR)/rpcmgmt.hh  \
        $(C_SRC_DIR)/srptmgmt.cc \
        $(C_SRC_DIR)/srptmgmt.hh \
        $(C_SRC_DIR)/stack.hh    \
        $(C_SRC_DIR)/timer.cc    \
        $(C_SRC_DIR)/timer.hh

SOURCES_XDC = \
        $(XDC_DIR)/clocks.xdc

PART = xcvu9p-flgb2104-2-i

# .PHONY: all lint test
.PHONY: all vlint

all: vlint xsim

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

waves: xsim
	xsim --gui srpt_grant_queue_tb_snapshot.wdb

xsim: xsim_elaborate
	xsim srpt_grant_queue_tb_snapshot --tclbatch ./tcl/xsim_cfg.tcl

xsim_elaborate: xsim_compile
	xelab -debug all -top srpt_grant_queue_tb --snapshot srpt_grant_queue_tb_snapshot

xsim_compile: $(SRC_V) $(TB_V) 
	xvlog $^

vlint: $(SRC_V) $(TB_V) 
	$(LINTER) $^

clean:
	rm vitis_hls.log
	rm -r homa
	rm -rf xsim.dir
	rm *.log
	rm *.jou
	rm *.log
	rm xelab*
