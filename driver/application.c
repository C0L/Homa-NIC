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
#include <immintrin.h>

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

#define SENDMSG_DEST     0
#define RECVMSG_DEST     1

struct msghdr_send_t {
    char saddr[16];
    char daddr[16];
    uint16_t sport;
    uint16_t dport;
    uint32_t buff_addr;
    uint32_t metadata;
    uint32_t interest;
    uint64_t id;
    uint64_t cc;
}__attribute__((packed));

struct msghdr_send_t msghdr_send_in __attribute__((aligned(64)));

void * h2c_metadata_map;
void * c2h_metadata_map;
void * h2c_msgbuff_map;
void * c2h_msgbuff_map;

void * axi_stream_regs; 
void * axi_stream_write;
void * axi_stream_read;

// TODO inline?
void iomov64B(__m256i * dst, __m256i * src) {
    __m256i ymm0;
    __m256i ymm1; 

    ymm0 = _mm256_stream_load_si256(src);
    ymm1 = _mm256_stream_load_si256(src+1);

    _mm256_store_si256(dst, ymm0);
    _mm256_store_si256(dst+1, ymm1);
    _mm_mfence();
}

void ipv6_to_str(char * str, char * s6_addr) {
   sprintf(str, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\0",
                 (int)s6_addr[0],  (int)s6_addr[1],
                 (int)s6_addr[2],  (int)s6_addr[3],
                 (int)s6_addr[4],  (int)s6_addr[5],
                 (int)s6_addr[6],  (int)s6_addr[7],
                 (int)s6_addr[8],  (int)s6_addr[9],
                 (int)s6_addr[10], (int)s6_addr[11],
                 (int)s6_addr[12], (int)s6_addr[13],
                 (int)s6_addr[14], (int)s6_addr[15]);
}

void sendmsg(struct msghdr_send_t * msghdr_send_in) {
    int i;
    uint32_t rlr;
    uint32_t rdfo;

    *((uint32_t *) (axi_stream_regs + AXI_STREAM_FIFO_TDR)) = SENDMSG_DEST;

    iomov64B(axi_stream_write, (void *) msghdr_send_in);

    printf("FIFO DEPTH %d\n", *((uint32_t*) (axi_stream_regs + AXI_STREAM_FIFO_TDFV)));

    *((uint32_t *) (axi_stream_regs + AXI_STREAM_FIFO_TLR)) = 64;

    printf("FIFO DEPTH %d\n", *((uint32_t*) (axi_stream_regs + AXI_STREAM_FIFO_TDFV)));
}

void print_msghdr(struct msghdr_send_t * msghdr_send) {
    printf("sendmsg content dump:\n");

    char saddr[64];
    char daddr[64];

    ipv6_to_str(saddr, msghdr_send->saddr);
    ipv6_to_str(daddr, msghdr_send->daddr);

    printf("  - source address      : %s\n", saddr);
    printf("  - destination address : %s\n", daddr);
    printf("  - source port         : %u\n", (unsigned int) msghdr_send->sport);
    printf("  - dest port           : %u\n", (unsigned int) msghdr_send->dport);
    printf("  - buffer address      : %lx\n", msghdr_send->buff_addr);
    printf("  - buffer size         : %u\n",  msghdr_send->metadata);
    printf("  - rpc id              : %lu\n", msghdr_send->id);
    printf("  - completion cookie   : %lu\n", msghdr_send->cc);
}

int main() {

    int c2h_metadata_fd = open("/dev/homa_nic_c2h_metadata", O_RDWR|O_SYNC);
    int h2c_metadata_fd = open("/dev/homa_nic_h2c_metadata", O_RDWR|O_SYNC);
    int c2h_msgbuff_fd  = open("/dev/homa_nic_c2h_msgbuff", O_RDWR|O_SYNC);
    int h2c_msgbuff_fd  = open("/dev/homa_nic_h2c_msgbuff", O_RDWR|O_SYNC);

    if (c2h_metadata_fd < 0 || h2c_metadata_fd < 0 || c2h_msgbuff_fd < 0 || h2c_msgbuff_fd < 0) {
    	perror("Invalid character device\n");
    }

    h2c_metadata_map = mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, h2c_metadata_fd, 0);
    c2h_metadata_map = mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, c2h_metadata_fd, 0);
    h2c_msgbuff_map  = mmap(NULL, 1 * HOMA_MAX_MESSAGE_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, h2c_msgbuff_fd, 0);
    c2h_msgbuff_map  = mmap(NULL, 1 * HOMA_MAX_MESSAGE_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, c2h_msgbuff_fd, 0);

    if (h2c_metadata_map == NULL || c2h_metadata_map == NULL || h2c_msgbuff_map == NULL || c2h_msgbuff_map == NULL) {
    	perror("mmap failed\n");
    }

    axi_stream_regs  = h2c_metadata_map + AXIL_OFFSET;
    axi_stream_write = h2c_metadata_map + AXIF_OFFSET;
    axi_stream_read  = h2c_metadata_map + AXIF_OFFSET + 0x1000;

    uint32_t size   = 129; // Lte 20 bits used
    uint32_t retoff = 4;   // Lte 12 bits used

    memset(msghdr_send_in.saddr, 0xF, 16);
    memset(msghdr_send_in.daddr, 0xA, 16); 
    msghdr_send_in.sport     = 0x1;
    msghdr_send_in.dport     = 0x1;
    msghdr_send_in.buff_addr = 0;
    msghdr_send_in.id        = 0;
    msghdr_send_in.cc        = 0;
	
    // msghdr_send_in.metadata  = size | (retoff << 20);

    msghdr_send_in.metadata  = (size << 12) | retoff;
    printf("%d\n", msghdr_send_in.metadata);

    char pattern[4] = "\xDE\xAD\xBE\xEF";

    for (int i = 0; i < (4*(64/4)); ++i) memcpy((((char*) h2c_msgbuff_map)) + (i*4), &pattern, 4);
    memset(c2h_msgbuff_map, 0, 512);
    memset(c2h_metadata_map, 0, 16384);

    printf("H2C Message Content Start\n");
    for (int i = 0; i < 4; ++i) {
    	printf("Chunk %d: ", i);
    	for (int j = 0; j < 64; ++j) printf("%02hhX", *(((unsigned char *) h2c_msgbuff_map) + j + (i*64)));
    	printf("\n");
    }
    printf("H2C Message Content End\n");

    printf("C2H Message Content Start\n");
    for (int i = 0; i < 4; ++i) {
    	printf("Chunk %d: ", i);
    	for (int j = 0; j < 64; ++j) printf("%02hhX", *(((unsigned char *) c2h_msgbuff_map) + j + (i*64)));
    	printf("\n");
    }
    printf("C2H Message Content End\n");

    printf("Initial Message Header\n");
    print_msghdr(&msghdr_send_in);

//     struct msghdr_send_t msghdr_send_out = *(((struct msghdr_send_t *) c2h_metadata_map) + retoff);

    struct msghdr_send_t msghdr_send_out = *(((struct msghdr_send_t *) c2h_metadata_map) + retoff);
    print_msghdr(&msghdr_send_out);

    sendmsg(&msghdr_send_in);

    usleep(1000000);

    msghdr_send_out = msghdr_send_out = *(((struct msghdr_send_t *) c2h_metadata_map) + retoff);

    printf("Completed Message Header\n");
    print_msghdr(&msghdr_send_out);

    ioctl(h2c_metadata_fd, 0, NULL);

    printf("C2H Message Contents Start\n");
    for (int i = 0; i < 4; ++i) {
    	printf("Chunk %d: ", i);
    	for (int j = 0; j < 64; ++j) printf("%02hhX", *(((unsigned char *) c2h_msgbuff_map) + j + (i*64)));
    	printf("\n");
    }
    printf("C2H Message Contents End\n");

    munmap(h2c_metadata_map, 16384);
    munmap(c2h_metadata_map, 16384);
    munmap(h2c_msgbuff_map, 1 * HOMA_MAX_MESSAGE_LENGTH);
    munmap(c2h_msgbuff_map, 1 * HOMA_MAX_MESSAGE_LENGTH);

    close(h2c_metadata_fd);
    close(c2h_metadata_fd);
    close(h2c_msgbuff_fd);
    close(c2h_msgbuff_fd);
}
