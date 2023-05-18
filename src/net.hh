#ifndef NET_H
#define NET_H

#include "ap_int.h"
#include <stdint.h>

#define ETHERTYPE_IPV6 0x86DD
#define IPPROTO_HOMA 0xFD

#define INADDR_ANY ((unsigned long int) 0x00000000)

/*
 * Socket structures
 */
typedef uint16_t sa_family_t;
typedef uint16_t in_port_t;

struct sockaddr_t {
  sa_family_t      sa_family;
  unsigned char    sa_data[14];
};

struct in_addr_t {
  uint32_t       s_addr;     /* address in network byte order */
};

struct sockaddr_in_t {
  sa_family_t    sin_family; /* address family: AF_INET */
  in_port_t      sin_port;   /* port in network byte order */
  in_addr_t      sin_addr;   /* internet address */
};

struct in6_addr_t {
  ap_uint<128>    s6_addr;   /* IPv6 address */
  //unsigned char   s6_addr[16];   /* IPv6 address */
};

struct sockaddr_in6_t {
  sa_family_t     sin6_family;   /* AF_INET6 */
  in_port_t       sin6_port;     /* port number */
  uint32_t        sin6_flowinfo; /* IPv6 flow information */
  in6_addr_t      sin6_addr;     /* IPv6 address */
  uint32_t        sin6_scope_id; /* Scope ID (new in 2.4) */
};

/**
 * Holds either an IPv4 or IPv6 address (smaller and easier to use than
 * sockaddr_storage).
 */
typedef union sockaddr_in_union {
  sockaddr_t sa;
  sockaddr_in_t in4;
  sockaddr_in6_t in6;
} sockaddr_in_union_t;


/*
 * Ethernet structures
 */
struct ipv6_header_t {
  ap_uint<4> version;
  ap_uint<8> traffic_class;
  ap_uint<20> flow_label;
  ap_uint<16> payload_length;
  ap_uint<8> next_header;
  ap_uint<8> hop_limit;
  ap_uint<128> src_address;
  ap_uint<128> dest_address;
};

struct ethernet_header_t {
  ap_uint<48> dest_mac;
  ap_uint<48> src_mac;
  ap_uint<16> ethertype;
};

/*
 * Homa structures
 */
enum homa_packet_type {
  DATA               = 0x10,
  GRANT              = 0x11,
  RESEND             = 0x12,
  UNKNOWN            = 0x13,
  BUSY               = 0x14,
  CUTOFFS            = 0x15,
  FREEZE             = 0x16,
  NEED_ACK           = 0x17,
  ACK                = 0x18,
};

struct common_header_t {
  uint16_t sport;
  uint16_t dport;
  uint32_t unused1;
  uint32_t unused2;
  uint8_t doff;
  uint8_t type;
  uint16_t unused3;
  uint16_t checksum;
  uint16_t unused4;
  uint64_t sender_id;
};

struct homa_ack_t {
  ap_uint<64> client_id;
  ap_uint<16> client_port;
  ap_uint<16> server_port;
};

struct data_segment_t {
  uint32_t offset;
  uint32_t segment_length;
  homa_ack_t ack;
  char data[0];
};

struct data_header_t {
  common_header_t common;
  ap_uint<32> message_length;
  ap_uint<32> incoming;
  ap_uint<16> cutoff_version;
  ap_uint<8> retransmit;
  ap_uint<8> pad;
  data_segment_t seg;
};


struct homa_ack {
  __be64 client_id;
  __be16 client_port;
  __be16 server_port;
};

#endif
