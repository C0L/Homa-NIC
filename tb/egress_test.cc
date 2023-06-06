// Local Variables:
// compile-command: "source /opt/Vitis_HLS/Vitis_HLS/2023.1/settings64.sh && cd .. && make egress_test"
// End:

#include <cstdio>
#include "ap_axi_sdata.h"
#include "hls_stream.h"

#include "homa.hh"

using namespace std;

#define TEST_T(x) if (x) { std::cerr << "PASS\n"; } else { std::cerr << "FAIL\n"; return 1;}

void print_packet(hls::stream<raw_stream_t> & link_egress) {
  raw_stream_t block;
  block.last = 0;

  while (!block.last) {
    link_egress.read(block);
    for (int i = 0; i < 64; ++i) {
      std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)(unsigned char) block.data[i];
      std::cout << "|" << std::endl;
      // std::cout << std::hex << (uint32_t) block.data[i];
    }
  }

  std::cerr << std::endl;
}

bool test_egress() {
  homa_t homa_cfg;
  homa_cfg.rtt_bytes = 60000;

  char msg[] = "TEST MESSAGE. TEST MESSAGE. TEST MESSAGE. TEST MESSAGE. TEST ME";

  hls::stream<raw_stream_t> link_ingress;
  hls::stream<raw_stream_t> link_egress;
  dbuff_chunk_t maxi_in[128];
  dbuff_chunk_t maxi_out[128];
  for (int i = 0; i < 128; ++i) {
    for (int j = 0; j < 64; ++j) {
      maxi_in[i].buff[j] = msg[j];
    }
  }

  sockaddr_in6_t dest_addr;
  sockaddr_in6_t src_addr;

  dest_addr.sin6_addr.s6_addr = 0xDEADBEEFDEADBEEF;
  dest_addr.sin6_port = 0xBEEF;
  src_addr.sin6_addr.s6_addr = 0xDEADBEEFDEADBEEF;
  src_addr.sin6_port = 0xBEEF;

  // New RPC, offest 0 DMA out, offset 0 DMA in, 128 bytes to send, id (request message), cookie
  params_t params = {0, 0, 128, dest_addr, src_addr, 0, 0xFFFFFFFFFFFFFFFF};

  // Construct a new RPC to ingest  
  homa(&homa_cfg, &params, link_ingress, link_egress, maxi_in, maxi_out);

  print_packet(link_egress);
  
  return 0;
}

int main() {
  std::cerr << "****************************** START TEST BENCH ******************************" << endl;

  if (test_egress()) { return 1; }

  std::cerr << "******************************  END TEST BENCH  ******************************" << endl;

  return 0;
}
