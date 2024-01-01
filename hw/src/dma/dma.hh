#ifndef DMA_H
#define DMA_H

#include "ap_int.h"
#include "ap_axi_sdata.h"

/**
 * dma_r_req_t - DMA read request
 */
#define DMA_R_REQ_SIZE      656     // Number of bits to express request
#define DMA_R_REQ_HOST_ADDR 63,0    // Where to read from
#define DMA_R_REQ_COOKIE    127,64  // Cookie to return to caller
#define DMA_R_REQ_DATA      639,128 // Data read from DMA
#define DMA_R_REQ_BYTES     655,640 // Number bytes
typedef ap_uint<DMA_R_REQ_SIZE> dma_r_req_t;

/**
 * dma_w_req_t - DMA write request
 */
#define DMA_W_REQ_SIZE      648
#define DMA_W_REQ_HOST_ADDR 63,0    // Where to write to 
#define DMA_W_REQ_COOKIE    127,64  // Cookie to return to caller
#define DMA_W_REQ_DATA      639,128 // Data to write to DMA
#define DMA_W_REQ_STROBE    647,640 // How many bytes to write
typedef ap_uint<DMA_W_REQ_SIZE> dma_w_req_t;


// #define AM_CMD_SIZE  72
// #define AM_CMD_SADDR 47,0
// #define AM_CMD_BTT   67,48
// #define AM_CMD_TAG   71,68

//#define AM_CMD_TYPE  23,23
//#define AM_CMD_DSA   29,24
//#define AM_CMD_EOF   30,30
//#define AM_CMD_DRR   31,31

//#define AM_CMD_TAG   83,80
//#define AM_CMD_RSVD  87,84
//#define AM_CMD_CACHE 91,88
//#define AM_CMD_USER  95,92


// #define AM_CMD_SIZE  96 
// #define AM_CMD_BTT   22,0
// #define AM_CMD_TYPE  23,23
// #define AM_CMD_DSA   29,24
// #define AM_CMD_EOF   30,30
// #define AM_CMD_DRR   31,31
// #define AM_CMD_SADDR 79,32
// #define AM_CMD_TAG   83,80
// #define AM_CMD_RSVD  87,84
// #define AM_CMD_CACHE 91,88
// #define AM_CMD_USER  95,92


#define AM_CMD_SIZE       106
#define AM_CMD_PCIE_ADDR  63,0
#define AM_CMD_SEL        65,64
#define AM_CMD_RAM_ADDR   81,66
#define AM_CMD_LEN        97,82
#define AM_CMD_TAG        105,98

typedef ap_axiu<AM_CMD_SIZE, 0, 0, 0> am_cmd_t;

#define AM_STATUS_SIZE   8
#define AM_STATUS_TAG    3,0
#define AM_STATUS_INTERR 4,4
#define AM_STATUS_DECERR 5,5
#define AM_STATUS_SLVERR 6,6
#define AM_STATUS_OKAY   7,7

typedef ap_axiu<AM_STATUS_SIZE, 0, 0, 0> am_status_t;

#define SRPT_QUEUE_ENTRY_SIZE      104
#define SRPT_QUEUE_ENTRY_RPC_ID    15,0  // ID of this transaction
#define SRPT_QUEUE_ENTRY_DBUFF_ID  25,16 // Corresponding on chip cache
#define SRPT_QUEUE_ENTRY_REMAINING 45,26 // Remaining to be sent or cached
#define SRPT_QUEUE_ENTRY_DBUFFERED 65,46 // Number of bytes cached
#define SRPT_QUEUE_ENTRY_GRANTED   85,66 // Number of bytes granted
#define SRPT_QUEUE_ENTRY_PRIORITY  88,86 // Deprioritize inactive messages

typedef ap_uint<SRPT_QUEUE_ENTRY_SIZE> srpt_queue_entry_t;

/* Offsets within the sendmsg and recvmsg bitvector for sendmsg and
 * recvmsg requests that form the msghdr
 */
#define MSGHDR_SADDR        127,0   // Address of sender (sendmsg) or receiver (recvmsg) (128 bits)
#define MSGHDR_DADDR        255,127 // Address of receiver (sendmsg) or sender (recvmsg) (128 bits)
#define MSGHDR_SPORT        271,256 // Port of sender (sendmsg) or receiver (recvmsg)    (16 bits)
#define MSGHDR_DPORT        287,272 // Address of receiver (sendmsg) or sender (recvmsg) (16 bits)
#define MSGHDR_BUFF_ADDR    319,288 // Message contents DMA offset                       (32 bits)
#define MSGHDR_RETURN       331,320 // Offset in metadata space to place result          (12 bits)
#define MSGHDR_BUFF_SIZE    351,332 // Size of message in DMA space                      (20 bits)

/* Offsets within the sendmsg bitvector for the sendmsg specific information */
#define MSGHDR_SEND_ID      447,384 // RPC identifier                                    (64 bits)
#define MSGHDR_SEND_CC      511,448 // Completion Cookie                                 (64 bits)
#define MSGHDR_SEND_SIZE    512     // Rounded to nearest                                (32 bits)

/* Offsets within the recvmsg bitvector for the recvmsg specific information */
#define MSGHDR_RECV_FLAGS   383,352 // Interest list                                     (32 bits)
#define MSGHDR_RECV_ID      447,384 // RPC identifier                                    (64 bits)
#define MSGHDR_RECV_CC      511,448 // Completion Cookie				 (32 bits)
#define MSGHDR_RECV_SIZE    512     // Rounded to nearest 32 bits

/**
 * msghdr_send_t - input bitvector from the user for sendmsg requests
 * NOTE: The TLAST signal (which is created by ap_axiu) must be
 * present to integrate with certain cores that expect a TLAST signal
 * even if there is only a single beat of the stream transaction.
 */
typedef ap_uint<MSGHDR_SEND_SIZE> msghdr_send_t;

/**
 * msghdr_recv_t - input bitvector from the user for recvmsg requests
 * NOTE: The TLAST signal (which is created by ap_axiu) must be
 * present to integrate with certain cores that expect a TLAST signal
 * even if there is only a single beat of the stream transaction.
 */
typedef ap_uint<MSGHDR_RECV_SIZE> msghdr_recv_t;


#define LOG_DATA_R_REQ  0x10
#define LOG_DATA_R_RESP 0x20

#define LOG_DATA_W_REQ  0x20

#define NUM_PORTS 16384

#endif
