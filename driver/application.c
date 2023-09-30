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
    msghdr_send.dport     = 0x2;
    msghdr_send.buff_addr = 0;
    msghdr_send.buff_size = 128;
    msghdr_send.id        = 0;
    msghdr_send.cc        = 0;

    int fd = open("/dev/homasend", O_RDWR);

    if (fd < 0) {
	perror("No homa module\n");
    }

    // Write 64 bytes to the kernel module
    write(fd, &msghdr_send, 64);
    
//    sendmsg_axil = mmap(NULL, AXI_STREAM_FIFO_AXIL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BAR_0 + SENDMSG_AXIL_OFFSET);
//
//    if (sendmsg_axil == MAP_FAILED) {
//	perror("Can't mmap AXIL sendmsg FIFO. Are you root?");
//	abort();
//    }
//
//    sendmsg_axif = mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BAR_0 + SENDMSG_AXIF_OFFSET);
//
//    if (sendmsg_axif == MAP_FAILED) {
//	perror("Can't mmap AXIF sendmsg FIFO. Are you root?");
//	abort();
//    }
// 
//    recvmsg_axil = mmap(NULL, AXI_STREAM_FIFO_AXIL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BAR_0 + RECVMSG_AXIL_OFFSET);
//
//    if (recvmsg_axil == MAP_FAILED) {
//	perror("Can't mmap AXIL recvmsg FIFO. Are you root?");
//	abort();
//    }
//
//    recvmsg_axif = mmap(NULL, AXI_STREAM_FIFO_AXIL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BAR_0 + RECVMSG_AXIF_OFFSET);
//
//    if (recvmsg_axif == MAP_FAILED) {
//	perror("Can't mmap AXIF recvmsg FIFO. Are you root?");
//	abort();
//    }
	// printf("SIZEOF %d\n", sizeof(struct msghdr_send_t));

//    char msg[13] = "Hello World!";
//
    // char * msg_buff = mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BAR_0);
    // mlockall(MCL_FUTURE);
    // char * msg_buff = (char *) malloc(16384);

    // free(msg_buff);
    //// char * recv_buff = mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BAR_0 + RECV_BUFF_OFFSET);
//
//    memcpy(recv_buff, msg, 13);
//
//    printf("Send Buff Message Contents: %.*s\n", 13, send_buff);
//    printf("Recv Buff Message Contents: %.*s\n", 13, recv_buff);

//  dump_axi_fifo_state(sendmsg_axil);

    // sendmsg(&msghdr_send);

//    printf("Send Buff Message Contents: %.*s\n", 13, send_buff);
//    printf("Recv Buff Message Contents: %.*s\n", 13, recv_buff);

    close(fd);
}
