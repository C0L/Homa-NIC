/**
 * Reimplementation of some missing network QOL
 */

ap_uint<16> ntohs(ap_uint<16> netorder) {
  return ((netorder & 0xff) << 8) | (netorder >> 8);
}

typedef unsigned short sa_family_t;
typedef uint16_t in_port_t;

struct sockaddr {
  unsigned short   sa_family;
  char             sa_data[14];
};

struct in_addr {
  uint32_t       s_addr;     /* address in network byte order */
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
  uint32_t        sin6_flowinfo; /* IPv6 flow information */
  struct in6_addr sin6_addr;     /* IPv6 address */
  uint32_t        sin6_scope_id; /* Scope ID (new in 2.4) */
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

