#ifndef DATABUFF_H
#define DATABUFF_H

#include "ap_int.h"
#include "ap_axi_sdata.h"

// #define NUM_EGRESS_BUFFS 1  // Number of data buffers (max outgoing RPCs) TODO change name
#define NUM_EGRESS_BUFFS 32  // Number of data buffers (max outgoing RPCs) TODO change name

#define SRPT_QUEUE_ENTRY_SIZE      99
#define SRPT_QUEUE_ENTRY_RPC_ID    15,0  // ID of this transaction
#define SRPT_QUEUE_ENTRY_DBUFF_ID  24,16 // Corresponding on chip cache
#define SRPT_QUEUE_ENTRY_REMAINING 45,26 // Remaining to be sent or cached
#define SRPT_QUEUE_ENTRY_DBUFFERED 65,46 // Number of bytes cached
#define SRPT_QUEUE_ENTRY_GRANTED   85,66 // Number of bytes granted
#define SRPT_QUEUE_ENTRY_PRIORITY  88,86 // Deprioritize inactive messages

typedef ap_uint<SRPT_QUEUE_ENTRY_SIZE> srpt_queue_entry_t;

/**
 * dma_r_req_t - DMA read request
 */
#define DMA_R_REQ_SIZE      640     // Number of bits to express request
#define DMA_R_REQ_HOST_ADDR 63,0    // Where to read from
#define DMA_R_REQ_COOKIE    127,64  // Cookie to return to caller
#define DMA_R_REQ_DATA      639,128 // Data read from DMA
typedef ap_uint<DMA_R_REQ_SIZE> dma_r_req_t;

#define LOG_DBUFF_NOTIF 0x80

#endif
