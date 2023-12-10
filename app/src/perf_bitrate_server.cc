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

    memset((void *) c2h_msgbuff_map, 0, HOMA_MAX_MESSAGE_LENGTH);

    char thread_name[50];
    snprintf(thread_name, sizeof(thread_name), "main");
    time_trace::thread_buffer thread_buffer(thread_name);

    char * head_poll = ((char *) c2h_msgbuff_map);
    *head_poll = 0;

    char * tail_poll = ((char *) c2h_msgbuff_map) + HOMA_MAX_MESSAGE_LENGTH;

    while(*head_poll == 0);
    tt(rdtsc(), "first", 0, 0, 0, 0);

    while(*tail_poll == 0);
    tt(rdtsc(), "last", 0, 0, 0, 0);

    time_trace::print_to_file("parse/perf_bitrate.tt");

    munmap(h2c_metadata_map, 16384);
    munmap(c2h_metadata_map, 16384);
    munmap(h2c_msgbuff_map, 1 * HOMA_MAX_MESSAGE_LENGTH);
    munmap(c2h_msgbuff_map, 1 * HOMA_MAX_MESSAGE_LENGTH);

    close(h2c_metadata_fd);
    close(c2h_metadata_fd);
    close(h2c_msgbuff_fd);
    close(c2h_msgbuff_fd);
}
