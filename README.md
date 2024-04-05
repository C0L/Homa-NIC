# Homa-NIC
This repository contains a work in progress implementation of the Homa transport protocol in hardware. It is intended to drive a 100Gbps even under small message workloads. Encapsulation, flow control, scheduling, are moved entirely into hardware. Users communicate with the NIC in terms of messages and no visibility into the underlying packets is granted. All hardware communication bypasses the kernel, with `sendmsg` and `recvmsg` requests implemented with MMIO writes, and message data transfer with DMA. A Linux kernel module character device configures these memory regions for MMIO and DMA at user application startup.

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

Each user applications are allocated four memory regions, which are used to carry out all critical path interactions with the network interface card. At a high level these are the following:
1) Host-to-Card Metadata: User applications initiating sendmsg and recvmsg requests
2) Card-to-Host Metadata: The return result from a sendmsg or recvmsg request
3) Host-to-Card Messages: Buffer of message data a user sends onto the network via a sendmsg request
4) Card-to-Host Messages: Buffer for message data a user receives from the network via a recvmsg request.

The steps to access these memory regions are discuessed within the kernel module and application section. We deal here only with the application interface once the channel has been opened.

### Host-to-Card and Card-to-Host Metadata

Sendmsg and recvmsg requests are encoded in the following 64 byte structure. 
```C
struct msghdr_t {
    uint32_t unused; 
    char saddr[16]; // Source address
    char daddr[16]; // Destination address
    uint16_t sport; // Source port
    uint16_t dport; // Desintation port
    uint32_t buff_addr; // Offset of message buffer for send/recv
    uint32_t metadata;  // TODO (size << 12) | retoff   
    uint64_t id; // zero for request, non-zero for reply
    uint64_t cc; // Completition cookie
}__attribute__((packed));
```
All application communication is handled via 64 byte MMIO writes of these structures to the host-to-card metadata region, and 64 byte DMA writes to the card-to-host region by the NIC itself. The host-to-card metadata region can be thought of as a function invocation, whose return result will be placed in the card-to-host metadata region.

TODO more description

### Host-to-Card and Card-to-Host Message Buffer
Currently, these are MB aligned (max Homa message size) regions of memory for message data.

TODO more description

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
```C
int h2c_metadata_fd = open("/dev/homa_nic_h2c_metadata", O_RDWR|O_SYNC);
char * h2c_metadata_map = (char *) mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, h2c_metadata_fd, 0);
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

### Internals
TODO address space layout 

## Hardware Flow
Currently the only target device is Alveo U250 FPGAs, though much of this code is general and could be retargeted with some effort.

The majorty of the Homa logic is implemented in Chisel, and Chisel is used to orchestrate the project as a whole. Verilog cores are wrapped as Chisel black boxes. Xilinx IP are represented with Chisel blackboxes that automatically generate the underlying tcl commands to create the respective module (more on this in related writings).

The result of chisel compilation (system verilog chisel output, verilog sources for black boxes, tcl scripts for Xilinx IP module instantaiton) are contained within the `gen/` folder after the build process.

The design has only been tested with Vivado 2023.2, sbt 1.9.8, Scala 2.13.12, Scala Test 3.2.16, and Chisel 7.0.0-M1.

### Building RTL/Outputs
The following steps will take the scala source, generate the system verilog result, grab the Verilog blackbox definitions, and output the tcl, all to the `gen/` folder.
```
cd hw
sbt run
```

### Testing Chisel/RTL
All tests are writen Chisel's built in peekpoketester. Even blackbox RTL cores are tested with this same system. To run the tests, execute the following: 
```
cd hw
sbt test 
```

### End-to-End Build
To generate a bitstream, execute the following:
```
cd hw
make homa
```

#### Hardware Architecture
TODO: Picture of the architecture here 


### Applications
A set of example applications are provided which utilize the NIC API to demonstrate functionality, test device robustness, and test performance. 

#### Build
```
cd app
make apps
```

#### Tests
TODO add description 

### Related Writings
https://colindrewes.com/writing/chisel_xilinx_ip.html

## Contact
drewes@cs.stanford.edu