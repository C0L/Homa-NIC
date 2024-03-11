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

#include <chrono>
#include <thread>

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

    for (int i = 0; i < (HOMA_MAX_MESSAGE_LENGTH/64)-1; ++i) {
        memset(pattern, i+2, 4);
    	for (int j = 0; j < 64/4; ++j) {
        	memcpy((((char*) h2c_msgbuff_map)) + (j*4) + (i*64), pattern, 4);
    	}
    }

    uint32_t retoff = 0; // Lte 12 bits used

    memset(msghdr_send_in.saddr, 0xF, 16);
    memset(msghdr_send_in.daddr, 0xA, 16); 
    msghdr_send_in.sport     = 0x1;
    msghdr_send_in.dport     = 0x1;
    msghdr_send_in.buff_addr = 0;
    msghdr_send_in.id        = 0;
    msghdr_send_in.cc        = 0;

    // char * poll = ((char *) c2h_metadata_map) + ((retoff) * 64) + 10;
    for (int i = 0; i < 10; ++i) {

    	char thread_name[50];
    	snprintf(thread_name, sizeof(thread_name), "main");
    	time_trace::thread_buffer thread_buffer(thread_name);

    	for (int j = 0; j < 32; ++j) {
    		// uint32_t size   = 16384; // Lte 20 bits used
    		uint32_t size   = 1024 * (i+1); // Lte 20 bits used
    		msghdr_send_in.metadata  = (size << 12) | retoff;

		volatile char * poll = ((volatile char *) c2h_msgbuff_map) + size - 1;
    		*poll = 0;

		__m512i ymm0;
		ymm0 = _mm512_load_si512(reinterpret_cast<__m512i*>(&msghdr_send_in));
		_mm_mfence();

		tt(rdtsc(), "write request", 0, 0, 0, 0);
		_mm512_store_si512(reinterpret_cast<__m512i*>(((char *) h2c_metadata_map)), ymm0);
		while(*poll == 0);
		tt(rdtsc(), "write response", 0, 0, 0, 0);
	}

	char file_name[50];
	snprintf(file_name, 50, "parse/perf_write_%d.tt\0", i);
    	time_trace::print_to_file(file_name);
    } 

    // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // printf("Data buffer in \n");
    // for (int i = 0; i < 256; ++i) {
    // 	printf("Chunk %d: ", i);
    // 	for (int j = 0; j < 64; ++j) printf("%02hhX", *(((unsigned char *) h2c_msgbuff_map) + j + (i*64)));
    // 	printf("\n");
    // }


    // printf("Data buffer out\n");
    // for (int i = 0; i < 256; ++i) {
    // 	printf("Chunk %d: ", i);
    // 	for (int j = 0; j < 64; ++j) printf("%02hhX", *(((unsigned char *) c2h_msgbuff_map) + j + (i*64)));
    // 	printf("\n");
    // }

    // printf("memcmp: %d\n", memcmp(c2h_msgbuff_map, h2c_msgbuff_map, size-1));



    munmap(h2c_metadata_map, 16384);
    munmap(c2h_metadata_map, 16384);
    munmap(h2c_msgbuff_map, 1 * HOMA_MAX_MESSAGE_LENGTH);
    munmap(c2h_msgbuff_map, 1 * HOMA_MAX_MESSAGE_LENGTH);

    close(h2c_metadata_fd);
    close(c2h_metadata_fd);
    close(h2c_msgbuff_fd);
    close(c2h_msgbuff_fd);
}

// TODO placesomewhere more organized
// void ipv6_to_str(char * str, char * s6_addr) {
//    sprintf(str, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
//                  (int)s6_addr[0],  (int)s6_addr[1],
//                  (int)s6_addr[2],  (int)s6_addr[3],
//                  (int)s6_addr[4],  (int)s6_addr[5],
//                  (int)s6_addr[6],  (int)s6_addr[7],
//                  (int)s6_addr[8],  (int)s6_addr[9],
//                  (int)s6_addr[10], (int)s6_addr[11],
//                  (int)s6_addr[12], (int)s6_addr[13],
//                  (int)s6_addr[14], (int)s6_addr[15]);
// }
// 
// void print_msghdr(struct msghdr_send_t * msghdr_send) {
//     char saddr[64];
//     char daddr[64];
// 
//     pr_alert("sendmsg content dump:\n");
// 
//     ipv6_to_str(saddr, msghdr_send->saddr);
//     ipv6_to_str(daddr, msghdr_send->daddr);
// 
//     pr_alert("  - source address      : %s\n", saddr);
//     pr_alert("  - destination address : %s\n", daddr);
//     pr_alert("  - source port         : %u\n", (unsigned int) msghdr_send->sport);
//     pr_alert("  - dest port           : %u\n", (unsigned int) msghdr_send->dport);
//     pr_alert("  - buffer address      : %llx\n", msghdr_send->buff_addr);
//     pr_alert("  - buffer size         : %u\n",  msghdr_send->metadata);
//     pr_alert("  - rpc id              : %llu\n", msghdr_send->id);
//     pr_alert("  - completion cookie   : %llu\n", msghdr_send->cc);
// }
