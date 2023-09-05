#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
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
#define AXI_STREAM_FIFO_SIZE 0x7C // Number of bytes for AXIL Interface

#define SENDMSG_AXIL 0x00106000
#define SENDMSG_AXIF 0x00104000

#define RECVMSG_AXIL 0x00101000
#define RECVMSG_AXIF 0x00102000

#define BAR_0 0xfe800000

//struct msghdr_send_t {
//    char[16] saddr;
//    char[16] daddr;
//    uint16_t dport;
//    uint16_t sport;
//    uint64_t id;
//    uint32_t iov;
//};


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
void sendmsg() {
//  Write data to AXI
    
}

int main() {
    int fd = open("/dev/mem", O_SYNC);
    
    sendmsg_axil = mmap(NULL, AXI_STREAM_FIFO_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + SENDMSG_AXIL);

    if (sendmsg_axil == MAP_FAILED) {
	perror("Can't mmap AXIL sendmsg FIFO. Are you root?");
	abort();
    }

    sendmsg_axif = mmap(NULL, AXI_STREAM_FIFO_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + SENDMSG_AXIF);

    if (sendmsg_axif == MAP_FAILED) {
	perror("Can't mmap AXIF sendmsg FIFO. Are you root?");
	abort();
    }
 
    recvmsg_axil = mmap(NULL, AXI_STREAM_FIFO_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + RECVMSG_AXIL);

    if (recvmsg_axil == MAP_FAILED) {
	perror("Can't mmap AXIL recvmsg FIFO. Are you root?");
	abort();
    }

    recvmsg_axif = mmap(NULL, AXI_STREAM_FIFO_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, BAR_0 + RECVMSG_AXIF);

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

    close(fd);
}
