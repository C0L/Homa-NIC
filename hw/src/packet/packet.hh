#ifndef LINK_H
#define LINK_H

#include "ap_int.h"
#include "ap_axi_sdata.h"

/*
 * Homa packet types
 *
 * TODO Only DATA and GRANT implemented
 */
#define DATA     0x10

#define HDR_BLOCK_SIZE     512
#define HDR_BLOCK_SADDR    127,0
#define HDR_BLOCK_DADDR    255,127
#define HDR_BLOCK_SPORT    271,256
#define HDR_BLOCK_DPORT    287,272
#define HDR_BLOCK_RPC_ID   351,336
#define HDR_BLOCK_CC       415,352
#define HDR_BLOCK_LOCAL_ID 429,416
#define HDR_BLOCK_MSG_ADDR 461,430
#define HDR_BLOCK_LENGTH   473,462

/* Offsets within the first 64B chunk of all packets that contain the general header.
 * These offsets are relative to the second 64B
 * chunk! Not the packet as a whole.
 */
#define CHUNK_IPV6_MAC_DEST         511,464
#define CHUNK_IPV6_MAC_SRC          463,416
#define CHUNK_IPV6_ETHERTYPE        415,400
#define CHUNK_IPV6_VERSION          399,396
#define CHUNK_IPV6_TRAFFIC          395,388
#define CHUNK_IPV6_FLOW             387,368
#define CHUNK_IPV6_PAYLOAD_LEN      367,352
#define CHUNK_IPV6_NEXT_HEADER      351,344
#define CHUNK_IPV6_HOP_LIMIT        343,336
#define CHUNK_IPV6_SADDR            335,208
#define CHUNK_IPV6_DADDR            207,80
#define CHUNK_HOMA_COMMON_SPORT     79,64
#define CHUNK_HOMA_COMMON_DPORT     63,48
#define CHUNK_HOMA_COMMON_UNUSED0   47,0

/* Offsets within the second 64B chunk of all packets that contain the
 * general header data. These offsets are relative to the second 64B
 * chunk! Not the packet as a whole.
 */
#define CHUNK_HOMA_COMMON_UNUSED1   511,496
#define CHUNK_HOMA_COMMON_DOFF      495,488
#define CHUNK_HOMA_COMMON_TYPE      487,480
#define CHUNK_HOMA_COMMON_UNUSED3   479,464
#define CHUNK_HOMA_COMMON_CHECKSUM  463,448
#define CHUNK_HOMA_COMMON_UNUSED4   447,432
#define CHUNK_HOMA_COMMON_SENDER_ID 431,368

/* Offsets within the second 64B chunk of a packet where DATA header
 * information will be placed. These offsets are relative to the second
 * 64B chunk! Not the packet as a whole.
 */
#define CHUNK_HOMA_DATA_MSG_LEN     367,336
#define CHUNK_HOMA_DATA_INCOMING    335,304
#define CHUNK_HOMA_DATA_CUTOFF      303,288
#define CHUNK_HOMA_DATA_REXMIT      287,280
#define CHUNK_HOMA_DATA_PAD         279,272
#define CHUNK_HOMA_DATA_OFFSET      271,240
#define CHUNK_HOMA_DATA_SEG_LEN     239,208
#define CHUNK_HOMA_ACK_ID           207,144
#define CHUNK_HOMA_ACK_SPORT        143,128
#define CHUNK_HOMA_ACK_DPORT        127,112

// TOIDO bad duplication 
#define NUM_EGRESS_BUFFS 32  // Number of data buffers (max outgoing RPCs) TODO change name
// TODO bad duplication
#define MAX_RPCS            16384/2 // Maximum number of RPCs

/* IPv6 Header Constants
 */
#define IPV6_VERSION       6
#define IPV6_TRAFFIC       0
#define IPV6_FLOW          0xFFFF
#define IPV6_ETHERTYPE     0x86DD
#define IPV6_HOP_LIMIT     0x00
#define IPPROTO_HOMA       0xFD
#define IPV6_HEADER_LENGTH 40

/* Ethernet Header Constants
 *
 * TODO: Who is responsible for setting the MAX_DST and MAX_SRC? The
 * PHY?
 */
#define MAC_DST 0xFFFFFFFFFFFF
#define MAC_SRC 0xDEADBEEFDEAD

/* Homa Header Constants: Used for computing packet chunk offsets
 */
#define DOFF             160 // Number of 4 byte chunks in the data header (<< 2)
#define DATA_PKT_HEADER  114 // How many bytes the Homa DATA packet header takes
#define GRANT_PKT_HEADER 87  // Number of bytes in ethernet + ipv6 + common + data 
#define PREFACE_HEADER   54  // Number of bytes in ethernet + ipv6 header 
#define HOMA_DATA_HEADER 60  // Number of bytes in homa data ethernet header

#define SRPT_QUEUE_ENTRY_SIZE      104
#define SRPT_QUEUE_ENTRY_RPC_ID    15,0  // ID of this transaction
#define SRPT_QUEUE_ENTRY_DBUFF_ID  24,16 // Corresponding on chip cache
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

#define DMA_W_REQ_SIZE      640
#define DMA_W_REQ_HOST_ADDR 63,0    // Where to write to 
#define DMA_W_REQ_COOKIE    127,64  // Cookie to return to caller
#define DMA_W_REQ_DATA      639,128 // Data to write to DMA
#define DMA_W_REQ_STROBE    647,640 // How many bytes to write
typedef ap_uint<DMA_W_REQ_SIZE> dma_w_req_t;

#define RAM_CMD_SIZE  96
#define RAM_CMD_ADDR  20,0
#define RAM_CMD_LEN   84,21
#define RAM_CMD_TAG   92,85

typedef ap_axiu<RAM_CMD_SIZE, 0, 0, 0> ram_cmd_t;

#define RAM_STATUS_SIZE  16
#define RAM_STATUS_ERROR 3,0
#define RAM_STATUS_TAG   11,4

typedef ap_axiu<RAM_STATUS_SIZE, 0, 0, 0> ram_status_t;

#endif
