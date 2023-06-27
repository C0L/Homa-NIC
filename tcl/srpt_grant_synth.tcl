set outputDir ./srpt_grant_synth
file mkdir $outputDir

read_verilog ./src/srpt_grant_queue.v
read_xdc ./src/clocks.xdc

synth_design -top srpt_grant_pkts -part xcvu9p-flgb2104-2-i
opt_design
write_checkpoint -force $outputDir/post_synth.dcp
report_utilization -file $outputDir/post_synth_util.rpt
report_timing_summary -file $outputDir/post_synth_timing.rpt
