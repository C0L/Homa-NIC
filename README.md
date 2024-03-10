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

## Application Programming Interface

## Kernel Module
**Only tested under Linux 4.15.**

The kernel module (`homanic.c`) is responsible for:
1) Performing PCIe and ethernet core initialization steps
2) Loading NIC configuration parameters
3) Creating character devices for user applications to open()/mmap()
4) Allocating unique IDs to identify NIC users
5) Mapping unique memory pages for MMIO and DMA access for users
5) Onboarding DMA address translations to the NIC when a device is opened
6) Cleaning up user IDs and memory mappings on close()

After loading the module, it should have instantiated four character devices:
```bash
ls /dev/homa_nic_*
/dev/homa_nic_c2h_metadata # Card-to-host metadata buffers (sendmsg/recvmsg responses)
/dev/homa_nic_c2h_msgbuff  # Card-to-host message buffers (recvmsg message data)
/dev/homa_nic_h2c_metadata # Host-to-card metadata buffers (sendmsg/recvmsg requests)
/dev/homa_nic_h2c_msgbuff  # Host-to-card message buffers (sendmsg message data)
```

To interact with the NIC, the user application can then open and mmap a character device as follows:
```
int h2c_metadata_fd = open("/dev/homa_nic_h2c_metadata", O_RDWR|O_SYNC);
char * h2c_metadata_map = (char *) mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, h2c_metadata_fd, 0);
```
The API is defined in more depth above.

### Building the Module
```
cd driver
make
```

### Loading the Module
```
sudo insmod homanic.ko
```

### Removing the Module
```
sudo rmmod homanic.ko
```

## Hardware Flow
Tested with Vivado 2023.2, sbt..., chisel...,

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

## Contact
drewes@cs.stanford.edu