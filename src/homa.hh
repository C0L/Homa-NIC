#ifndef HOMA_H
#define HOMA_H

#include <iostream>

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_task.h"
#include "hls_stream.h"
#include "hls_vector.h"

#define Q(x) #x
#define QUOTE(x) Q(x)

#define AM_CMD_SIZE  72
#define AM_CMD_BTT   21,0
#define AM_CMD_TYPE  22,22
#define AM_CMD_DSA   28,23
#define AM_CMD_EOF   29,29
#define AM_CMD_DRR   31,30
#define AM_CMD_SADDR 63,32
#define AM_CMD_TAG   67,64
#define AM_CMD_RSVD  71,68

typedef ap_uint<AM_CMD_SIZE> am_cmd_t;

#define AM_STATUS_SIZE   8
#define AM_STATUS_TAG    3,0
#define AM_STATUS_INTERR 4,4
#define AM_STATUS_DECERR 5,5
#define AM_STATUS_SLVERR 6,6
#define AM_STATUS_OKAY   7,7

typedef ap_uint<AM_STATUS_SIZE> am_status_t;

/**
 * integral_t - The homa core operates in 64B chunks. Data in the data
 * buffer is stored in 64B entries. Chunks are sent to the link 64B at
 * a time. DMA reads and writes at 64B in size to maximize the
 * throughput.
 */
typedef ap_uint<512> integral_t;

/**
 * raw_stream_t - This defines an actual axi stream type, in contrast
 * to the internal streams which are all ap_fifo types. The actual
 * data that will be passed by this stream is 512 bit chunks.
 * 
 * TODO: Need to leave side-channel signals enabled to avoid linker error?
 */
#ifdef COSIM
struct raw_stream_t {
    ap_uint<512> data;
    ap_uint<1> last;
};
#else  
typedef ap_axiu<512, 1, 1, 1> raw_stream_t;
#endif

#define SIG_MSGHDR_SEND_I 0
#define SIG_MSGHDR_SEND_O 1

#define SIG_MSGHDR_RECV_I 2
#define SIG_MSGHDR_RECV_O 3

#define SIG_W_CMD_O    4
#define SIG_W_DATA_O   5
#define SIG_W_STATUS_I 6

#define SIG_R_CMD_O    7
#define SIG_R_DATA_I   8
#define SIG_R_STATUS_I 9

#define SIG_ING_I 10
#define SIG_EGR_O 11

#define SIG_END 12

/* For cosimulation purposes it is easier to allow a full packet to be
 * buffered in the internal queues.
 */ 
#define STREAM_DEPTH 2

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
#define MAC_DST 0xAAAAAAAAAAAA
#define MAC_SRC 0xBBBBBBBBBBBB

/* Homa Header Constants
 * #DEOFF            - Number of 4 byte chunks in the data header (<< 2)
 * #DATA_PKT_HEADER - How many bytes the Homa DATA packet header takes
 * (ethernet + ipv6 + common header + data header)
 * #GRANT_PKT_HEADER - How many bytes the Homa GRANT packet header
 * takes (ethernet + ipv6 + common header + data header)
 * #PREFACE_HEADER   - Number of bytes in ethernet + ipv6 header
 * #HOMA_DATA_HEADER - Number of bytes in a hoima data ethernet header
 */
#define DOFF             160
#define DATA_PKT_HEADER  114
#define GRANT_PKT_HEADER 87 
#define PREFACE_HEADER   54 
#define HOMA_DATA_HEADER 60 // Number of bytes in homa data ethernet header

/* Homa Configuration: These parameters configure the behavior of the
 * Homa core. These parameters are a part of the design and can not be
 * runtime configured.
 *
 * #HOMA_PAYLOAD_SIZE       - How many bytes maxmimum of message data can fit in a packet
 * #HOMA_MAX_MESSAGE_LENGTH - The maximum number of bytes in a single message
 * #OVERCOMMIT_BYTES        - The maximum number of bytes that can be
 * actively granted to. WARNING: This will only effect simulation and
 * the def needs to be modified in the verilog module for this to
 * change in hardware
 * #RTT_BYTES - How many bytes can be sent in the time it takes a
 * packet to make a round trip
 */
#define HOMA_PAYLOAD_SIZE       1386    
#define HOMA_MAX_MESSAGE_LENGTH 1000000 
#define OVERCOMMIT_BYTES        480000
// #define RTT_BYTES (ap_uint<32>) 5000

/*
 * Homa packet types
 *
 * TODO Only DATA and GRANT implemented
 */
#define DATA     0x10
#define GRANT    0x11
#define RESEND   0x12
#define UNKNOWN  0x13
#define BUSY     0x14
#define CUTOFFS  0x15
#define FREEZE   0x16
#define NEED_ACK 0x17
#define ACK      0x18

/**
 * homa_packet_type_t - Store one of the homa packet types
 */
typedef ap_uint<8> homa_packet_type_t;

/**
 * data_bytes_e - Stored in an outgoing chunk to instruct the data
 * buffer how many bytes need to be captured and stored in the chunk
 */
enum data_bytes_e {
    NO_DATA      = 0,
    ALL_DATA     = 64,
    PARTIAL_DATA = 14,
};

/* Peer Configuration: Used for storing state associated with a peer
 * and performing lookups on incoming header data to determine the local
 * peer ID.
 *
 * #MAX_PEERS      - The number of distinct peers the home core can track
 * #MAX_PEERS_LOG2 - An index into the set of peers (leaving room for +1 adjustment).
 * 
 */
#define MAX_PEERS            16384
#define MAX_PEERS_LOG2       14
#define PEER_BUCKETS         16384
#define PEER_BUCKET_SIZE     8
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
#define MAX_RPCS            16384 // TODO Maximum number of RPCs
#define MAX_SERVER_IDS      16384/2 // TODO Maximum number of RPCs
#define MAX_CLIENT_IDS      16384/2 // TODO Maximum number of RPCs
#define RPC_BUCKETS         16384 // Size of cuckoo hash sub-tables
#define RPC_BUCKET_SIZE     8     // Size of cuckoo hash sub-tables
#define RPC_SUB_TABLE_INDEX 14    // Index into sub-table entries

typedef ap_uint<MAX_RPCS_LOG2> local_id_t;


struct rpc_hashpack_t {
   ap_uint<128> s6_addr;
   ap_uint<64> id;
   ap_uint<16> port;
   ap_uint<16> empty;

#define RPC_HP_SIZE 7 // Number of 32 bit chunks to hash for RPC table

   bool operator==(const rpc_hashpack_t & other) const {
      return (s6_addr == other.s6_addr && id == other.id && port == other.port);
   }
};

/**
 * srpt_grant_in_t - Output of the grant SRPT core which indicates the
 * next best grant packet to be sent.
 */
#define SRPT_GRANT_OUT_SIZE     97
#define SRPT_GRANT_OUT_PEER_ID  13,0
#define SRPT_GRANT_OUT_RPC_ID   29,14
#define SRPT_GRANT_OUT_RECV     61,30
#define SRPT_GRANT_OUT_GRANT    93,62
#define SRPT_GRANT_OUT_PRIORITY 96,94

typedef ap_uint<SRPT_GRANT_OUT_SIZE> srpt_grant_out_t;

/**
 * srpt_grant_in_t - Output of the grant SRPT core which indicates the
 * next best grant packet to be sent.
 */
#define SRPT_GRANT_IN_SIZE    126
#define SRPT_GRANT_IN_PEER_ID 13,0
#define SRPT_GRANT_IN_RPC_ID  29,14
#define SRPT_GRANT_IN_OFFSET  61,30
#define SRPT_GRANT_IN_MSG_LEN 93,62
#define SRPT_GRANT_IN_PMAP    95,94 

typedef ap_uint<SRPT_GRANT_IN_SIZE> srpt_grant_in_t;

/**
 * srpt_sendmsg_t - Input to the srpt data core which creates a new
 * entry, indicating the RPC is eligible to be transmitted as long as
 * it is granted and buffered.
 */
#define SRPT_SENDMSG_SIZE       86
#define SRPT_SENDMSG_RPC_ID     15,0
#define SRPT_SENDMSG_MSG_LEN    35,16
#define SRPT_SENDMSG_GRANTED    55,36
#define SRPT_SENDMSG_DBUFF_ID   65,56
#define SRPT_SENDMSG_DMA_OFFSET 85,66

typedef ap_uint<SRPT_SENDMSG_SIZE> srpt_sendmsg_t;

/**
 * srpt_pktq_t - An output from the srpt data core which communicates
 * the next best data chunk to grab from DMA.
 */
#define PKTQ_SIZE      99
#define PKTQ_RPC_ID    15,0
#define PKTQ_DBUFF_ID  24,16
#define PKTQ_REMAINING 45,26
#define PKTQ_DBUFFERED 65,46
#define PKTQ_GRANTED   85,66
#define PKTQ_PRIORITY  98,86

typedef ap_uint<PKTQ_SIZE> srpt_pktq_t;

/**
 * srpt_sendq_t - An output from the srpt data core which communicates
 * the best RPC to be sent.
 */
#define SENDQ_SIZE      101
#define SENDQ_RPC_ID    15,0
#define SENDQ_DBUFF_ID  25,16
#define SENDQ_OFFSET    57,26
#define SENDQ_DBUFFERED 77,58
#define SENDQ_MSG_LEN   97,78
#define SENDQ_PRIORITY  100,98

typedef ap_uint<SENDQ_SIZE> srpt_sendq_t;

/**
 * srpt_dbuff_notif_t - Bitvector input into the srpt data core. This
 * provides the offset of new data that has been incorporated into the
 * on chip data buffer which can be used to update the internal RPC
 * state of the queue and determine its eligibility.
 */
#define SRPT_DBUFF_NOTIF_SIZE   36
#define SRPT_DBUFF_NOTIF_RPC_ID 15,0
#define SRPT_DBUFF_NOTIF_OFFSET 35,16

typedef ap_uint<SRPT_DBUFF_NOTIF_SIZE> srpt_dbuff_notif_t;

/**
 * srpt_grant_notif_t - Bitvector input into the srpt grant core. The
 * very first packet creates the entry in the grant queue. After that,
 * arrived packets are used to notify the core of arrived packets so
 * that it may update its internal state of the RPC to reflect that
 * change.
 */
#define SRPT_GRANT_NOTIF_SIZE   36
#define SRPT_GRANT_NOTIF_RPC_ID 15,0
#define SRPT_GRANT_NOTIF_OFFSET 35,16

typedef ap_uint<SRPT_GRANT_NOTIF_SIZE> srpt_grant_notif_t;

/* Cutoff for retramsission
 * 1ms == 1000000ns == 200000 cycles
 */
#define REXMIT_CUTOFF 200000

#define PMAP_INIT (ap_uint<3>)1
#define PMAP_BODY (ap_uint<3>)2
#define PMAP_COMP (ap_uint<3>)4

typedef ap_uint<3> pmap_state_t;

// Index into a packetmap bitmap
typedef ap_uint<10> packetmap_idx_t; // TODO rename

/** 
 * pmap_entry_t - Entry associated with each incoming RPC to determine
 * packets that have already arrived. Uses a 64 bit sliding
 * window. Window only slides when bit 0 is set and it slides as far
 * as there are 1 values.
 */
struct pmap_entry_t {
    packetmap_idx_t head;
    packetmap_idx_t length;
    ap_uint<64> map;
};

// TODO 
struct touch_t {
    local_id_t rpc_id;
    bool init;
    packetmap_idx_t offset; // Bit to set in packet map
    packetmap_idx_t length; // Total length of the message
};

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
 * infomration will be placed. These offsets are relative to the second
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

/* GRANT Packet */
#define CHUNK_HOMA_GRANT_OFFSET     111,80
#define CHUNK_HOMA_GRANT_PRIORITY   79,72


/* Data Buffer Configuration:
 * The data buffer stores message data on chip so that it can be
 * quickly accessed when an RPC is selected to sent. There should be
 * sufficient data on chip such that if an RPC is selected it can run
 * to completion without stalling for data.
 */
#define NUM_EGRESS_BUFFS 64  // Number of data buffers (max outgoing RPCs) TODO change name
#define DBUFF_INDEX      10  // Index into the data buffers
#define DBUFF_CHUNK_SIZE 64  // Size of a "chunk" of a data buffer
#define DBUFF_NUM_CHUNKS 256 // Number of "chunks" in one buffer
#define DBUFF_CHUNK_INDEX 8  // Index into 256 chunks
#define DBUFF_BYTE_INDEX 14  // Byte index within data buffer
 
#define NUM_INGRESS_BUFFS 64  // How many ingress DMA buffers are availible

#define NUM_INGRESS_DMA   64  // How many ingress DMA slots are availible

/**
 * dbuff_id_t - An DBUFF_INDEX bitwidth index into the NUM_BUFF number
 * of data buffers
 */
typedef ap_uint<DBUFF_INDEX> dbuff_id_t;

/**
 * dbuff_t - A single data buffer is an array of DBUFF_NUM_CHUNK 512
 * bit vectors. So, DBUFF_NUM_CHUNKS * 64 bytes in size
 */
typedef integral_t dbuff_t[DBUFF_NUM_CHUNKS];

/**
 * dbuff_boffset_t - Byte offset into a dbuff_t
 */
typedef ap_uint<DBUFF_BYTE_INDEX> dbuff_boffset_t;

/**
 * dbuff_coffset_t - Chunk offset into a dbuff_t
 */
typedef ap_uint<DBUFF_CHUNK_INDEX> dbuff_coffset_t;

/**
 * struct dbuff_in_t - After data is retrieved from DMA it is placed
 * in one of these structures to be passed to the data buffer core
 * which will store that DMA data until it is ready to be sent.
 */
struct dbuff_in_t {
    integral_t data;
    dbuff_id_t dbuff_id;
    local_id_t local_id;
    ap_uint<32> offset;
    ap_uint<32> msg_len;
    ap_uint<1> last;
};

/**
 * struct in_chunk_t - incoming packets arrive one chunk at a time and
 * so as the data arrives that needs to DMA'd, it is passed in one of
 * these structures which carries the data and offset in DMA space to
 * be placed.
 */
struct in_chunk_t {
    integral_t   buff;   // Data to be written to DMA
    // ap_uint<6> valid;
    data_bytes_e type;   // Partial Chunk, Full Chunk, Empty Chunk
    ap_uint<1>   last;   // Indicate last packet in transaction
};

/**
 * struct out_chunk_t - outgoing headers are broken up into 64B chunks
 * that will read from the data buffer to grab message data and then
 * eventually be placed onto the link. This structure includes space
 * for the chunk of data that needs to be sent, and the local data
 * needed to retrieve that information.
 */
struct out_chunk_t {
    homa_packet_type_t type;   // What is the type of this outgoing packet
    dbuff_id_t egress_buff_id; // Which data buffer is the message stored in
    local_id_t local_id;     // What is the RPC ID associated with this message
    ap_uint<32> offset;      // What byte offset is this output chunk for
    ap_uint<32> length;      // What is the total length of this message
    data_bytes_e data_bytes; // How many data bytes to add to this block
    integral_t buff;         // Data to be sent onto the link
    ap_uint<1> last;         // Is this the last chunk in the sequence
};

/**
 * struct homa_rpc_t - State associated with and RPC
 */
struct homa_rpc_t {
    ap_uint<128> saddr;    // Address of sender (sendmsg) or receiver (recvmsg)
    ap_uint<128> daddr;    // Address of receiver (sendmsg) or sender (recvmsg)
    ap_uint<16>  dport;    // Port of sender (sendmsg) or receiver (recvmsg)
    ap_uint<16>  sport;    // Port of sender (sendmsg) or receiver (recvmsg)
    ap_uint<64>  id;       // RPC ID (potentially not local)
    ap_uint<32>  iov_size; // 
    ap_uint<32>  iov;      //

    dbuff_id_t   ingress_dma_id; // ID for offset of packet data -> DMA
    dbuff_id_t   egress_buff_id; // ID for cache of DMA -> packet datra
};

/* Offsets within the sendmsg and recvmsg bitvector for sendmsg and
 * recvmsg requests that form the msghdr
 */
#define MSGHDR_SADDR        127,0   // Address of sender (sendmsg) or receiver (recvmsg)
#define MSGHDR_DADDR        255,127 // Address of receiver (sendmsg) or sender (recvmsg)
#define MSGHDR_SPORT        273,256 // Port of sender (sendmsg) or receiver (recvmsg)
#define MSGHDR_DPORT        289,274 // Address of receiver (sendmsg) or sender (recvmsg) 
#define MSGHDR_IOV          321,290 // Message contents DMA offset
#define MSGHDR_IOV_SIZE     353,322 // Size of message in DMA space

/* Offsets within the sendmsg bitvector for the sendmsg specific information */
#define MSGHDR_SEND_ID      417,354 // RPC identifier
#define MSGHDR_SEND_CC      481,418 // Completion Cookie
#define MSGHDR_SEND_SIZE    482

/* Offsets within the recvmsg bitvector for the recvmsg specific information */
#define MSGHDR_RECV_ID      417,354 // RPC identifier
#define MSGHDR_RECV_CC      481,418 // Completion Cookie
#define MSGHDR_RECV_FLAGS   513,482 // Interest list
#define MSGHDR_RECV_SIZE    514

/**
 * msghdr_send_t - input bitvector from the user for sendmsg requests
 */
typedef ap_uint<MSGHDR_SEND_SIZE> msghdr_send_t;

/**
 * msghdr_recv_t - input bitvector from the user for recvmsg requests
 */
typedef ap_uint<MSGHDR_RECV_SIZE> msghdr_recv_t;

/* Flags which specify the interest and behavior of the recvmsg call
 * from the user.
 * 
 * TODO not all implemented
 */
#define HOMA_RECVMSG_REQUEST       0x01
#define HOMA_RECVMSG_RESPONSE      0x02
#define HOMA_RECVMSG_NONBLOCKING   0x04
#define HOMA_RECVMSG_VALID_FLAGS   0x07

/* Maximum number of recvmsgs that can be queued pending a match with
 * a completed packet and the maximum number of completed packets that
 * can be queued pending a match with a recvmsg request.
 */
#define MAX_RECV_MATCH 16

/**
 * struct recv_interest_t - Matched with a an incoming completed RPC
 * to be returned to the user who invoked the recvmsg request/
 */ 
struct recv_interest_t {
    ap_uint<16>  sport; // Port of the caller
    ap_uint<32>  flags; // Interest list
    ap_uint<64>  id;    // ID of interest
};

/**
 * struct onboard_send_t - Structure for onboarding new sendmsg
 * requests into the system. This data is distributed to the rpc_map,
 * rpc_state, and eventually the srpt_data core to be sent.
 */
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
    local_id_t  local_id;       // Local RPC ID 
    dbuff_id_t  egress_buff_id; // Data buffer ID for outgoing data
    peer_id_t   peer_id;        // Local ID for this destination address
};

/**
 * struct header_t - This structure is passed around for outgoing and
 * incoming packets * A number of local parameters store the where
 * information is located * for this packet and the state of this
 * packet. For outgoing packets, * the fields are populated as the header
 * is passed through cores. For * incoming packets, the fields are
 * populated from the data read from the * link, and then is passed
 * through the cores for dispersion.
 */
struct header_t {
    // Local Values
    local_id_t  local_id;           // ID within RPC State
    peer_id_t   peer_id;            // ID of this peer
    dbuff_id_t  egress_buff_id;     // ID of buffer for DMA -> packet data
    dbuff_id_t  ingress_dma_id;     // ID of buffer for packet data -> DMA 
    ap_uint<64> completion_cookie;  // Cookie from the origin sendmsg
    ap_uint<32> packet_bytes;
    pmap_state_t packetmap;

    // IPv6 + Common Header
    ap_uint<16>        payload_length;
    ap_uint<128>       saddr;
    ap_uint<128>       daddr;
    ap_uint<16>        sport;
    ap_uint<16>        dport;
    homa_packet_type_t type;
    ap_uint<64>        id;

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

/**
 * struct dma_w_req_t - DMA write request require the actual data that
 * needs to be written and a global offset in the DMA space where that
 * data needs to be written
 */
struct dma_w_req_t {
    integral_t data;
    ap_uint<32> offset;
    ap_uint<32> strobe;
};

/**
 * WARNING: C Simulation Only
 * Internal storage for an RPC that needs to be sent onto the link
 */
struct srpt_data_t {
    local_id_t rpc_id;
    dbuff_id_t dbuff_id;
    ap_uint<32> remaining;
    ap_uint<32> total;
};

/**
 * WARNING: C Simulation Only
 * Internal storage for an RPC that may need grants
 */
struct srpt_grant_t {
    peer_id_t peer_id;
    local_id_t rpc_id;
    ap_uint<32> recv_bytes;
    ap_uint<32> grantable_bytes;
};

/**
 * WARNING: C Simulation Only
 * How many simultaneous peers can have active grants.
 */
#define MAX_OVERCOMMIT 8

/* Helper Macros */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/* Translates between the RPC ID assigned to Send message requests,
 * and its index within the RPC table. Send message requests are
 * allocated the upper half of the ID space. The last bit is reserved
 * to determine if the core is the client or the server, and the 0
 * value is reserved as a "null" rpc ID.
 */
#define SEND_RPC_ID_FROM_INDEX(a) ((a + 1 + MAX_RPCS/2) << 1)
#define SEND_INDEX_FROM_RPC_ID(a) ((a >> 1) - 1)

/* Translates between the RPC ID assigned to receive message requests,
 * and its index within the RPC table. Receive message requsts are
 * allocated the lower half of the ID space. The last bit is reserved
 * to determine if the core is the client or the server, and the 0
 * value is reserved as a "null" rpc ID.
 */
#define RECV_RPC_ID_FROM_INDEX(a) ((a + 1) << 1)
#define RECV_INDEX_FROM_RPC_ID(a) ((a >> 1) - 1)

/* Translates between the peer ID, and its index within the peer table
 * A peer ID of 0 is the "null" peer that has no slot in the peer table
 */
#define PEER_ID_FROM_INDEX(a) (a + 1)
#define INDEX_FROM_PEER_ID(a) (a - 1)

/* The last bit determines if the core is the client or the server in
 * an interaction * This can be called on an RPC ID to convert
 * Sender->Client and Client->Sender
 */
#define LOCALIZE_ID(sender_id) ((sender_id) ^ 1);

/* Used to determine if an ID is a client ID or a server ID which is
 * encoded in the last bit of the RPC ID
 */
#define IS_CLIENT(id) ((id & 1) == 0) // Client RPC IDs are even, servers are odd

#ifdef COSIM
void homa(hls::stream<msghdr_send_t> & msghdr_send_shim_i,
	  hls::stream<msghdr_send_t> & msghdr_send_shim_o,
	  hls::stream<msghdr_recv_t> & msghdr_recv_shim_i,
	  hls::stream<msghdr_recv_t> & msghdr_recv_shim_o,
	  hls::stream<am_cmd_t>      & w_cmd_queue_shim_o,
	  hls::stream<ap_uint<512>>  & w_data_queue_shim_o,
	  hls::stream<am_status_t>   & w_status_queue_shim_i,
	  hls::stream<am_cmd_t>      & r_cmd_queue_shim_o,
	  hls::stream<ap_uint<512>>  & r_data_queue_shim_i,
	  hls::stream<am_status_t>   & r_status_queue_shim_i,
	  hls::stream<raw_stream_t>  & link_ingress_shim_i,
	  hls::stream<raw_stream_t>  & link_egress_shim_o,
	  hls::stream<uint32_t>      & cmd_token_i);
#else
void homa(hls::stream<msghdr_send_t> & msghdr_send_i,
	  hls::stream<msghdr_send_t> & msghdr_send_o,
	  hls::stream<msghdr_recv_t> & msghdr_recv_i,
	  hls::stream<msghdr_recv_t> & msghdr_recv_o,
	  hls::stream<am_cmd_t>      & w_cmd_queue_o,
	  hls::stream<ap_uint<512>>  & w_data_queue_o,
	  hls::stream<am_status_t>   & w_status_queue_i,
	  hls::stream<am_cmd_t>      & r_cmd_queue_o,
	  hls::stream<ap_uint<512>>  & r_data_queue_i,
	  hls::stream<am_status_t>   & r_status_queue_i,
	  hls::stream<raw_stream_t>  & link_ingress,
	  hls::stream<raw_stream_t>  & link_egress);
#endif

#endif


