#ifndef NET_H
#define NET_H

#include <ap_int.h>

/**
 * Reimplementation of some missing network QOL
 */
typedef unsigned short sa_family_t;
typedef ap_uint<16> in_port_t;

struct sockaddr {
  unsigned short   sa_family;
  char             sa_data[14];
};

struct in_addr {
  ap_uint<32>       s_addr;     /* address in network byte order */
};

struct sockaddr_in {
  sa_family_t    sin_family; /* address family: AF_INET */
  in_port_t      sin_port;   /* port in network byte order */
  struct in_addr sin_addr;   /* internet address */
};

struct in6_addr {
  unsigned char   s6_addr[16];   /* IPv6 address */
};

struct sockaddr_in6 {
  sa_family_t     sin6_family;   /* AF_INET6 */
  in_port_t       sin6_port;     /* port number */
  ap_uint<32>        sin6_flowinfo; /* IPv6 flow information */
  struct in6_addr sin6_addr;     /* IPv6 address */
  ap_uint<32>        sin6_scope_id; /* Scope ID (new in 2.4) */
};

/**
 * Holds either an IPv4 or IPv6 address (smaller and easier to use than
 * sockaddr_storage).
 */
typedef union sockaddr_in_union {
  struct sockaddr sa;
  struct sockaddr_in in4;
  struct sockaddr_in6 in6;
} sockaddr_in_union;

/**
 * struct homa_ack - Identifies an RPC that can be safely deleted by its
 * server. After sending the response for an RPC, the server must retain its
 * state for the RPC until it knows that the client has successfully
 * received the entire response. An ack indicates this. Clients will
 * piggyback acks on future data packets, but if a client doesn't send
 * any data to the server, the server will eventually request an ack
 * explicitly with a NEED_ACK packet, in which case the client will
 * return an explicit ACK.
 */
struct homa_ack_t {
  /**
   * @id: The client's identifier for the RPC. 0 means this ack
   * is invalid.
   */
  ap_uint<64> client_id;

  /** @client_port: The client-side port for the RPC. */
  ap_uint<16> client_port;

  /** @server_port: The server-side port for the RPC. */
  ap_uint<16> server_port;
};//__attribute__((packed));

/**
 * struct common_header - Wire format for the first bytes in every Homa
 * packet. This must partially match the format of a TCP header so that
 * Homa can piggyback on TCP segmentation offload (and possibly other
 * features, such as RSS).
 */
struct common_header_t {
  /**
   * @sport: Port on source machine from which packet was sent.
   * Must be in the same position as in a TCP header.
   */
  ap_uint<16> sport;

  /**
   * @dport: Port on destination that is to receive packet. Must be
   * in the same position as in a TCP header.
   */
  ap_uint<16> dport;

  /**
   * @unused1: corresponds to the sequence number field in TCP headers;
   * must not be used by Homa, in case it gets incremented during TCP
   * offload.
   */
  ap_uint<32> unused1;

  ap_uint<32> unused2;

  /**
   * @doff: High order 4 bits holds the number of 4-byte chunks in a
   * data_header (low-order bits unused). Used only for DATA packets;
   * must be in the same position as the data offset in a TCP header.
   */
  ap_uint<8> doff;

  /** @type: One of the values of &enum packet_type. */
  ap_uint<8> type;

  ap_uint<16> unused3;

  /**
   * @checksum: not used by Homa, but must occupy the same bytes as
   * the checksum in a TCP header (TSO may modify this?).*/
  ap_uint<16> checksum;

  ap_uint<16> unused4;

  /**
   * @sender_id: the identifier of this RPC as used on the sender (i.e.,
   * if the low-order bit is set, then the sender is the server for
   * this RPC).
   */
  ap_uint<64> sender_id;
};//__attribute__((packed));


/**
 * struct data_segment - Wire format for a chunk of data that is part of
 * a DATA packet. A single sk_buff can hold multiple data_segments in order
 * to enable send and receive offload (the idea is to carry many network
 * packets of info in a single traversal of the Linux networking stack).
 * A DATA sk_buff contains a data_header followed by any number of
 * data_segments.
 */
struct data_segment_t {
  /**
   * @offset: Offset within message of the first byte of data in
   * this segment. Segments within an sk_buff are not guaranteed
   * to be in order.
   */
  ap_uint<32> offset;

  /** @segment_length: Number of bytes of data in this segment. */
  ap_uint<32> segment_length;

  /** @ack: If the @client_id field is nonzero, provides info about
   * an RPC that the recipient can now safely free.
   */
  homa_ack_t ack;

  /** @data: the payload of this segment. */
  char data[0];
};//__attribute__((packed));


/* struct data_header - Overall header format for a DATA sk_buff, which
 * contains this header followed by any number of data_segments.
 */
struct data_header_t {
  common_header_t common;

  /** @message_length: Total #bytes in the *message* */
  ap_uint<32> message_length;

  /**
   * @incoming: The receiver can expect the sender to send all of the
   * bytes in the message up to at least this offset (exclusive),
   * even without additional grants. This includes unscheduled
   * bytes, granted bytes, plus any additional bytes the sender
   * transmits unilaterally (e.g., to send batches, such as with GSO).
   */
  ap_uint<32> incoming;

  /**
   * @cutoff_version: The cutoff_version from the most recent
   * CUTOFFS packet that the source of this packet has received
   * from the destination of this packet, or 0 if the source hasn't
   * yet received a CUTOFFS packet.
   */
  ap_uint<16> cutoff_version;

  /**
   * @retransmit: 1 means this packet was sent in response to a RESEND
   * (it has already been sent previously).
   */
  ap_uint<8> retransmit;

  ap_uint<8> pad;

  /** @seg: First of possibly many segments */
  data_segment_t seg;
};//__attribute__((packed));

#endif
