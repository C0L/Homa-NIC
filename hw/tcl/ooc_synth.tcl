set argc [llength $argv]
if { $argc != 5 } {
    set errmsg "Usage: <part> <verilog src> <top> <constraints> <output dir>"
    puts $errmsg
    return 1
}

set part [lindex $argv 0] 
set v_src [lindex $argv 1] 
set top [lindex $argv 2] 
set constraints [lindex $argv 3] 
set outputDir [lindex $argv 4] 

file mkdir $outputDir

read_verilog $v_src
read_xdc $constraints

synth_design -top $top -part $part
opt_design
write_checkpoint -force $outputDir/post_synth.dcp
report_utilization -file $outputDir/post_synth_util.rpt
report_timing_summary -file $outputDir/post_synth_timing.rpt
