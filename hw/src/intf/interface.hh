#ifndef INF_H
#define INF_H

#include "ap_int.h"
#include "ap_axi_sdata.h"

#define MSGHDR_RECV_SIZE    512
// typedef ap_uint<MSGHDR_RECV_SIZE> msghdr_recv_t;
// 
#define MSGHDR_SEND_SIZE    512
// typedef ap_uint<MSGHDR_SEND_SIZE> msghdr_send_t;

typedef ap_uint<512> port_to_phys_t;

#endif 
