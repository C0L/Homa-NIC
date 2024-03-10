# Homa-NIC
This repository contains a work in progress hardware implementation of the Homa transport protocol. It is intended to drive a 100Gbps even under small message workloads. Encapsulation, flow control, scheduling, are moved entirely into hardware. User communicate with the NIC is in terms of messages and no visibility into the underlying packets is granted. All hardware communication occurs with kernel bypass, with `sendmsg` and `recvmsg` requests implemented with MMIO writes, and message data transfer with DMA. A Linux kernel module character device configures these memory regions for MMIO and DMA at user application startup.

Currently the only supported platform is the AMD Alveo U250 FPGA.

## Project Structure
```bash
.
├── app # Example user applications and performance benchmarks
│   ├── lib # Library code for timing measurement and NIC interface
│   ├── parse # Scripts to parse timing measurement output
│   └── src # Application sources
├── driver # Linux kernel driver for MMIO/DMA setup
├── firmware # Experimental code for on-chip RISCV processor  
├── hw # Hardware design sources
│   ├── gen  # Output directory for chisel project
│   ├── src  
│   │   ├── main
│   │   │   ├── resources
│   │   │   │   ├── verilog_libs # Verilog libraries 
│   │   │   │   │   ├── axi2axis # https://github.com/ZipCPU/wb2axip/
│   │   │   │   │   └── pcie     # https://github.com/alexforencich/verilog-pcie
│   │   │   │   └── verilog_src  # Verilog source code
│   │   │   └── scala # Scala/Chisel source code
│   │   └── test 
│   │       └── scala # Scala/Chisel test benches
│   ├── tcl # Vivado compilation flow
│   └── xdc # Constrants for Alveo U250
└── model # Python simulation for different priority queue strategies
```

## Software Flow
### Dependencies
- Linux Kernel 4.15
### Kernel Module
#### Build
```
cd driver
make
```
#### Install
```
sudo insmod homanic.ko
```

### Remove
```
sudo rmmod homanic.ko
```

### Applications
#### Build
```
cd app
make apps
```

#### Tests
##### Perf Testing
Stresses the NIC with 64B echo requests, 64B device register reads, 64B packet loopback.
```
make parse
```

## HW-Dev Dependencies
- Vivado 2023.2

## Contact
drewes@cs.stanford.edu