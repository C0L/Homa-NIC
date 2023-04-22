#ifndef DMA_H
#define DMA_H

#include <hls_stream.h>
#include <stdint.h>

#include "net.hh"
#include "homa.hh"
#include "xmitbuff.hh"


struct homa_t;

struct params_t {
  // Offset in DMA space for output
  uint32_t buffout;
  // Offset in DMA space for input
  uint32_t buffin;
  int length;
  sockaddr_in6_t dest_addr;
  uint64_t id;
  uint64_t completion_cookie;
};

struct args_t {
  params_t params;
  xmit_buffer_t buffer;
};


#endif
