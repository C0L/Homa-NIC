synth:
	vitis_hls tcl/synth.tcl

test:
	vitis_hls tcl/test.tcl

clean:
	rm vitis_hls.log
	rm -r homa
