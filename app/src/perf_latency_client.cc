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

#include "time_trace.h"
#include "nic_utils.h"

struct msghdr_send_t msghdr_send_in __attribute__((aligned(64)));

char * h2c_metadata_map;
char * c2h_metadata_map;
char * h2c_msgbuff_map;
char * c2h_msgbuff_map;

int main() {
    int c2h_metadata_fd = open("/dev/homa_nic_c2h_metadata", O_RDWR|O_SYNC);
    int h2c_metadata_fd = open("/dev/homa_nic_h2c_metadata", O_RDWR|O_SYNC);
    int c2h_msgbuff_fd  = open("/dev/homa_nic_c2h_msgbuff", O_RDWR|O_SYNC);
    int h2c_msgbuff_fd  = open("/dev/homa_nic_h2c_msgbuff", O_RDWR|O_SYNC);

    if (c2h_metadata_fd < 0 || h2c_metadata_fd < 0 || c2h_msgbuff_fd < 0 || h2c_msgbuff_fd < 0) {
    	perror("Invalid character device\n");
    }

    h2c_metadata_map = (char *) mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, h2c_metadata_fd, 0);
    c2h_metadata_map = (char *) mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, c2h_metadata_fd, 0);
    h2c_msgbuff_map  = (char *) mmap(NULL, 1 * HOMA_MAX_MESSAGE_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, h2c_msgbuff_fd, 0);
    c2h_msgbuff_map  = (char *) mmap(NULL, 1 * HOMA_MAX_MESSAGE_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, c2h_msgbuff_fd, 0);

    if (h2c_metadata_map == NULL || c2h_metadata_map == NULL || h2c_msgbuff_map == NULL || c2h_msgbuff_map == NULL) {
    	perror("mmap failed\n");
    }

    char pattern[5] = "\xDE\xAD\xBE\xEF";

    for (int i = 0; i < (4*(64/4)); ++i) memcpy((((char*) h2c_msgbuff_map)) + (i*4), &pattern, 4);
    memset((void *) c2h_msgbuff_map, 0, 512);

    uint32_t retoff = 0; // Lte 12 bits used
    // uint32_t size   = (64*8); // Lte 20 bits used
    uint32_t size   = 64; // Lte 20 bits used
    memset(msghdr_send_in.saddr, 0xF, 16);
    memset(msghdr_send_in.daddr, 0xA, 16); 
    msghdr_send_in.sport     = 0x1;
    msghdr_send_in.dport     = 0x1;
    msghdr_send_in.buff_addr = 0;
    msghdr_send_in.id        = 0;
    msghdr_send_in.cc        = 0;
    msghdr_send_in.metadata  = (size << 12) | retoff;

    char * poll = ((char *) c2h_msgbuff_map);
    *poll = 0;

    char thread_name[50];
    snprintf(thread_name, sizeof(thread_name), "main");
    time_trace::thread_buffer thread_buffer(thread_name);

    for (int i = 0; i < 1000; i++) {

	__m512i ymm0;
	ymm0 = _mm512_load_si512(reinterpret_cast<__m512i*>(&msghdr_send_in));
	_mm_mfence();

	tt(rdtsc(), "ping", 0, 0, 0, 0);
	_mm512_store_si512(reinterpret_cast<__m512i*>((char *) h2c_metadata_map), ymm0);

	while(*poll == 0);
	tt(rdtsc(), "pong", 0, 0, 0, 0);
	*poll = 0;
    }

    time_trace::print_to_file("parse/ping_pong.tt");

    munmap(h2c_metadata_map, 16384);
    munmap(c2h_metadata_map, 16384);
    munmap(h2c_msgbuff_map, 1 * HOMA_MAX_MESSAGE_LENGTH);
    munmap(c2h_msgbuff_map, 1 * HOMA_MAX_MESSAGE_LENGTH);

    close(h2c_metadata_fd);
    close(c2h_metadata_fd);
    close(h2c_msgbuff_fd);
    close(c2h_msgbuff_fd);
}
