#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>

#define HOMA_MAX_MESSAGE_LENGTH 1000000 // The maximum number of bytes in a single message

#define AXI_STREAM_FIFO_ISR  0x00 // Interrupt Status Register (r/clear on w)
#define AXI_STREAM_FIFO_IER  0x04 // Interrupt Enable Register (r/w)
#define AXI_STREAM_FIFO_TDFR 0x08 // Transmit Data FIFO Reset (w)
#define AXI_STREAM_FIFO_TDFV 0x0C // Transmit Data FIFO Vacancy (r)
#define AXI_STREAM_FIFO_TDFD 0x10 // Transmit Data FIFO Write (w)
#define AXI_STREAM_FIFO_TLR  0x14 // Transmit Length Register (w)
#define AXI_STREAM_FIFO_RDFR 0x18 // Receive Data FIFO Reset (w)
#define AXI_STREAM_FIFO_RDFO 0x1C // Receive Data FIFO Occupancy (r)
#define AXI_STREAM_FIFO_RDFD 0x20 // Receive Data FIFO Read (r)
#define AXI_STREAM_FIFO_RLR  0x24 // Receive Length Register (r)
#define AXI_STREAM_FIFO_SRR  0x28 // AXI4-Stream Reset (w)
#define AXI_STREAM_FIFO_TDR  0x2C // Transmit Destination Register (w)
#define AXI_STREAM_FIFO_RDR  0x30 // Receive Destination Register (r)

#define AXIL_OFFSET 0x00000000
#define AXIF_OFFSET 0x00001000

struct msghdr_send_t {
    char saddr[16];
    char daddr[16];
    uint16_t sport;
    uint16_t dport;
    uint64_t buff_addr;
    uint32_t buff_size;
    uint64_t id;
    uint64_t cc;
}__attribute__((packed));

void * ctl_map; 
void * h2c_map;
void * c2h_map;

void * sendmsg_regs;  
void * sendmsg_write; 
void * sendmsg_read;  

void sendmsg(struct msghdr_send_t * msghdr_send_in, struct msghdr_send_t * msghdr_send_out) {
    int i;
    uint32_t rlr;
    uint32_t rdfo;

    for (i = 0; i < 16; ++i) *((uint32_t *) sendmsg_write) = *(((uint32_t *) msghdr_send_in) + i);

    printf("sendmsg TDFV: %d\n", *((uint32_t *) (sendmsg_regs + AXI_STREAM_FIFO_TDFV)));
    *((uint32_t *) (sendmsg_regs + AXI_STREAM_FIFO_TLR)) = 64;

    rdfo = *((uint32_t *) sendmsg_regs + AXI_STREAM_FIFO_RDFO);
    rlr  = *((uint32_t *) sendmsg_regs + AXI_STREAM_FIFO_RLR);

    printf("sendmsg rdfo: %x\n", rdfo);
    printf("sendmsg rlr: %x\n", rlr);

    // TODO should sanity check rlr and rdfo
    for (i = 0; i < 16; ++i) *(((uint32_t *) msghdr_send_out) + i) = *((uint32_t *) sendmsg_read);
}

void print_msghdr(struct msghdr_send_t * msghdr_send) {
    printf("sendmsg content dump:\n");
    printf("  - source address      : %.*s\n", 16, msghdr_send->saddr);
    printf("  - destination address : %.*s\n", 16, msghdr_send->saddr);
    printf("  - source port         : %u\n", (unsigned int) msghdr_send->sport);
    printf("  - dest port           : %u\n", (unsigned int) msghdr_send->dport);
    printf("  - buffer address      : %lx\n", msghdr_send->buff_addr);
    printf("  - buffer size         : %u\n", msghdr_send->buff_size);
    printf("  - rpc id              : %lu\n", msghdr_send->id);
    printf("  - completion cookie   : %lu\n", msghdr_send->cc);
}

int main() {

    int ctl_fd = open("/dev/homa_nic_ctl", O_RDWR|O_SYNC);
    int h2c_fd = open("/dev/homa_nic_h2c", O_RDWR|O_SYNC);
    int c2h_fd = open("/dev/homa_nic_c2h", O_RDWR|O_SYNC);

    if (ctl_fd < 0) {
	perror("Invalid homa_nic_ctl device\n");
    }

    if (h2c_fd < 0) {
	perror("Invalid homa_nic_h2c device\n");
    }

    if (c2h_fd < 0) {
	perror("Invalid homa_nic_c2h device\n");
    }

    ctl_map = mmap(NULL, 2000000, PROT_READ | PROT_WRITE, MAP_SHARED, ctl_fd, 0);
    h2c_map = mmap(NULL, 1 * HOMA_MAX_MESSAGE_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, h2c_fd, 0);
    c2h_map = mmap(NULL, 1 * HOMA_MAX_MESSAGE_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, c2h_fd, 0);

    if (ctl_map == NULL) {
	perror("Invalid ctl_fd mmap\n");
    }

    if (h2c_map == NULL) {
	perror("Invalid h2c_fd mmap\n");
    }

    if (c2h_map == NULL) {
	perror("Invalid c2h_fd mmap\n");
    }

    sendmsg_regs  = ctl_map + AXIL_OFFSET;
    sendmsg_write = ctl_map + AXIF_OFFSET;
    sendmsg_read  = ctl_map + AXIF_OFFSET + 0x1000;

    printf("sendmsg_regs %p\n", sendmsg_regs);
    printf("sendmsg_write %p\n", sendmsg_write);
    printf("sendmsg_read %p\n", sendmsg_read);

    struct msghdr_send_t msghdr_send_in;

    memset(msghdr_send_in.saddr, 0xF, 16);
    memset(msghdr_send_in.daddr, 0xA, 16); 
    msghdr_send_in.sport     = 0x1;
    msghdr_send_in.dport     = 0x1;
    msghdr_send_in.buff_addr = 0;
    msghdr_send_in.buff_size = 512;
    msghdr_send_in.id        = 0;
    msghdr_send_in.cc        = 0;

    struct msghdr_send_t msghdr_send_out;

    // print_msghdr(&msghdr_send_in);

    // printf("Copying pattern to h2c buffer\n");

    char pattern[4] = "\xDE\xAD\xBE\xEF";
    char clear[4] = "\x00\x00\x00\x00";

    // for (int i = 0; i < (512/4); ++i) memcpy(h2c_map + (i*4), &pattern, 4);
    for (int i = 0; i < (512/4); ++i) memcpy(c2h_map + (i*4), &clear, 4);

    time_t now = time(0);

    // *((unsigned char *) h2c_map) = (unsigned char) now;
    *((unsigned char *) h2c_map) = 1;

    //printf("H2C Message Content Start\n");

    //for (int i = 0; i < 8; ++i) {
    //	printf("Chunk %d: ", i);
    //	for (int j = 0; j < 64; ++j) printf("%02hhX", *(((unsigned char *) h2c_map) + j + (i*64)));
    //	printf("\n");
    //}

    //printf("H2C Message Content End\n");

    time_t start = time(NULL);

    for (int i = 0; i < 10; ++i) {

     	unsigned char test_pattern = *((unsigned char *) h2c_map);

	printf("H2C Message Content Start\n");
     	for (int i = 0; i < 8; ++i) {
     	    printf("Chunk %d: ", i);
     	    for (int j = 0; j < 64; ++j) printf("%02hhX", *(((unsigned char *) h2c_map) + j + (i*64)));
     	    printf("\n");
     	}

	printf("H2C Message Content End\n");

	sendmsg(&msghdr_send_in, &msghdr_send_out);

	print_msghdr(&msghdr_send_out);

	sleep(1);

	ioctl(ctl_fd, 0, NULL);

	printf("C2H Message Contents Start\n");

	for (int i = 0; i < 8; ++i) {
	    printf("Chunk %d: ", i);
	    for (int j = 0; j < 64; ++j) printf("%02hhX", *(((unsigned char *) c2h_map) + j + (i*64)));
	    printf("\n");
	}

	printf("C2H Message Contents End\n");

	// while (*((unsigned char *) c2h_map) != test_pattern);

	unsigned char new_pattern = *((unsigned char *) h2c_map)+1;

	*((unsigned char *) h2c_map) = new_pattern;
	// while (*((unsigned char *) h2c_map) != new_pattern);

    }

    time_t end = time(NULL);
    printf("Took %f seconds\n", difftime(end, start));

    // printf("C2H Message Contents Start\n");
    // printf("%.2f\n", (double)(time(NULL) - start));



    munmap(ctl_map, 0);
    munmap(h2c_map, 0);
    munmap(c2h_map, 0);

    close(ctl_fd);
    close(h2c_fd);
    close(c2h_fd);
}
