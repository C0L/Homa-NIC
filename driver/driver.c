#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#define AXI_STREAM_FIFO_ISR  0x00 // Interrupt Status Register (r/clear on w)
#define AXI_STREAM_FIFO_IER  0x04 // Interrupt Enable Register (r/w)
#define AXI_STREAM_FIFO_TDFR 0x08 // Transmit Data FIFO Reset (w)
#define AXI_STREAM_FIFO_TDFV 0x0C // Transmit Data FIFO Vacancy (r)
#define AXI_STREAM_FIFO_TLR  0x14 // Transmit Length Register (w)
#define AXI_STREAM_FIFO_RDFR 0x18 // Receive Data FIFO Reset (w)
#define AXI_STREAM_FIFO_RDFO 0x1C // Receive Data FIFO Occupancy (r)
#define AXI_STREAM_FIFO_RLR  0x24 // Receive Length Register (r)
#define AXI_STREAM_FIFO_SRR  0x28 // AXI4-Stream Reset (w)
#define AXI_STREAM_FIFO_TDR  0x2C // Transmit Destination Register (w)
#define AXI_STREAM_FIFO_RDR  0x30 // Receive Destination Register (r)

#define AXI_STREAM_FIFO_AXIL_SIZE 0x7C  // Number of bytes for AXIL Interface
#define AXI_STREAM_FIFO_AXIF_SIZE 0x400 // Number of bytes for AXIF Interface

#define SENDMSG_AXIL_OFFSET 0x00101000
#define SENDMSG_AXIF_OFFSET 0x00104000

#define RECVMSG_AXIL_OFFSET 0x00106000
#define RECVMSG_AXIF_OFFSET 0x00102000

#define RECV_BUFF_OFFSET 0x00000000
#define SEND_BUFF_OFFSET 0x00080000

#define BAR_0 0xfe800000

struct msghdr_send_t {
    char saddr[16];
    char daddr[16];
    uint16_t sport;
    uint16_t dport;
    uint32_t buff_addr;
    uint32_t buff_size;
    uint32_t id;
    uint64_t cc;
}__attribute__((packed));


volatile char * sendmsg_axil;
volatile char * sendmsg_axif;
volatile char * recvmsg_axil;
volatile char * recvmsg_axif;

void dump_axi_fifo_state(volatile char * fifo_axil) {
    printf("Interrupt Status Register   : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_ISR)));
    printf("Interrupt Enable Register   : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_IER)));
    printf("Transmit Data FIFO Vacancy  : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_TDFV)));
    printf("Transmit Length Register    : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_TLR)));
    printf("Receive Data FIFO Occupancy : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_RDFO)));
    printf("Receive Length Register     : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_RLR)));
    printf("Interrupt Status Register   : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_RDR)));
}

void print_msghdr(struct msghdr_send_t * msghdr_send) {
    printf("sendmsg content dump:\n");
    printf("  SOURCE ADDRESS      : %.*s\n", 16, msghdr_send->saddr);
    printf("  DESTINATION ADDRESS : %.*s\n", 16, msghdr_send->saddr);
    printf("  SOURCE PORT         : %hu\n", msghdr_send->sport);
    printf("  DESTINATION PORT    : %hu\n", msghdr_send->dport);
    printf("  BUFFER ADDRESS      : %u\n",  msghdr_send->buff_addr);
    printf("  BUFFER SIZE         : %u\n",  msghdr_send->buff_size);
    printf("  RPC ID              : %u\n",  msghdr_send->id);
    printf("  COMPLETION COOKIE   : %lu\n", msghdr_send->cc);
}

/* Programming Sequence Using Direct Register Read/Write
 * https://docs.xilinx.com/v/u/4.1-English/pg080-axi-fifo-mm-s
 */
void sendmsg(struct msghdr_send_t * msghdr_send) {
    print_msghdr(msghdr_send);

    printf("SENDMSG AXIF: %p\n", sendmsg_axif);

    /*
    printf("sendmsg call: \n");
    printf("  Resetting Interrput Status Register\n");
    *((unsigned int*) (sendmsg_axil + AXI_STREAM_FIFO_ISR)) = 0xffffffff;
    printf("  Current sendmsg FIFO vacancy %d\n", *((unsigned int*) (sendmsg_axil + AXI_STREAM_FIFO_TDFV)));
    printf("  Resetting Interrput Enable Register\n");
    *((unsigned int*) (sendmsg_axil + AXI_STREAM_FIFO_IER)) = 0x0C000000;

    printf("  Writing sendmsg data\n");
    for (int i = 0; i < 16; ++i) {
	printf("WROTE CHUNK %d\n", i);
	*((unsigned int*) sendmsg_axif) = *(((unsigned int*) msghdr_send) + i);
    }

    printf("  Current sendmsg FIFO vacancy: %d\n", *((unsigned int*) (sendmsg_axil + AXI_STREAM_FIFO_TDFV)));

    printf("  Draining sendmsg FIFO\n");
    *((unsigned int*) (sendmsg_axil + AXI_STREAM_FIFO_TLR)) = 64;

    printf("  Current sendmsg FIFO vacancy: %d\n", *((unsigned int*) (sendmsg_axil + AXI_STREAM_FIFO_TDFV)));

    printf("  Interrupt Status Register: %d\n", *((unsigned int*) (sendmsg_axil + AXI_STREAM_FIFO_ISR)));

    printf("  Stalling for completed sendmsg\n");

    // TODO Could also check the status registers
    while (*(sendmsg_axil + AXI_STREAM_FIFO_RDFO) != 16);

    // TODO set read size
    printf("  Setting FIFO read size");
    *((unsigned int*) (sendmsg_axil + AXI_STREAM_FIFO_RLR)) = 16;

    printf("  Reading sendmsg data\n");
    for (int i = 0; i < 16; ++i) {
	*(((unsigned int*) msghdr_send) + i) = *((unsigned int*) sendmsg_axif);
    }

    print_msghdr(msghdr_send);
    */
}

int main() {
    int fd = open("/dev/mem", O_SYNC);
    
    sendmsg_axil = mmap(NULL, AXI_STREAM_FIFO_AXIL_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + SENDMSG_AXIL_OFFSET);

    if (sendmsg_axil == MAP_FAILED) {
	perror("Can't mmap AXIL sendmsg FIFO. Are you root?");
	abort();
    }

    sendmsg_axif = mmap(NULL, AXI_STREAM_FIFO_AXIF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + SENDMSG_AXIF_OFFSET);

    if (sendmsg_axif == MAP_FAILED) {
	perror("Can't mmap AXIF sendmsg FIFO. Are you root?");
	abort();
    }
 
    recvmsg_axil = mmap(NULL, AXI_STREAM_FIFO_AXIL_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + RECVMSG_AXIL_OFFSET);

    if (recvmsg_axil == MAP_FAILED) {
	perror("Can't mmap AXIL recvmsg FIFO. Are you root?");
	abort();
    }

    recvmsg_axif = mmap(NULL, AXI_STREAM_FIFO_AXIF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + RECVMSG_AXIF_OFFSET);

    if (recvmsg_axif == MAP_FAILED) {
	perror("Can't mmap AXIF recvmsg FIFO. Are you root?");
	abort();
    }

    struct msghdr_send_t msghdr_send;

    memset(msghdr_send.saddr, 0xDEADBEEF, 16);
    memset(msghdr_send.daddr, 0xBEEFDEAD, 16); 
    msghdr_send.sport     = 0xFFFF;
    msghdr_send.dport     = 0xAAAA;
    msghdr_send.buff_addr = 0;
    msghdr_send.buff_size = 128;
    msghdr_send.id        = 0;
    msghdr_send.cc        = 0;

    char msg[13] = "Hello World!";

    char * send_buff = mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + SEND_BUFF_OFFSET);
    char * recv_buff = mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + RECV_BUFF_OFFSET);

    memcpy(send_buff, msg, 13);

    printf("Send Buff Message Contents: %.*s\n", 13, send_buff);
    printf("Recv Buff Message Contents: %.*s\n", 13, recv_buff);

    sendmsg(&msghdr_send);

    printf("Send Buff Message Contents: %.*s\n", 13, send_buff);
    printf("Recv Buff Message Contents: %.*s\n", 13, recv_buff);

    close(fd);
}
