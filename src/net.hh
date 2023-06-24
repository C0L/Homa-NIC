#ifndef NET_H
#define NET_H

#include "ap_int.h"
#include <stdint.h>

//#define IPPROTO_HOMA 0xFD

#define INADDR_ANY ((unsigned long int) 0x00000000)

// Number of bytes in ethernet + ipv6 + data header
#define DATA_PKT_HEADER 114

// Number of bytes in ethernet + ipv6 header
#define PREFACE_HEADER 54

// Number of bytes in homa data ethernet header
#define HOMA_DATA_HEADER 60

// Maximum number of bytes in a homa DATA payload
//#define HOMA_PAYLOAD_SIZE 1386

//void chunk_byte_swap(ap_uint<512> & in_i, ap_uint<512> & out_o) {
//  for (int i = 0; i < 64; ++i) {
//#pragma HLS unroll
//    out_o(512 - (i*8) - 1, 512 - 8 - (i*8)) = in_i(7 + (i*8), (i*8));
//  }
//}



#endif
