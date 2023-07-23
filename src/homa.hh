#ifndef HOMA_H
#define HOMA_H

#include <iostream>

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_task.h"
#include "hls_stream.h"

// Configure the design in a reduced size for compilation speed
#define DEBUG

// Configure the size of the stream/fifo depths
#define STREAM_DEPTH 2

/* Helper Macros */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Client RPC IDs are even, servers are odd 
#define IS_CLIENT(id) ((id & 1) == 0)

// Sender->Client and Client->Sender rpc ID conversion
#define LOCALIZE_ID(sender_id) ((sender_id) ^ 1);

/* SRPT Configuration */
#ifndef DEBUG 
#define MAX_OVERCOMMIT 8
#else
#define MAX_OVERCOMMIT 1
#endif

#define SRPT_GRANT_OUT_SIZE     97
#define SRPT_GRANT_OUT_PEER_ID  13,0
#define SRPT_GRANT_OUT_RPC_ID   29,14
#define SRPT_GRANT_OUT_RECV     61,30
#define SRPT_GRANT_OUT_GRANT    93,62
#define SRPT_GRANT_OUT_PRIORITY 96,94
#define PRIORITY_SIZE      3

typedef ap_uint<SRPT_GRANT_OUT_SIZE> srpt_grant_out_t;

#define SRPT_GRANT_IN_SIZE    126
#define SRPT_GRANT_IN_PEER_ID 13,0
#define SRPT_GRANT_IN_RPC_ID  29,14
#define SRPT_GRANT_IN_OFFSET  61,30
#define SRPT_GRANT_IN_MSG_LEN 93,62
#define SRPT_GRANT_IN_INC     125,94

typedef ap_uint<SRPT_GRANT_IN_SIZE> srpt_grant_in_t;

#define SRPT_DATA_SIZE      123
#define SRPT_DATA_RPC_ID    15,0
#define SRPT_DATA_REMAINING 47,16
#define SRPT_DATA_GRANTED   79,48
#define SRPT_DATA_DBUFFERED 111,80
#define SRPT_DATA_DBUFF_ID  119,112
#define SRPT_DATA_PRIORITY  122,120

typedef ap_uint<SRPT_DATA_SIZE> srpt_data_in_t;
typedef ap_uint<SRPT_DATA_SIZE> srpt_data_out_t;

#define SRPT_DBUFF_NOTIF_SIZE     72
#define SRPT_DBUFF_NOTIF_DBUFF_ID 9,0
#define SRPT_DBUFF_NOTIF_MSG_LEN  41,10
#define SRPT_DBUFF_NOTIF_OFFSET   71,42

typedef ap_uint<SRPT_DBUFF_NOTIF_SIZE> srpt_dbuff_notif_t;

#define SRPT_GRANT_NOTIF_SIZE     42
#define SRPT_GRANT_NOTIF_DBUFF_ID 9,0
#define SRPT_GRANT_NOTIF_OFFSET   41,10

typedef ap_uint<SRPT_GRANT_NOTIF_SIZE> srpt_grant_notif_t;


/* Peer Tab Configuration */
#ifndef DEBUG 
#define MAX_PEERS_LOG2 14
#define MAX_PEERS 16383
#define PEER_SUB_TABLE_SIZE 16384
#define PEER_SUB_TABLE_INDEX 14
#else
#define MAX_PEERS_LOG2 3
#define MAX_PEERS 2
#define PEER_SUB_TABLE_SIZE 2
#define PEER_SUB_TABLE_INDEX 1
#endif

typedef ap_uint<MAX_PEERS_LOG2> peer_id_t;

struct peer_hashpack_t {
   ap_uint<128> s6_addr;
#define PEER_HP_SIZE 4
   bool operator==(const peer_hashpack_t & other) const {
      return (s6_addr == other.s6_addr); 
   }
};

/* RPC Store Configuration */
#ifndef DEBUG 
// RPC IDs are LSH 1 bit 
#define MAX_RPCS_LOG2 16
#define MAX_RPCS 9000 
#define RPC_SUB_TABLE_SIZE 16384
#define RPC_SUB_TABLE_INDEX 14
#define MAX_OPS 64
#else
#define MAX_RPCS_LOG2 3
#define MAX_RPCS 2
#define RPC_SUB_TABLE_SIZE 2
#define RPC_SUB_TABLE_INDEX 1
#define MAX_OPS 64
#endif

typedef ap_uint<MAX_RPCS_LOG2> rpc_id_t;

#define RPC_HP_SIZE 7


/* Link Configuration:
 * This defines an actual axi stream type, in contrast to the internal streams
 * which are all ap_fifo types. The actual data that will be passed by this
 * stream is 512 bit chunks.
 * 
 * Need to leave side-channel signals enabled to avoid linker error?
 */
typedef ap_axiu<512, 1, 0, 0> raw_stream_t;


/* Network Constants */
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

// Maximum Homa message size
#define HOMA_MAX_MESSAGE_LENGTH 1000000
#define IPV6_HEADER_LENGTH 40
#define HOMA_ETH_OVERHEAD 24
#define HOMA_MIN_PKT_LENGTH 26
#define HOMA_MAX_HEADER 90
#define ETHERNET_MAX_PAYLOAD 1500
#define HOMA_MAX_PRIORITIES 8
#define OVERCOMMIT_BYTES 480000
// Number of bytes in ethernet + ipv6 + common header + data header
#define DATA_PKT_HEADER 114
// ethernet header: 14 + ipv6 header: 40 + common header: 28 + grant header: 5
// Number of bytes in ethernet + ipv6 + common header + grant header
#define GRANT_PKT_HEADER 87
// Number of bytes in ethernet + ipv6 header
#define PREFACE_HEADER 54
// Number of bytes in homa data ethernet header
#define HOMA_DATA_HEADER 60



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

// Internal 64B chunk of data (for streams in, out, storing data internally...etc)
typedef ap_uint<512> integral_t;

/* Data Buffer Configuration */
#ifndef DEBUG 
// Number of data buffers (max outgoing RPCs)
#define NUM_DBUFF 1024 
// Index into the data buffers
#define DBUFF_INDEX 10 
#else
// Number of data buffers (max outgoing RPCs)
// #define NUM_DBUFF 1024 
#define NUM_DBUFF 2
// Index into the data buffers
// #define DBUFF_INDEX 10 
#define DBUFF_INDEX 1
#endif

// Size of a "chunk" of a data buffer
#define DBUFF_CHUNK_SIZE 64  
// Number of "chunks" in one buffer
#define DBUFF_NUM_CHUNKS 256  
// Index into 256 chunks
#define DBUFF_CHUNK_INDEX 8   
// Byte index within data buffer (NUM_DBUFF * DBUFF_CHUNK_SIZE * DBUFF_NUM_CHUNKS)
#define DBUFF_BYTE_INDEX 14

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
#define DBUFF_IN_CHUNK    529,522
#define DBUFF_IN_RPC_ID   545,530
#define DBUFF_IN_LAST     547,546
#define DBUFF_IN_MSG_LEN  579,548

typedef ap_uint<DBUFF_IN_SIZE> dbuff_in_raw_t;

typedef ap_uint<8> homa_packet_type;

/**
 * struct homa - Overall information about the Homa protocol implementation.
 */
struct homa_t {
   uint32_t dma_count;
   ap_int<32> mtu;
   ap_uint<32> rtt_bytes;
   ap_int<32> link_mbps;
   ap_int<32> num_priorities;
   ap_int<32> priority_map[HOMA_MAX_PRIORITIES];
   ap_int<32> max_sched_prio;
   ap_int<32> unsched_cutoffs[HOMA_MAX_PRIORITIES];
   ap_int<32> cutoff_version;
   ap_int<32> duty_cycle;
   ap_int<32> grant_threshold;
   ap_int<32> max_overcommit;
   ap_int<32> max_incoming;
   ap_int<32> resend_ticks;
   ap_int<32> resend_interval;
   ap_int<32> timeout_resends;
   ap_int<32> request_ack_ticks;
   ap_int<32> reap_limit;
   ap_int<32> dead_buffs_limit;
   ap_int<32> max_dead_buffs;
   ap_uint<32> timer_ticks;
   ap_int<32> flags;
};

struct homa_rpc_t {
   ap_uint<32> buffout;
   ap_uint<32> buffin;

   // TODO not sure if this will be utilized
   enum {
      RPC_OUTGOING            = 5,
      RPC_INCOMING            = 6,
      RPC_IN_SERVICE          = 8,
      RPC_DEAD                = 9
   } state;

   ap_uint<128> saddr;
   ap_uint<128> daddr;
   ap_uint<16> dport;
   ap_uint<16> sport;
   peer_id_t peer_id;
   rpc_id_t rpc_id; 
   ap_uint<64> id;
   ap_uint<64> completion_cookie;
   ap_uint<32> rtt_bytes;

   ap_int<32> length;
   dbuff_id_t dbuff_id;
};

struct sendmsg_t {

#define SENDMSG_BUFFIN 31,0
#define SENDMSG_LENGTH 63,32
#define SENDMSG_SADDR 191,64
#define SENDMSG_DADDR 319,192
#define SENDMSG_SPORT 335,320
#define SENDMSG_DPORT 351,336
#define SENDMSG_ID 415,352
#define SENDMSG_CC 479,416
#define SENDMSG_RTT 511,480
#define SENDMSG_SIZE 512

   ap_uint<32> buffin; // Offset in DMA space for input
   ap_uint<32> length; // Total length of message
   ap_uint<128> saddr; // Sender address
   ap_uint<128> daddr; // Destination address
   ap_uint<16> sport; // Sender port
   ap_uint<16> dport; // Destination port
   ap_uint<64> id; // RPC specified by caller
   ap_uint<64> completion_cookie;

   // Configuration
   ap_uint<32> rtt_bytes;

   // Internal use
   ap_uint<32> granted;
   ap_uint<32> dbuffered;
   rpc_id_t local_id; // Local RPC ID 
   dbuff_id_t dbuff_id; // Data buffer ID for outgoing data
   peer_id_t peer_id; // Local ID for this destination address
};

typedef ap_uint<SENDMSG_SIZE> sendmsg_raw_t;

struct recvmsg_t {

#define RECVMSG_BUFFOUT 31,0
#define RECVMSG_SADDR 159,32
#define RECVMSG_DADDR 287,160
#define RECVMSG_SPORT 303,288
#define RECVMSG_DPORT 319,304
#define RECVMSG_ID 383,320
#define RECVMSG_RTT 415,384
#define RECVMSG_SIZE 416

   // Message parameters
   ap_uint<32> buffout; // Offset in DMA space for output
   ap_uint<128> saddr; // Sender address
   ap_uint<128> daddr; // Destination address
   ap_uint<16> sport; // Sender port
   ap_uint<16> dport; // Destination port
   ap_uint<64> id; // RPC specified by caller

   // Configuration
   ap_uint<32> rtt_bytes; // TODO?

   // Internal use
   rpc_id_t local_id; // Local RPC ID 
   peer_id_t peer_id; // Local ID for this peer
};

typedef ap_uint<RECVMSG_SIZE> recvmsg_raw_t;

/* forwarding structures */
struct header_t {
   // Local Values
   rpc_id_t local_id;
   peer_id_t peer_id;
   dbuff_id_t dbuff_id;
   ap_uint<32> dma_offset;
   ap_uint<32> processed_bytes;
   ap_uint<32> packet_bytes;
   ap_uint<32> rtt_bytes;

   // IPv6 + Common Header
   ap_uint<16> payload_length;
   ap_uint<128> saddr;
   ap_uint<128> daddr;
   ap_uint<16> sport;
   ap_uint<16> dport;
   homa_packet_type type;
   ap_uint<64> id;

   // Data Header
   ap_uint<32> message_length; 
   ap_uint<32> incoming;
   ap_uint<16> cutoff_version;
   ap_uint<8> retransmit;

   // Data Segment
   ap_uint<32> data_offset;
   ap_uint<32> segment_length;

   // Ack Header
   ap_uint<64> client_id;
   ap_uint<16> client_port;
   ap_uint<16> server_port;

   // Grant Header
   ap_uint<32> grant_offset;
   ap_uint<8> priority;

   ap_uint<1> valid;
};

enum data_bytes_e {
   NO_DATA      = 0,
   ALL_DATA     = 64,
   PARTIAL_DATA = 14,
};

#define DMA_R_REQ_SIZE     122
#define DMA_R_REQ_OFFSET   31,0
#define DMA_R_REQ_BURST    63,32
#define DMA_R_REQ_MSG_LEN  95,64
#define DMA_R_REQ_DBUFF_ID 105,96
#define DMA_R_REQ_RPC_ID   121,106

typedef ap_uint<DMA_R_REQ_SIZE> dma_r_req_raw_t;

#define DMA_W_REQ_SIZE   544
#define DMA_W_REQ_OFFSET 31,0
#define DMA_W_REQ_BLOCK  543,32

typedef ap_uint<DMA_W_REQ_SIZE> dma_w_req_raw_t;


// WARNING: For C simulation only
struct srpt_data_t {
   rpc_id_t rpc_id;
   dbuff_id_t dbuff_id;
   ap_uint<32> remaining;
   ap_uint<32> total;
};

// WARNING: For C simulation only
struct srpt_grant_t {
   peer_id_t peer_id;
   rpc_id_t rpc_id;
   ap_uint<32> recv_bytes;
   ap_uint<32> grantable_bytes;
};

struct in_chunk_t {
   integral_t buff;
   ap_uint<32> offset;
   ap_uint<1> last;
};

struct out_chunk_t {
   homa_packet_type type;
   dbuff_id_t dbuff_id;     // Which data buffer is the RPC message stored in
   ap_uint<32> offset;      // Offset in data message
   data_bytes_e data_bytes; // How many data bytes to add to this block
   ap_uint<1> last;
   integral_t buff;
};

extern "C"{
   void homa(hls::stream<sendmsg_raw_t> & sendmsg_i,
         hls::stream<recvmsg_raw_t> & recvmsg_i,
         hls::stream<dma_r_req_raw_t> & dma_r_req_o,
         hls::stream<dbuff_in_raw_t> & dma_r_resp_i,
         hls::stream<dma_w_req_raw_t> & dma_w_req_o,
         hls::stream<raw_stream_t> & link_ingress,
         hls::stream<raw_stream_t> & link_egress);
}
#endif
