#ifndef HOMA_H
#define HOMA_H

#include <iostream>

// extern "C"{
#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_task.h"
#include "hls_stream.h"
//}

/* HLS Configuration */ 

/* https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/HLS-Stream-Library The
 * verification depth controls the size of the "RTL verification adapter" This
 * needs to be configured to be the maximum size of any stream encountered
 * through the execution of the testbench. This should not effect the actual
 * generation of the RTL however 
 */
#define VERIF_DEPTH 128

/* Reduces the size of some parameters for faster compilation/testing */
#define DEBUG

/* Helper Macros */

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Client RPC IDs are even, servers are odd 
#define IS_CLIENT(id) ((id & 1) == 0)

// Sender->Client and Client->Sender rpc ID conversion
#define LOCALIZE_ID(sender_id) ((sender_id) ^ 1);



/* SRPT Configuration */

#ifndef DEBUG 
#define MAX_SRPT 1024
#else
#define MAX_SRPT 8
#endif

#ifndef DEBUG 
#define MAX_OVERCOMMIT 8
#else
#define MAX_OVERCOMMIT 1
#endif

#define SRPT_UPDATE 0
#define SRPT_UPDATE_BLOCK 1
#define SRPT_INVALIDATE 2
#define SRPT_UNBLOCK 3
#define SRPT_EMPTY 4
#define SRPT_BLOCKED 5
#define SRPT_ACTIVE 6

#define ENTRY_SIZE 51
#define PEER_ID 50,37
#define RPC_ID 36,23
#define RECV_PKTS 22,13
#define GRNTBLE_PKTS 12,3
#define PRIORITY 2,0
#define PRIORITY_SIZE 3

// Bit indexes for input "headers" to srpt_grant
#define HEADER_SIZE 58
#define HDR_PEER_ID 57,44
#define HDR_RPC_ID 43,30
#define HDR_MSG_LEN 29,20
#define HDR_INCOMING 19,10
#define HDR_OFFSET 9,0


/* Peer Tab Configuration */
#define MAX_PEERS_LOG2 14
typedef ap_uint<MAX_PEERS_LOG2> peer_id_t;


/* RPC Store Configuration */
// To index all the RPCs, we need LOG2 of the max number of RPCs
#define MAX_RPCS_LOG2 14
typedef ap_uint<MAX_RPCS_LOG2> rpc_id_t;

/* Link Configuration */
/* 
 * This defines an actual axi stream type, in contrast to the internal streams
 * which are all ap_fifo types. The actual data that will be passed by this
 * stream is 512 bit chunks.
 * 
 * Need to leave side-channel signals enabled to avoid linker error
 */
typedef ap_axiu<512, 1, 1, 1> raw_stream_t;

//typedef struct {
//   ap_uint<512> data; 
//   ap_uint<1> last;
//} raw_stream_t;

/* Homa Kernel Configuration */

// TODO organize this and make some of this runtime configurable
// Maximum Homa message size
#define HOMA_MAX_MESSAGE_LENGTH 1000000
#define IPV6_HEADER_LENGTH 40
#define HOMA_ETH_OVERHEAD 24
#define HOMA_MIN_PKT_LENGTH 26
#define HOMA_MAX_HEADER 90
#define ETHERNET_MAX_PAYLOAD 1500
#define HOMA_MAX_PRIORITIES 8
#define RTT_PKTS 44
#define OVERCOMMIT_PKTS 352

// Number of bytes in ethernet + ipv6 + data header
#define DATA_PKT_HEADER 114

// Number of bytes in ethernet + ipv6 header
#define PREFACE_HEADER 54

// Number of bytes in homa data ethernet header
#define HOMA_DATA_HEADER 60

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

// Central internal 64B chunk of data (for streams in, out, storing data internally...etc)
struct integral_t {
   ap_uint<512> data;
};



/* Data Buffer Configuration */

// Number of data buffers (max outgoing RPCs)
#define NUM_DBUFF 1024 
// Index into the data buffers
#define DBUFF_INDEX 10 
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

struct dbuff_in_t {
#define DBUFF_IN_DATA 511,0
   integral_t block;

#define DBUFF_IN_ID 521,512
   dbuff_id_t dbuff_id;

#define DBUFF_IN_CHUNK 529,522
   dbuff_coffset_t dbuff_chunk;
};

struct dbuff_notif_t {
   dbuff_id_t dbuff_id;
   dbuff_coffset_t dbuff_chunk; 
};

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

/**
 * struct homa_message_out - Describes a message (either request or response)
 * for which this machine is the sender.
 */
struct homa_message_out_t {
   ap_int<32> length;
   dbuff_id_t dbuff_id;
   ap_int<32> unscheduled;
   ap_int<32> granted;
   //ap_uint<8> sched_priority;
   //ap_uint<64> init_cycles;
};

/* struct homa_message_in - Holds the state of a message received by
 * this machine; used for both requests and responses.
 */
struct homa_message_in_t {
   ap_int<32> total_length;
   ap_int<32> bytes_remaining;
   ap_int<32> incoming;
   ap_int<32> priority;
   bool scheduled;
   ap_uint<64> birth;
   ap_int<64> copied_out;
};

/* struct homa_rpc - One of these structures exists for each active
 * RPC. The same structure is used to manage both outgoing RPCs on
 * clients and incoming RPCs on servers.
 */
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
   ap_uint<64> completion_cookie;
   homa_message_in_t msgin;
   homa_message_out_t msgout;
};

/* */
struct sendmsg_t {
#define SENDMSG_BUFFIN 31,0
   ap_uint<32> buffin; // Offset in DMA space for input
                       //
#define SENDMSG_LENGTH 63,32
   ap_uint<32> length; // Total length of message

#define SENDMSG_SADDR 191,64
   ap_uint<128> saddr; // Sender address
#define SENDMSG_DADDR 319,192
   ap_uint<128> daddr; // Destination address

#define SENDMSG_SPORT 335,320
   ap_uint<16> sport; // Sender port
#define SENDMSG_DPORT 351,336
   ap_uint<16> dport; // Destination port

#define SENDMSG_ID 415,352
   ap_uint<64> id; // RPC specified by caller
#define SENDMSG_CC 479,416
   ap_uint<64> completion_cookie;

   // Configuration
#define SENDMSG_RTT 511,480
   ap_uint<32> rtt_bytes;

   // ap_uint<1> valid;

   // Internal use
   ap_uint<32> granted;
   rpc_id_t local_id; // Local RPC ID 
   dbuff_id_t dbuff_id; // Data buffer ID for outgoing data
   peer_id_t peer_id; // Local ID for this destination address
};

struct recvmsg_t {
#define RECVMSG_BUFFOUT 31,0
   ap_uint<32> buffout; // Offset in DMA space for output
                        
#define RECVMSG_SADDR 159,32
   ap_uint<128> saddr; // Sender address
#define RECVMSG_DADDR 287,160
   ap_uint<128> daddr; // Destination address

#define RECVMSG_SPORT 303,288
   ap_uint<16> sport; // Sender port
#define RECVMSG_DPORT 319,304
   ap_uint<16> dport; // Destination port
                      
#define RECVMSG_ID 383,320
   ap_uint<64> id; // RPC specified by caller
                   
   // Internal use
   rpc_id_t local_id; // Local RPC ID 
   peer_id_t peer_id; // Local ID for this peer
};

/* continuation structures */

// TODO switch all of these to ap_uints (big endian). Perform conversion on head.
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
   ap_uint<64> sender_id;

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

struct ready_grant_pkt_t {
   peer_id_t peer_id;
   rpc_id_t rpc_id;
   ap_uint<10> grant_increment;
};

struct ready_data_pkt_t {
   rpc_id_t rpc_id;
   dbuff_id_t dbuff_id;
   ap_uint<32> remaining;
   ap_uint<32> total;
   ap_uint<32> granted;
};

enum data_bytes_e {
   NO_DATA      = 0,
   ALL_DATA     = 64,
   PARTIAL_DATA = 14,
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

struct dma_r_req_t {
#define DMA_R_REQ_OFFSET 31,0
   ap_uint<32> offset;
#define DMA_R_REQ_LENGTH 63,32
   ap_uint<32> length;
#define DMA_R_REQ_DBUFF_ID 73,64
   dbuff_id_t dbuff_id;
};

struct dma_w_req_t {
#define DMA_W_REQ_OFFSET 31,0
   ap_uint<32> offset;
#define DMA_W_REQ_BLOCK 543,32
   integral_t block;
};



const ap_uint<16> ETHERTYPE_IPV6 = 0x86DD;
const ap_uint<48> MAC_DST = 0xAAAAAAAAAAAA;
const ap_uint<48> MAC_SRC = 0xBBBBBBBBBBBB;
const ap_uint<32> VTF = 0x600FFFFF;
const ap_uint<8> IPPROTO_HOMA = 0xFD;
const ap_uint<8> HOP_LIMIT = 0x00;
const ap_uint<8> DOFF = 10 << 4;
const ap_uint<8> DATA_TYPE = DATA;
const ap_uint<32> HOMA_PAYLOAD_SIZE = 1386;

struct srpt_data_t {
   rpc_id_t rpc_id;
   dbuff_id_t dbuff_id;
   ap_uint<32> remaining;
   ap_uint<32> total;
};

struct srpt_grant_t {
   ap_uint<14> peer_id;
   ap_uint<14> rpc_id;
   ap_uint<10> recv_pkts;
   ap_uint<10> grantable_pkts;
};

template<typename T, int FIFO_SIZE>
struct fifo_t {
   T buffer[FIFO_SIZE];

   int read_head;

   fifo_t() {
      read_head = -1;
   }

   void insert(T value) {
#pragma HLS array_partition variable=buffer type=complete

      for (int i = FIFO_SIZE-2; i >= 0; --i) {
#pragma HLS unroll
         buffer[i+1] = buffer[i];
      }

      buffer[0] = value;

      read_head++;
   }

   T remove() {
#pragma HLS array_partition variable=buffer type=complete
      T val = buffer[read_head];

      read_head--;
      return val;
   }

   void dump() {
      for (int i = 0; i < 64; ++i) {
         std::cerr << buffer[i].offset << std::endl;
      }
   }

   T & head() {
      return buffer[read_head];
   }

   bool full() {
      return read_head == FIFO_SIZE-1;
   }

   int size() {
      return read_head;
   }

   bool empty() {
      return read_head == -1;
   }
};

extern "C"{
void homa(hls::stream<ap_axiu<512,1,1,1>> & sendmsg_i,
         hls::stream<ap_axiu<384,1,1,1>> & recvmsg_i,
         hls::stream<ap_axiu<80,1,1,1>> & dma_r_req_o,
         hls::stream<ap_axiu<536,1,1,1>> & dma_r_resp_i,
         hls::stream<ap_axiu<544,1,1,1>> & dma_w_req_o,
         hls::stream<raw_stream_t> & link_ingress,
         hls::stream<raw_stream_t> & link_egress);

//void homa(hls::stream<sendmsg_t> & sendmsg_i,
//      hls::stream<recvmsg_t> & recvmsg_i,
//      hls::stream<dma_r_req_t> & dma_r_req_o,
//      hls::stream<dbuff_in_t> & dma_r_resp_i,
//      hls::stream<dma_w_req_t> & dma_w_req_o,
//      hls::stream<raw_stream_t> & link_ingress,
//      hls::stream<raw_stream_t> & link_egress);
}
#endif
