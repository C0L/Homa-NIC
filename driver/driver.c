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

#define SENDMSG_AXIL 0x00101000
#define SENDMSG_AXIF 0x00104000

#define RECVMSG_AXIL 0x00106000
#define RECVMSG_AXIF 0x00102000

#define BAR_0 0xfe800000

struct msghdr_send_t {
    char saddr[16];
    char daddr[16];
    uint16_t sport;
    uint16_t dport;
    uint32_t iov;
    uint32_t iov_size;
    uint32_t id;
    uint64_t cc;
}__attribute__((packed));


char * sendmsg_axil;
char * sendmsg_axif;
char * recvmsg_axil;
char * recvmsg_axif;

void dump_axi_fifo_state(char * fifo_axil) {
    printf("Interrupt Status Register   : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_ISR)));
    printf("Interrupt Enable Register   : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_IER)));
    printf("Transmit Data FIFO Vacancy  : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_TDFV)));
    printf("Transmit Length Register    : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_TLR)));
    printf("Receive Data FIFO Occupancy : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_RDFO)));
    printf("Receive Length Register     : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_RLR)));
    printf("Interrupt Status Register   : %x\n", *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_RDR)));
}

void axil_reset(char * fifo_axil) {
    *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_TDFR)) = 0x000000A6;
    *((unsigned int*) (fifo_axil + AXI_STREAM_FIFO_RDFR)) = 0x000000A6;
}

/* Programming Sequence Using Direct Register Read/Write
 * https://docs.xilinx.com/v/u/4.1-English/pg080-axi-fifo-mm-s
 */
void sendmsg(struct msghdr_send_t * msghdr_send) {
    *((unsigned int*) (sendmsg_axil + AXI_STREAM_FIFO_IER)) = 0x0C000000;
    printf("%ld\n", sizeof(struct msghdr_send_t));

    *((unsigned int*) sendmsg_axif) = 0xDEADBEEF;
    // memcpy(sendmsg_axif, msghdr_send, sizeof(struct msghdr_send_t));
    // *((unsigned int*) (sendmsg_axil + AXI_STREAM_FIFO_TLR)) = sizeof(struct msghdr_send_t);
}

int main() {
    int fd = open("/dev/mem", O_SYNC);
    
    sendmsg_axil = mmap(NULL, AXI_STREAM_FIFO_AXIL_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + SENDMSG_AXIL);

    if (sendmsg_axil == MAP_FAILED) {
	perror("Can't mmap AXIL sendmsg FIFO. Are you root?");
	abort();
    }

    sendmsg_axif = mmap(NULL, AXI_STREAM_FIFO_AXIF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + SENDMSG_AXIF);

    if (sendmsg_axif == MAP_FAILED) {
	perror("Can't mmap AXIF sendmsg FIFO. Are you root?");
	abort();
    }
 
    recvmsg_axil = mmap(NULL, AXI_STREAM_FIFO_AXIL_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + RECVMSG_AXIL);

    if (recvmsg_axil == MAP_FAILED) {
	perror("Can't mmap AXIL recvmsg FIFO. Are you root?");
	abort();
    }

    recvmsg_axif = mmap(NULL, AXI_STREAM_FIFO_AXIF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + RECVMSG_AXIF);

    if (recvmsg_axif == MAP_FAILED) {
	perror("Can't mmap AXIF recvmsg FIFO. Are you root?");
	abort();
    }

    axil_reset(sendmsg_axil);
    axil_reset(recvmsg_axil);

    printf("Sendmsg FIFO AXIL State\n");
    dump_axi_fifo_state(sendmsg_axil);

    printf("Recvmsg FIFO AXIL State\n");
    dump_axi_fifo_state(recvmsg_axil);

    struct msghdr_send_t msghdr_send;

    // TODO should be 128 bits
    memset(msghdr_send.saddr, 0xDEADBEEF, 16);
    memset(msghdr_send.daddr, 0xBEEFDEAD, 16); 
    msghdr_send.sport    = 0xFFFF;
    msghdr_send.dport    = 0xAAAA;
    msghdr_send.iov      = 0;
    msghdr_send.iov_size = 128;
    msghdr_send.id       = 0;
    msghdr_send.cc       = 0;

    sendmsg(&msghdr_send);

    printf("Sendmsg FIFO AXIL State\n");
    dump_axi_fifo_state(sendmsg_axil);

    printf("Recvmsg FIFO AXIL State\n");
    dump_axi_fifo_state(recvmsg_axil);

    close(fd);
}
