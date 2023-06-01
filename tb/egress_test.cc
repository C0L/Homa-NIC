// Local Variables:
// compile-command: "source /opt/Vitis_HLS/Vitis_HLS/2023.1/settings64.sh && cd .. && make egress_test"
// End:

#include <cstdio>
#include "ap_axi_sdata.h"
#include "hls_stream.h"

#include "homa.hh"

using namespace std;

#define TEST_T(x) if (x) { std::cerr << "PASS\n"; } else { std::cerr << "FAIL\n"; return 1;}

bool test_egress() {
  homa_t homa_cfg;

  hls::stream<raw_frame_t> link_ingress;
  hls::stream<raw_stream_t> link_egress;
  xmit_mblock_t maxi_in[128];
  xmit_mblock_t maxi_out[128];

  sockaddr_in6_t dest_addr;
  sockaddr_in6_t src_addr;
  params_t params = {0, 0, 128, dest_addr, src_addr, 0, 0xFFFFFFFFFFFFFFFF};

  // Construct a new RPC to ingest  
  homa(&homa_cfg, &params, link_ingress, link_egress, maxi_in, maxi_out);

  raw_stream_t block;
  link_egress.read(block);

  std::cerr << "PACKET OUT: " << block.data << std::endl;

  return 1;
}

int main() {
  std::cerr << "****************************** START TEST BENCH ******************************" << endl;

  if (test_egress()) { return 1; }

  std::cerr << "******************************  END TEST BENCH  ******************************" << endl;

  return 0;
}
