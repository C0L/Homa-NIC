#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

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

int main() {

    struct msghdr_send_t msghdr_send;

    memset(msghdr_send.saddr, 0xF, 16);
    memset(msghdr_send.daddr, 0xA, 16); 
    msghdr_send.sport     = 0x1;
    msghdr_send.dport     = 0x1;
    msghdr_send.buff_addr = 0;
    msghdr_send.buff_size = 512;
    msghdr_send.id        = 0;
    msghdr_send.cc        = 0;

    int fd = open("/dev/homasend", O_RDWR);

    if (fd < 0) {
	perror("No homa module\n");
    }

    // Write 64 bytes to the kernel module
    write(fd, &msghdr_send, 64);

    close(fd);
}
