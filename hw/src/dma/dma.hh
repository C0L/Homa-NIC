#ifndef DMA_H
#define DMA_H

#include "ap_int.h"
#include "ap_axi_sdata.h"

/**
 * dma_r_req_t - DMA read request
 */
#define DMA_R_REQ_SIZE      640     // Number of bits to express request
#define DMA_R_REQ_HOST_ADDR 63,0    // Where to read from
#define DMA_R_REQ_COOKIE    127,64  // Cookie to return to caller
#define DMA_R_REQ_DATA      639,128 // Data read from DMA
typedef ap_uint<DMA_R_REQ_SIZE> dma_r_req_t;

/**
 * dma_w_req_t - DMA write request
 */
#define DMA_W_REQ_SIZE      640
#define DMA_W_REQ_HOST_ADDR 63,0    // Where to write to 
#define DMA_W_REQ_COOKIE    127,64  // Cookie to return to caller
#define DMA_W_REQ_DATA      639,128 // Data to write to DMA
#define DMA_W_REQ_STROBE    647,640 // How many bytes to write
typedef ap_uint<DMA_W_REQ_SIZE> dma_w_req_t;

#define AM_CMD_SIZE  88
#define AM_CMD_BTT   22,0
#define AM_CMD_TYPE  23,23
#define AM_CMD_DSA   29,24
#define AM_CMD_EOF   30,30
#define AM_CMD_DRR   31,31
#define AM_CMD_SADDR 79,32
#define AM_CMD_TAG   83,80
#define AM_CMD_RSVD  87,84

typedef ap_axiu<AM_CMD_SIZE, 0, 0, 0> am_cmd_t;

#define AM_STATUS_SIZE   8
#define AM_STATUS_TAG    3,0
#define AM_STATUS_INTERR 4,4
#define AM_STATUS_DECERR 5,5
#define AM_STATUS_SLVERR 6,6
#define AM_STATUS_OKAY   7,7

typedef ap_axiu<AM_STATUS_SIZE, 0, 0, 0> am_status_t;

#define LOG_DATA_R_REQ  0x10
#define LOG_DATA_R_RESP 0x20

#define LOG_DATA_W_REQ  0x20

#endif
