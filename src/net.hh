#ifndef NET_H
#define NET_H

#include "ap_int.h"
#include <stdint.h>
 
#define ETHERTYPE_IPV6 0x86DD
#define IPPROTO_HOMA 0xFD

#define INADDR_ANY ((unsigned long int) 0x00000000)

// Number of bytes in ethernet + ipv6 + data header
#define DATA_PKT_HEADER 114

// Number of bytes in ethernet + ipv6 header
#define PREFACE_HEADER 54

// Maximum number of bytes in a homa DATA payload
#define HOMA_PAYLOAD_SIZE 1386

#endif
