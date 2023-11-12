#ifndef NICUTILS_H
#define NICUTILS_H

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


/*
void ipv6_to_str(char * str, char * addr) {
   sprintf(str, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                 (int)addr[0],  (int)addr[1],
                 (int)addr[2],  (int)addr[3],
                 (int)addr[4],  (int)addr[5],
                 (int)addr[6],  (int)addr[7],
                 (int)addr[8],  (int)addr[9],
                 (int)addr[10], (int)addr[11],
                 (int)addr[12], (int)addr[13],
                 (int)addr[14], (int)addr[15]);
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
    printf("  - buffer address      : %ux\n", msghdr_send->buff_addr);
    printf("  - buffer size         : %u\n",  msghdr_send->metadata);
    printf("  - rpc id              : %lu\n", msghdr_send->id);
    printf("  - completion cookie   : %lu\n", msghdr_send->cc);
}
*/

#endif
