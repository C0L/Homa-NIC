#ifndef HOMA_H
#define HOMA_H

#include <iostream>

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_task.h"
#include "hls_stream.h"

// Configure the size of the stream/fifo depths
#define STREAM_DEPTH 16

/* Helper Macros */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Client RPC IDs are even, servers are odd 
#define IS_CLIENT(id) ((id & 1) == 0)

// Convert between representation and index
// #define SEND_RPC_ID_FROM_INDEX(a) ((a + 1 + MAX_RPCS/2) << 1)
// #define SEND_INDEX_FROM_RPC_ID(a) ((a >> 1) - 1 - MAX_RPCS/2)

#define SEND_RPC_ID_FROM_INDEX(a) ((a + 1) << 1)
#define SEND_INDEX_FROM_RPC_ID(a) ((a >> 1) - 1)

#define RECV_RPC_ID_FROM_INDEX(a) ((a + 1) << 1)
#define RECV_INDEX_FROM_RPC_ID(a) ((a >> 1) - 1)

#define PEER_ID_FROM_INDEX(a) (a + 1)
#define INDEX_FROM_PEER_ID(a) (a - 1)

// Sender->Client and Client->Sender rpc ID conversion
#define LOCALIZE_ID(sender_id) ((sender_id) ^ 1);

/* Homa Configuration */

// TODO this wall of defs needs to be organized
#define IPV6_VERSION 6
#define IPV6_TRAFFIC 0
#define IPV6_FLOW    0xFFFF
#define IPV6_ETHERTYPE 0x86DD
#define MAC_DST 0xAAAAAAAAAAAA
#define MAC_SRC 0xBBBBBBBBBBBB
#define IPPROTO_HOMA 0xFD
#define HOP_LIMIT 0x00
#define DOFF 160
#define HOMA_PAYLOAD_SIZE 1386
#define HOMA_MAX_MESSAGE_LENGTH 1000000 // Maximum Homa message size
#define IPV6_HEADER_LENGTH 40
#define HOMA_ETH_OVERHEAD 24
#define HOMA_MIN_PKT_LENGTH 26
#define HOMA_MAX_HEADER 90
#define ETHERNET_MAX_PAYLOAD 1500
#define HOMA_MAX_PRIORITIES 8
#define OVERCOMMIT_BYTES 480000
#define DATA_PKT_HEADER 114 // Number of bytes in ethernet + ipv6 + common header + data header
#define GRANT_PKT_HEADER 87 // Number of bytes in ethernet + ipv6 + common header + grant header
#define PREFACE_HEADER 54 // Number of bytes in ethernet + ipv6 header
#define HOMA_DATA_HEADER 60 // Number of bytes in homa data ethernet header
#define RTT_BYTES (ap_uint<32>) 5000

// Internal 64B chunk of data (for streams in, out, storing data internally...etc)
typedef ap_uint<512> integral_t;
		  
typedef ap_uint<8> homa_packet_type;

enum data_bytes_e {
    NO_DATA      = 0,
    ALL_DATA     = 64,
    PARTIAL_DATA = 14,
};

/* Peer Tab Configuration */
#define MAX_PEERS_LOG2 14
#define MAX_PEERS 16383
#define PEER_SUB_TABLE_SIZE 16384
#define PEER_SUB_TABLE_INDEX 14

typedef ap_uint<MAX_PEERS_LOG2> peer_id_t;

struct peer_hashpack_t {
    ap_uint<128> s6_addr;
#define PEER_HP_SIZE 4
    bool operator==(const peer_hashpack_t & other) const {
	return (s6_addr == other.s6_addr); 
    }
};

/* RPC Store Configuration */

#define MAX_RPCS_LOG2       16    // Number of bits to express an RPC
#define MAX_RPCS            9000  // TODO Maximum number of RPCs
#define RPC_SUB_TABLE_SIZE  16384 // Size of cuckoo hash sub-tables
#define RPC_SUB_TABLE_INDEX 14    // Index into sub-table entries
#define MAX_OPS             64 // TODO rename this

typedef ap_uint<MAX_RPCS_LOG2> local_id_t;

#define RPC_HP_SIZE 7 // Number of 32 bit chunks to hash for RPC table

/* Link Configuration:
 * This defines an actual axi stream type, in contrast to the internal streams
 * which are all ap_fifo types. The actual data that will be passed by this
 * stream is 512 bit chunks.
 * 
 * Need to leave side-channel signals enabled to avoid linker error?
 */
typedef ap_axiu<512, 1, 1, 1> raw_stream_t;
//struct raw_stream_t {
//    integral_t data;
//    ap_uint<1> last;
//};

/* SRPT Configuration */
#define MAX_OVERCOMMIT 8

#define SRPT_GRANT_OUT_SIZE     97
#define SRPT_GRANT_OUT_PEER_ID  13,0
#define SRPT_GRANT_OUT_RPC_ID   29,14
#define SRPT_GRANT_OUT_RECV     61,30
#define SRPT_GRANT_OUT_GRANT    93,62
#define SRPT_GRANT_OUT_PRIORITY 96,94

typedef ap_uint<SRPT_GRANT_OUT_SIZE> srpt_grant_out_t;

#define SRPT_GRANT_IN_SIZE    126
#define SRPT_GRANT_IN_PEER_ID 13,0
#define SRPT_GRANT_IN_RPC_ID  29,14
#define SRPT_GRANT_IN_OFFSET  61,30
#define SRPT_GRANT_IN_MSG_LEN 93,62
#define SRPT_GRANT_IN_INC     125,94 // TODO still needed?

typedef ap_uint<SRPT_GRANT_IN_SIZE> srpt_grant_in_t;

#define SRPT_SENDMSG_SIZE       86
#define SRPT_SENDMSG_RPC_ID     15,0
#define SRPT_SENDMSG_MSG_LEN    35,16
#define SRPT_SENDMSG_GRANTED    55,36
#define SRPT_SENDMSG_DBUFF_ID   65,56
#define SRPT_SENDMSG_DMA_OFFSET 85,66

typedef ap_uint<SRPT_SENDMSG_SIZE> srpt_sendmsg_t;

#define PKTQ_SIZE      99
#define PKTQ_RPC_ID    15,0
#define PKTQ_DBUFF_ID  24,16
#define PKTQ_REMAINING 45,26
#define PKTQ_DBUFFERED 65,46
#define PKTQ_GRANTED   85,66
#define PKTQ_PRIORITY  98,86

typedef ap_uint<PKTQ_SIZE> srpt_pktq_t;

#define SENDQ_SIZE      101
#define SENDQ_RPC_ID    15,0
#define SENDQ_DBUFF_ID  25,16
#define SENDQ_OFFSET    57,26
#define SENDQ_DBUFFERED 77,58
#define SENDQ_MSG_LEN   97,78
#define SENDQ_PRIORITY  100,98

typedef ap_uint<SENDQ_SIZE> srpt_sendq_t;

#define SRPT_DBUFF_NOTIF_SIZE   36
#define SRPT_DBUFF_NOTIF_RPC_ID 15,0
#define SRPT_DBUFF_NOTIF_OFFSET 35,16

typedef ap_uint<SRPT_DBUFF_NOTIF_SIZE> srpt_dbuff_notif_t;

#define SRPT_GRANT_NOTIF_SIZE   36
#define SRPT_GRANT_NOTIF_RPC_ID 15,0
#define SRPT_GRANT_NOTIF_OFFSET 35,16

typedef ap_uint<SRPT_GRANT_NOTIF_SIZE> srpt_grant_notif_t;

/* Packet Map */

// 1ms == 1000000ns == 200000 cycles
#define REXMIT_CUTOFF 200000

// Index into a packetmap bitmap
typedef ap_uint<10> packetmap_idx_t;

struct packetmap_t {
    packetmap_idx_t head;
    packetmap_idx_t length;
    ap_uint<64> map;
};

struct touch_t {
    local_id_t rpc_id;

    bool init;

    // Bit to set in packet map
    packetmap_idx_t offset;

    // Total length of the message
    packetmap_idx_t length;
};

struct rexmit_t {
    local_id_t rpc_id;

    // Bit to set in packet map
    packetmap_idx_t offset;
};

/* The homa core writes out packets in 64B units. As a result, we need to know
 * the local offsets for header data in the 64B chunks 
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

#define CHUNK_HOMA_COMMON_UNUSED1   511,496
#define CHUNK_HOMA_COMMON_DOFF      495,488
#define CHUNK_HOMA_COMMON_TYPE      487,480
#define CHUNK_HOMA_COMMON_UNUSED3   479,464
#define CHUNK_HOMA_COMMON_CHECKSUM  463,448
#define CHUNK_HOMA_COMMON_UNUSED4   447,432
#define CHUNK_HOMA_COMMON_SENDER_ID 431,368

/* DATA Packet */
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

/* GRANT Packet */
#define CHUNK_HOMA_GRANT_OFFSET     111,80
#define CHUNK_HOMA_GRANT_PRIORITY   79,72

/* Homa packet types */
#define DATA     0x10
#define GRANT    0x11
#define RESEND   0x12
#define UNKNOWN  0x13
#define BUSY     0x14
#define CUTOFFS  0x15
#define FREEZE   0x16
#define NEED_ACK 0x17
#define ACK      0x18

/* Data Buffer Configuration */
#define NUM_DBUFF        64  // Number of data buffers (max outgoing RPCs)
#define DBUFF_INDEX      10  // Index into the data buffers
#define DBUFF_CHUNK_SIZE 64  // Size of a "chunk" of a data buffer
#define DBUFF_NUM_CHUNKS 256 // Number of "chunks" in one buffer
#define DBUFF_CHUNK_INDEX 8  // Index into 256 chunks
#define DBUFF_BYTE_INDEX 14  // Byte index within data buffer

// Index into data buffers
typedef ap_uint<DBUFF_INDEX> dbuff_id_t;

// One data buffer stores 2^14 bytes
typedef integral_t dbuff_t[DBUFF_NUM_CHUNKS];

// Pointer to a byte in the data buffer
typedef ap_uint<DBUFF_BYTE_INDEX> dbuff_boffset_t;

// Pointer to a 64B chunk in the data buffer
typedef ap_uint<DBUFF_CHUNK_INDEX> dbuff_coffset_t;

#define DBUFF_IN_SIZE     580
#define DBUFF_IN_DATA     511,0
#define DBUFF_IN_DBUFF_ID 521,512
#define DBUFF_IN_RPC_ID   537,522
#define DBUFF_IN_OFFSET   557,538
#define DBUFF_IN_MSG_LEN  577,558
#define DBUFF_IN_LAST     578,578

typedef ap_uint<DBUFF_IN_SIZE> dbuff_in_raw_t;

struct dbuff_in_t {
    integral_t data;
    dbuff_id_t dbuff_id;
    local_id_t local_id;
    ap_uint<32> offset;
    ap_uint<32> msg_len;
    ap_uint<1> last;
};

struct in_chunk_t {
    integral_t  buff;   // Data to be written to DMA
    ap_uint<32> offset; // Byte offset of this chunk in msg
    ap_uint<1>  last;   // 1 to notify srpt_data_queue
};

struct out_chunk_t {
    homa_packet_type type;   // What is the type of this outgoing packet
    dbuff_id_t dbuff_id;     // Which data buffer is the message stored in
    local_id_t local_id;     // What is the RPC ID associated with this message
    ap_uint<32> offset;      // What byte offset is this output chunk for
    ap_uint<32> length;      // What is the total length of this message
    data_bytes_e data_bytes; // How many data bytes to add to this block
    integral_t buff;         // Data to be sent onto the link
    ap_uint<1> last;         // Is this the last chunk in the sequence
};


struct homa_rpc_t {
    ap_uint<128> saddr;    // Address of sender (sendmsg) or receiver (recvmsg)
    ap_uint<128> daddr;    // Address of receiver (sendmsg) or sender (recvmsg)
    ap_uint<16>  dport;    // Port of sender (sendmsg) or receiver (recvmsg)
    ap_uint<16>  sport;    // Port of sender (sendmsg) or receiver (recvmsg)
    ap_uint<64>  id;       // RPC ID (potentially not local)
    ap_uint<32>  iov_size; // 
    ap_uint<32>  iov;      // 
    dbuff_id_t   obuff_id; // ID for outgoing data
    dbuff_id_t   ibuff_id; // ID for incoming data
};

#define MSGHDR_SADDR        127,0   // Address of sender (sendmsg) or receiver (recvmsg)
#define MSGHDR_DADDR        255,127 // Address of receiver (sendmsg) or sender (recvmsg)
#define MSGHDR_SPORT        273,256 // Port of sender (sendmsg) or receiver (recvmsg)
#define MSGHDR_DPORT        289,274 // Address of receiver (sendmsg) or sender (recvmsg) 
#define MSGHDR_IOV          321,290 // Message contents DMA offset
#define MSGHDR_IOV_SIZE     353,322 // Size of message in DMA space 

#define MSGHDR_SEND_ID      417,354 // RPC identifier
#define MSGHDR_SEND_CC      481,418 // Completion Cookie
#define MSGHDR_SEND_SIZE    482 

#define MSGHDR_RECV_ID      417,354 // RPC identifier
#define MSGHDR_RECV_CC      481,418 // Completion Cookie
#define MSGHDR_RECV_FLAGS   513,482 // Interest list
#define MSGHDR_RECV_SIZE    514

typedef ap_uint<MSGHDR_SEND_SIZE> msghdr_send_t;
typedef ap_uint<MSGHDR_RECV_SIZE> msghdr_recv_t;

#define HOMA_RECVMSG_REQUEST       0x01
#define HOMA_RECVMSG_RESPONSE      0x02
#define HOMA_RECVMSG_NONBLOCKING   0x04
#define HOMA_RECVMSG_VALID_FLAGS   0x07

#define MAX_RECV_MATCH 1024
#define MAX_HDR_MATCH  1024

struct recv_interest_t {
    ap_uint<16>  sport; // Port of the caller
    ap_uint<32>  flags; // Interest list
    ap_uint<64>  id;    // ID of interest
};

struct onboard_send_t {
    ap_uint<128> saddr;    // Address of sender (sendmsg) or receiver (recvmsg)
    ap_uint<128> daddr;    // Address of receiver (sendmsg) or sender (recvmsg)
    ap_uint<16>  sport;    // Port of sender (sendmsg) or receiver (recvmsg) 
    ap_uint<16>  dport;    // Port of sender (sendmsg) or receiver (recvmsg)
    ap_uint<32>  iov;      // Message contents DMA offset
    ap_uint<32>  iov_size; // Size of message in DMA space 

    ap_uint<64>  id;    // RPC identifier 
    ap_uint<64>  cc;    // Completion Cookie

    ap_uint<32> dbuffered;
    ap_uint<32> granted;
    local_id_t  local_id; // Local RPC ID 
    dbuff_id_t  dbuff_id; // Data buffer ID for outgoing data
    peer_id_t   peer_id;  // Local ID for this destination address
};


/* forwarding structures */
struct header_t {
    // Local Values
    local_id_t  local_id;           // ID within RPC State
    peer_id_t   peer_id;            // ID of this peer
    dbuff_id_t  obuff_id;           // ID of buffer of data to send
    dbuff_id_t  ibuff_id;           // ID of buffer for received data
    ap_uint<64> completion_cookie;  // Cookie from the origin sendmsg
    ap_uint<32> packet_bytes;

    // IPv6 + Common Header
    ap_uint<16>      payload_length;
    ap_uint<128>     saddr;
    ap_uint<128>     daddr;
    ap_uint<16>      sport;
    ap_uint<16>      dport;
    homa_packet_type type;
    ap_uint<64>      id;

    // Data Header
    ap_uint<32> message_length; 
    ap_uint<32> incoming;
    ap_uint<16> cutoff_version;
    ap_uint<8>  retransmit;

    // Data Segment
    ap_uint<32> data_offset;
    ap_uint<32> segment_length;

    // Ack Header
    ap_uint<64> client_id;
    ap_uint<16> client_port;
    ap_uint<16> server_port;

    // Grant Header
    ap_uint<32> grant_offset;
    ap_uint<8>  priority;
};

/* DMA Configuration */

#define DMA_R_REQ_SIZE     122
#define DMA_R_REQ_OFFSET   31,0
#define DMA_R_REQ_BURST    63,32
#define DMA_R_REQ_MSG_LEN  95,64
#define DMA_R_REQ_DBUFF_ID 105,96
#define DMA_R_REQ_RPC_ID   121,106

typedef ap_uint<DMA_R_REQ_SIZE> dma_r_req_raw_t;

struct dma_r_req_t {
    ap_uint<32> offset;
    ap_uint<32> burst; // TODO?
    ap_uint<32> msg_len;
    dbuff_id_t dbuff_id;
    local_id_t local_id;
    ap_uint<1> last;
};

#define DMA_W_REQ_SIZE   544
#define DMA_W_REQ_OFFSET 31,0
#define DMA_W_REQ_BLOCK  543,32

typedef ap_uint<DMA_W_REQ_SIZE> dma_w_req_raw_t;

struct dma_w_req_t {
    integral_t data;
    ap_uint<32> offset;
};

#define MAX_INIT_CACHE_BURST 16384 // Number of initial 64 Byte chunks to cache for a message

// WARNING: For C simulation only
struct srpt_data_t {
    local_id_t rpc_id;
    dbuff_id_t dbuff_id;
    ap_uint<32> remaining;
    ap_uint<32> total;
};

// WARNING: For C simulation only
struct srpt_grant_t {
    peer_id_t peer_id;
    local_id_t rpc_id;
    ap_uint<32> recv_bytes;
    ap_uint<32> grantable_bytes;
};
void homa(hls::stream<msghdr_send_t> & msghdr_send_i,
	  hls::stream<msghdr_send_t> & msghdr_send_o,
	  hls::stream<msghdr_recv_t> & msghdr_recv_i,
	  hls::stream<msghdr_recv_t> & msghdr_recv_o,
	  ap_uint<512> * maxi_in,
	  ap_uint<512> * maxi_out,
	  hls::stream<raw_stream_t> & link_ingress,
	  hls::stream<raw_stream_t> & link_egress);


#endif
