synth:
	vitis_hls tcl/synth.tcl

srpt_queue_synth:
	vivado -mode tcl -source tcl/srpt_grant_synth.tcl

srpt_queue_test:
	vivado -mode tcl -source tcl/srpt_grant_test.tcl


unit_test:
	vitis_hls tcl/unit_tests.tcl

srptmgmt_test:
	vitis_hls tcl/srptmgmt_test.tcl

link_test:
	vitis_hls tcl/link_test.tcl

clean:
	rm vitis_hls.log
	rm -r homa
