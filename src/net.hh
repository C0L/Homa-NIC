#ifndef NET_H
#define NET_H

#include "ap_int.h"
#include <stdint.h>

#define INADDR_ANY ((unsigned long int) 0x00000000)

/**
 * Reimplementation of some missing network QOL
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

struct homa_ack_t {
  ap_uint<64> client_id;
  ap_uint<16> client_port;
  ap_uint<16> server_port;
};//__attribute__((packed));

struct common_header_t {
  ap_uint<16> sport;
  ap_uint<16> dport;
  ap_uint<32> unused1;
  ap_uint<32> unused2;
  ap_uint<8> doff;
  ap_uint<8> type;
  ap_uint<16> unused3;
  ap_uint<16> checksum;
  ap_uint<16> unused4;
  ap_uint<64> sender_id;
};//__attribute__((packed));


struct data_segment_t {
  ap_uint<32> offset;
  ap_uint<32> segment_length;
  homa_ack_t ack;
  char data[0];
};//__attribute__((packed));


struct data_header_t {
  common_header_t common;
  ap_uint<32> message_length;
  ap_uint<32> incoming;
  ap_uint<16> cutoff_version;
  ap_uint<8> retransmit;
  ap_uint<8> pad;
  data_segment_t seg;
};//__attribute__((packed));

/**
 * Given an IPv4 address, return an equivalent IPv6 address (an IPv4-mapped
 * one)
 * @ip4: IPv4 address, in network byte order.
 */
//in6_addr_t ipv4_to_ipv6(uint32_t ip4) {
//  in6_addr_t ret;
//  //if (ip4 == INADDR_ANY) return in6addr_any;
//  ret.in6_u.u6_addr32[2] = htonl(0xffff);
//  ret.in6_u.u6_addr32[3] = ip4;
//  return ret;
//}
//
//in6_addr_t canonical_ipv6_addr(sockaddr_in_union & addr) {
//  //if (addr) {
//  return (addr.sa.sa_family == AF_INET6)
//    ? addr.in6.sin6_addr
//    : ipv4_to_ipv6(addr.in4.sin_addr.s_addr);
//    //} else {
//    //return in6addr_any;
//  //}
//}
#endif
