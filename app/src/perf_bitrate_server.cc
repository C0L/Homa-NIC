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
    int c2h_msgbuff_fd  = open("/dev/homa_nic_c2h_msgbuff", O_RDWR|O_SYNC);

    if (c2h_msgbuff_fd < 0) {
    	perror("Invalid character device\n");
    }

    c2h_msgbuff_map  = (char *) mmap(NULL, 1 * HOMA_MAX_MESSAGE_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, c2h_msgbuff_fd, 0);

    if (c2h_msgbuff_map == NULL) {
    	perror("mmap failed\n");
    }

    memset((void *) c2h_msgbuff_map, 0, HOMA_MAX_MESSAGE_LENGTH);

    char thread_name[50];
    snprintf(thread_name, sizeof(thread_name), "main");
    time_trace::thread_buffer thread_buffer(thread_name);

    // for (int recv = 0; recv < 4; ++recv) {
    	volatile char * head_poll = ((volatile char *) c2h_msgbuff_map + 0);
    	*head_poll = 0;

    	volatile char * tail_poll = ((volatile char *) c2h_msgbuff_map) + 16384 - 128;

    	*tail_poll = 0;

    	while(*head_poll == 0);
    	tt(rdtsc(), "first", 0, 0, 0, 0);

    	while(*tail_poll == 0);
    	tt(rdtsc(), "last", 0, 0, 0, 0);
    // }

    time_trace::print_to_file("parse/perf_bitrate.tt");

    munmap(c2h_msgbuff_map, 1 * HOMA_MAX_MESSAGE_LENGTH);

    close(c2h_msgbuff_fd);
}
