VIVADO = vivado

chisel: clean
	sbt run

homa: chisel
	# $(VIVADO) -mode batch -source tcl/compile.tcl
	$(VIVADO) -stack 10000 -mode batch -source tcl/compile.tcl

clean:
	rm -f *.log
	rm -f *.jou
	rm -f gen/*.f
	rm -f gen/*.v
	rm -f gen/*.sv
	rm -f gen/*.tcl
