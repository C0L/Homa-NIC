synth:
	vitis_hls tcl/synth.tcl

unit_test:
	vitis_hls tcl/unit_tests.tcl

clean:
	rm vitis_hls.log
	rm -r homa
