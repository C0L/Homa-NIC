VIVADO = vivado

chisel: clean
	sbt run

homa: chisel
	$(VIVADO) -mode batch -source tcl/compile.tcl

clean:
	rm -f gen/*.f
	rm -f gen/*.v
	rm -f gen/*.sv
	rm -f gen/*.tcl
