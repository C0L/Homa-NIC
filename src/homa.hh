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

#define AM_CMD_SIZE  32+48+4+4
#define AM_CMD_BTT   22,0
#define AM_CMD_TYPE  23,23
#define AM_CMD_DSA   29,24
#define AM_CMD_EOF   30,30
#define AM_CMD_DRR   31,31
#define AM_CMD_SADDR 32+48-1,32
#define AM_CMD_TAG   32+48+4-1,32+48
#define AM_CMD_RSVD  32+48+4+4-1,32+48+4


// 64 bit
// #define AM_CMD_SIZE  104
// #define AM_CMD_BTT   22,0
// #define AM_CMD_TYPE  23,23
// #define AM_CMD_DSA   29,24
// #define AM_CMD_EOF   30,30
// #define AM_CMD_DRR   31,31
// #define AM_CMD_SADDR 95,32
// #define AM_CMD_TAG   99,96
// #define AM_CMD_RSVD  103,100

typedef ap_axiu<AM_CMD_SIZE, 0, 0, 0> am_cmd_t;

#define AM_STATUS_SIZE   8
#define AM_STATUS_TAG    3,0
#define AM_STATUS_INTERR 4,4
#define AM_STATUS_DECERR 5,5
#define AM_STATUS_SLVERR 6,6
#define AM_STATUS_OKAY   7,7

typedef ap_axiu<AM_STATUS_SIZE, 0, 0, 0> am_status_t;

/**
 * integral_t - The homa core operates in 64B chunks. Data in the data
 * buffer is stored in 64B entries. Chunks are sent to the link 64B at
 * a time. DMA reads and writes at 64B in size to maximize the
 * throughput.
 */
typedef ap_uint<512> integral_t;

/* Offset into a message (up to HOMA_MAX_MESSAGE_LENGTH in size)
 */
typedef ap_uint<32> msg_addr_t;

/* Offset host memory (very large)
 */
typedef ap_uint<64> host_addr_t;

typedef ap_axiu<512, 0, 0, 0> raw_stream_t;

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

/* Homa Header Constants: Used for computing packet chunk offsets
 */
#define DOFF             160 // Number of 4 byte chunks in the data header (<< 2)
#define DATA_PKT_HEADER  114 // How many bytes the Homa DATA packet header takes
#define GRANT_PKT_HEADER 87  // Number of bytes in ethernet + ipv6 + common + data 
#define PREFACE_HEADER   54  // Number of bytes in ethernet + ipv6 header 
#define HOMA_DATA_HEADER 60  // Number of bytes in homa data ethernet header

/* Homa Configuration: These parameters configure the behavior of the
 * Homa core. These parameters are a part of the design and can not be
 * runtime configured (yet).
 */
#define HOMA_PAYLOAD_SIZE       1386     // How many bytes maxmimum of message data can fit in a packet
#define HOMA_MAX_MESSAGE_LENGTH 1000000  // The maximum number of bytes in a single message
/* WARNING: Cosimulation only*/
#define OVERCOMMIT_BYTES        480000   // The maximum number of bytes that can be actively granted to.

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
//enum data_bytes_e {
//    NO_DATA      = 0,
//    ALL_DATA     = 64,
//    PARTIAL_DATA = 14,
//}

/* Peer Configuration: Used for storing state associated with a peer
 * and performing lookups on incoming header data to determine the local
 * peer ID.
 */
#define MAX_PEERS            16384 // The number of distinct peers the home core can track
#define MAX_PEERS_LOG2       14    // An index into the set of peers (leaving room for +1 adjustment)
#define PEER_BUCKETS         16384 // Each hash bucket has PEER_BUCKET_SIZE entries
#define PEER_BUCKET_SIZE     8     // How many entries in each bucket
#define PEER_SUB_TABLE_INDEX 14    // Number of bits to index a PEER_BUCKETS buckets

typedef ap_uint<MAX_PEERS_LOG2> peer_id_t;

struct peer_hashpack_t {
    ap_uint<128> s6_addr;
#define PEER_HP_SIZE 4
    bool operator==(const peer_hashpack_t & other) const {
	return (s6_addr == other.s6_addr); 
    }
};

/* RPC ID Configuration: There are 2^14
 */
#define MAX_RPCS_LOG2       14    // Number of bits to express an RPC. TODO this changed
#define MAX_RPCS            16384 // Maximum number of RPCs
#define MAX_PORTS           16384 // Maximum number of ports
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
 * srpt_grant_send_t - Output of the grant SRPT core which indicates the
 * next best grant packet to be sent.
 */
#define SRPT_GRANT_SEND_SIZE     48
#define SRPT_GRANT_SEND_RPC_ID   15,0 // The RPC ID we want to grant to
#define SRPT_GRANT_SEND_MSG_ADDR 47,16 // New grant offset for ID

typedef ap_uint<SRPT_GRANT_SEND_SIZE> srpt_grant_send_t;

/**
 * srpt_grant_new_t - Input to the grant SRPT core which communicates
 * when data has been recieved from a certain packet.
 */
#define SRPT_GRANT_NEW_SIZE     64
#define SRPT_GRANT_NEW_PEER_ID  13,0  // Unique ID for this peer
#define SRPT_GRANT_NEW_RPC_ID   29,14 // Local ID valid in rpc state core
#define SRPT_GRANT_NEW_MSG_LEN  61,30 // Total number of bytes in message
#define SRPT_GRANT_NEW_PMAP     63,62 // First packet in a sequence?

typedef ap_uint<SRPT_GRANT_NEW_SIZE> srpt_grant_new_t;

#define SRPT_QUEUE_ENTRY_SIZE      99
#define SRPT_QUEUE_ENTRY_RPC_ID    15,0  // ID of this transaction
#define SRPT_QUEUE_ENTRY_DBUFF_ID  24,16 // Corresponding on chip cache
#define SRPT_QUEUE_ENTRY_REMAINING 45,26 // Remaining to be sent or cached
#define SRPT_QUEUE_ENTRY_DBUFFERED 65,46 // Number of bytes cached
#define SRPT_QUEUE_ENTRY_GRANTED   85,66 // Number of bytes granted
#define SRPT_QUEUE_ENTRY_PRIORITY  88,86 // Deprioritize inactive messages

typedef ap_uint<SRPT_QUEUE_ENTRY_SIZE> srpt_queue_entry_t;

#define DMA_R_REQ_SIZE      185
#define DMA_R_REQ_RPC_ID    15,0  // ID of this transaction
#define DMA_R_REQ_DBUFF_ID  24,16 // Corresponding on chip cache
#define DMA_R_REQ_REMAINING 45,26 // Remaining to be sent or cached
#define DMA_R_REQ_DBUFFERED 65,46 // Number of bytes cached
#define DMA_R_REQ_GRANTED   85,66 // Number of bytes granted
#define DMA_R_REQ_PRIORITY  88,86 // Deprioritize inactive messages
#define DMA_R_REQ_HOST_ADDR 152,89 // Deprioritize inactive messages
#define DMA_R_REQ_MSG_LEN   184,153 // Deprioritize inactive messages

typedef ap_uint<DMA_R_REQ_SIZE> dma_r_req_t;

/**
 * srpt_data_new_in_t - Input to the srpt data core which creates a new
 * entry, indicating the RPC is eligible to be transmitted as long as
 * it is granted and buffered.
 */
//#define SRPT_DATA_NEW_SIZE      66
//#define SRPT_DATA_NEW_RPC_ID    15,0    // Local ID valid in rpc state core
//#define SRPT_DATA_NEW_MSG_LEN   35,16   // Total number of bytes in message
//#define SRPT_DATA_NEW_GRANTED   55,36   // Unscheduled bytes
//#define SRPT_DATA_NEW_DBUFF_ID  65,56   // On chip data buffer
// #define SRPT_DATA_NEW_HOST_ADDR 129,66  // Where are we buffering data from

// typedef ap_uint<SRPT_DATA_NEW_SIZE> srpt_data_new_t;

/**
 * srpt_data_send_t - An output from the srpt data core which communicates
 * the best RPC to be sent.
 */
// #define SRPT_DATA_SEND_SIZE      56
// #define SRPT_DATA_SEND_RPC_ID    15,0  // Local ID valid in rpc state core
// #define SRPT_DATA_SEND_REMAINING 35,16 // How many more bytes remaining to send
// #define SRPT_DATA_SEND_GRANTED   55,36 // Head msg offset granted bytes
// 
// typedef ap_uint<SRPT_DATA_SEND_SIZE> srpt_data_send_t;

/**
 * srpt_data_fetch_t - An output from the srpt data core which
 * communicates the next best data chunk to grab from DMA.
 */
//#define SRPT_DATA_FETCH_SIZE       152
//#define SRPT_DATA_FETCH_RPC_ID     15,0    // RPC we want data for
//#define SRPT_DATA_FETCH_DBUFF_ID   25,16   // Where this data will be placed
//#define SRPT_DATA_FETCH_HOST_ADDR  89,26   // Where in host memory do we read from
//#define SRPT_DATA_FETCH_PORT       105,90   // Where in host memory do we read from TODO would be nice to remove
//#define SRPT_DATA_FETCH_MSG_LEN    137,106 // What is the total length of the msg  

// typedef ap_uint<SRPT_DATA_FETCH_SIZE> srpt_data_fetch_t;

/**
 * srpt_dbuff_notif_t - Input notificaiton to the SRPT data core of
 * data arrivial in the on chip cache for outgoing messages. This
 * provides the offset of new data that has been incorporated into the
 * on chip data buffer which can be used to update the internal RPC
 * state of the queue and determine its eligibility.
 */
//#define SRPT_DATA_DBUFF_NOTIF_SIZE     36
//#define SRPT_DATA_DBUFF_NOTIF_RPC_ID   15,0  // The local ID this is relevent to
//#define SRPT_DATA_DBUFF_NOTIF_MSG_ADDR 35,16 // What offset just arrived
//
//typedef ap_uint<SRPT_DATA_DBUFF_NOTIF_SIZE> srpt_data_dbuff_notif_t;

/**
 * srpt_grant_notif_t - Input notification to the SRPT data core of a
 * received grant. The very first packet creates the entry in the
 * grant queue. After that, arrived packets are used to notify the
 * core of arrived packets so that it may update its internal state of
 * the RPC to reflect that change.
 */
//#define SRPT_DATA_GRANT_NOTIF_SIZE     36
//#define SRPT_DATA_GRANT_NOTIF_RPC_ID   15,0  // The local ID this grant is relevent to
//#define SRPT_DATA_GRANT_NOTIF_MSG_ADDR 35,16 // Offset granted up to
//
//typedef ap_uint<SRPT_DATA_GRANT_NOTIF_SIZE> srpt_data_grant_notif_t;


/**
 * rpc_to_offset_t - Provides a mapping from an RPC ID to an offset
 * within the particular user's buffer where data should be read to
 * written from
 */
struct rpc_to_offset_t {
    local_id_t rpc_id;
    ap_uint<32> offset;
};

/**
 * port_to_phys_t - Provides a mapping from a Homa port of a host
 * application to a physical base address of a buffer for data moving
 * from card 2 host
 */
#define PORT_TO_PHYS_SIZE 96
#define PORT_TO_PHYS_ADDR 63,0
#define PORT_TO_PHYS_PORT 95,64 
typedef ap_uint<PORT_TO_PHYS_SIZE> port_to_phys_t;

/**
 * log_entry_t - A single piece of debug information which is clumped
 * with other debug entries to form an entire log_t
 */
typedef ap_uint<8> log_status_t;

/**
 * log_entry_t - A log value for debugging that can be read from the host
 */
typedef ap_axiu<128, 0, 0, 0> log_entry_t;

/* Cutoff for retramsission
 * 1ms == 1000000ns == 200000 cycles
 */
#define REXMIT_CUTOFF 200000

#define PMAP_INIT (ap_uint<3>)1
#define PMAP_BODY (ap_uint<3>)2
#define PMAP_COMP (ap_uint<3>)4

typedef ap_uint<3> pmap_state_t;

/** 
 * pmap_entry_t - Entry associated with each incoming RPC to determine
 * packets that have already arrived. Uses a 64 bit sliding
 * window. Window only slides when bit 0 is set and it slides as far
 * as there are 1 values.
 */
struct pmap_entry_t {
    ap_uint<32> head;
    ap_uint<32> length;
    ap_uint<64> map;
};

// TODO 
struct touch_t {
    local_id_t rpc_id;
    bool init;
    //packetmap_idx_t offset; // Bit to set in packet map
    //packetmap_idx_t length; // Total length of the message
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
#define NUM_EGRESS_BUFFS 32  // Number of data buffers (max outgoing RPCs) TODO change name
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
 * struct h2c_dbuff_t - After data is retrieved from DMA it is placed
 * in one of these structures to be passed to the data buffer core
 * which will store that DMA data until it is ready to be sent.
 */
struct h2c_dbuff_t {
    integral_t  data;
    dbuff_id_t  dbuff_id;
    local_id_t  local_id;
    // host_addr_t  host_addr;
    msg_addr_t msg_addr;
    ap_uint<32> msg_len;
    ap_uint<1>  last;
};

/**
 * struct h2c_chunk_t - incoming packets arrive one chunk at a time and
 * so as the data arrives that needs to DMA'd, it is passed in one of
 * these structures which carries the data and offset in DMA space to
 * be placed.
 */
struct c2h_chunk_t {
    integral_t  data;   // Data to be written to DMA
    ap_uint<32> width;  // How many bytes (starting from byte 0) to write
    ap_uint<1>  last;   // Indicate last packet in transaction
};

/**
 * struct c2h_chunk_t - outgoing headers are broken up into 64B chunks
 * that will read from the data buffer to grab message data and then
 * eventually be placed onto the link. This structure includes space
 * for the chunk of data that needs to be sent, and the local data
 * needed to retrieve that information.
 */
struct h2c_chunk_t {
    homa_packet_type_t type;   // What is the type of this outgoing packet
    dbuff_id_t h2c_buff_id; // Which data buffer is the message stored in
    local_id_t local_id;       // What is the RPC ID associated with this message
    msg_addr_t msg_addr;       // What byte offset is this output chunk for
    ap_uint<32> width;         // What is the total length of this message
    ap_uint<32> keep;          // What is the total length of this message
    integral_t data;           // Data to be sent onto the link
    ap_uint<1> last;           // Is this the last chunk in the sequence
};

/**
 * struct homa_rpc_t - State associated with and RPC
 */
struct homa_rpc_t {
    ap_uint<128> saddr;         // Address of sender (sendmsg) or receiver (recvmsg)
    ap_uint<128> daddr;         // Address of receiver (sendmsg) or sender (recvmsg)
    ap_uint<16>  dport;         // Port of sender (sendmsg) or receiver (recvmsg)
    ap_uint<16>  sport;         // Port of sender (sendmsg) or receiver (recvmsg)
    ap_uint<64>  id;            // RPC ID (always local)
    msg_addr_t   buff_addr;     // Buffer offset at phys addr associated with this port
    ap_uint<32>  buff_size;     // Size of the message to send
    // TODO must this be stored in here? It is already in the SRPT_DATA_NEW!?!
    dbuff_id_t   h2c_buff_id;   //
};

/* Offsets within the sendmsg and recvmsg bitvector for sendmsg and
 * recvmsg requests that form the msghdr
 */
#define MSGHDR_SADDR        127,0   // Address of sender (sendmsg) or receiver (recvmsg) (128 bits)
#define MSGHDR_DADDR        255,127 // Address of receiver (sendmsg) or sender (recvmsg) (128 bits)
#define MSGHDR_SPORT        271,256 // Port of sender (sendmsg) or receiver (recvmsg) (16 bits)
#define MSGHDR_DPORT        287,272 // Address of receiver (sendmsg) or sender (recvmsg) (16 bits)
#define MSGHDR_BUFF_ADDR    351,288 // Message contents DMA offset (64 bits)
#define MSGHDR_BUFF_SIZE    383,352 // Size of message in DMA space (32 bits)

/* Offsets within the sendmsg bitvector for the sendmsg specific information */
#define MSGHDR_SEND_ID      447,384 // RPC identifier (64 bits)
#define MSGHDR_SEND_CC      511,448 // Completion Cookie (64 bits)
#define MSGHDR_SEND_SIZE    512     // Rounded to nearest 32 bits

/* Offsets within the recvmsg bitvector for the recvmsg specific information */
#define MSGHDR_RECV_ID      447,384 // RPC identifier
#define MSGHDR_RECV_CC      511,448 // Completion Cookie
#define MSGHDR_RECV_FLAGS   543,512 // Interest list
#define MSGHDR_RECV_SIZE    544     // Rounded to nearest 32 bits

/**
 * msghdr_send_t - input bitvector from the user for sendmsg requests
 * NOTE: The TLAST signal (which is created by ap_axiu) must be
 * present to integrate with certain cores that expect a TLAST signal
 * even if there is only a single beat of the stream transaction.
 */
typedef ap_axiu<MSGHDR_SEND_SIZE, 0, 0, 0> msghdr_send_t;

/**
 * msghdr_recv_t - input bitvector from the user for recvmsg requests
 * NOTE: The TLAST signal (which is created by ap_axiu) must be
 * present to integrate with certain cores that expect a TLAST signal
 * even if there is only a single beat of the stream transaction.
 */
typedef ap_axiu<MSGHDR_RECV_SIZE, 0, 0, 0> msghdr_recv_t;

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
    ap_uint<128> saddr;      // Address of sender (sendmsg) or receiver (recvmsg)
    ap_uint<128> daddr;      // Address of receiver (sendmsg) or sender (recvmsg)
    ap_uint<16>  sport;      // Port of sender (sendmsg) or receiver (recvmsg) 
    ap_uint<16>  dport;      // Port of sender (sendmsg) or receiver (recvmsg)
    msg_addr_t   buff_addr;  // Message contents DMA offset
    ap_uint<32>  buff_size;  // Size of message in DMA space 

    ap_uint<64>  id;         // RPC identifier 
    ap_uint<64>  cc;         // Completion Cookie

    // ap_uint<32> dbuffered;
    // ap_uint<32> granted; ?? 
     
    local_id_t  local_id;    // Local RPC ID 
    dbuff_id_t  h2c_buff_id; // Data buffer ID for outgoing data
    peer_id_t   peer_id;     // Local ID for this destination address
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
    local_id_t  local_id;          // ID within RPC State
    peer_id_t   peer_id;           // ID of this peer
    dbuff_id_t  h2c_buff_id;       // ID of buffer for DMA -> packet data
    host_addr_t host_addr;         // Address in hosts memory of msg buff 
    ap_uint<64> completion_cookie; // Cookie from the origin sendmsg
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
    local_id_t  rpc_id;
    ap_uint<16> port;
    integral_t  data;
    ap_uint<64> offset;
    ap_uint<32> strobe;
};

/**
 * WARNING: C Simulation Only
 * Internal storage for an RPC that needs to be sent onto the link
 */
struct srpt_data_t {
    local_id_t rpc_id;
    dbuff_id_t dbuff_id;
    msg_addr_t remaining;
    msg_addr_t granted;
    msg_addr_t dbuffered_req;
    msg_addr_t dbuffered_recv;
    msg_addr_t msg_len;
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

/* The last bit determines if the core is the client or the server in
 * an interaction. This can be called on an RPC ID to convert
 * Sender->Client and Client->Sender
 */
#define LOCALIZE_ID(sender_id) ((sender_id) ^ 1);

/* Used to determine if an ID is a client ID or a server ID which is
 * encoded in the last bit of the RPC ID
 */
#define IS_CLIENT(id) ((id & 1) == 0) // Client RPC IDs are even, servers are odd

/* Initial stack configurations
 */
#define STACK_EVEN 0
#define STACK_ODD  1
#define STACK_ALL  2

#define SRPT_INVALIDATE   0
#define SRPT_DBUFF_UPDATE 1
#define SRPT_GRANT_UPDATE 2 
#define SRPT_EMPTY        3
#define SRPT_BLOCKED      4 
#define SRPT_ACTIVE       5

/* DMA Log Values */
#define LOG_DATA_READ 0

/* Packet Selector Values */
#define LOG_GRANT_OUT 0
#define LOG_DATA_OUT 1

void homa(hls::stream<msghdr_send_t> & msghdr_send_i,
	  hls::stream<msghdr_send_t> & msghdr_send_o,
	  hls::stream<msghdr_recv_t> & msghdr_recv_i,
	  hls::stream<msghdr_recv_t> & msghdr_recv_o,
	  hls::stream<am_cmd_t>      & w_cmd_queue_o,
	  hls::stream<ap_axiu<512,0,0,0>>  & w_data_queue_o,
	  hls::stream<am_status_t>   & w_status_queue_i,
	  hls::stream<am_cmd_t>      & r_cmd_queue_o,
	  hls::stream<ap_axiu<512,0,0,0>>  & r_data_queue_i,
	  hls::stream<am_status_t>   & r_status_queue_i,
	  hls::stream<raw_stream_t>  & link_ingress,
	  hls::stream<raw_stream_t>  & link_egress,
	  hls::stream<port_to_phys_t> & h2c_port_to_phys_i,
	  hls::stream<port_to_phys_t> & c2h_port_to_phys_i,
	  hls::stream<log_entry_t> & log_out_o);

#endif
