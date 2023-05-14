// Local Variables:
// compile-command: "source /opt/Vitis_HLS/2022.2/settings64.sh && cd .. && make srptmgmt_test"
// End:

#include <iostream>
#include <cstdio>
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "srptmgmt.hh"

using namespace std;

#define TEST_T(x) if (x) { std::cerr << "PASS\n"; } else { std::cerr << "FAIL\n"; return 1;}

bool test_srpt_xmit() {
  std::cerr << "Functional Test: Xmit SRPT Queue" << endl;

  {
    std::cerr << "Single Packet Fully Granted: ";

    hls::stream<srpt_xmit_entry_t> srpt_queue_insert;
    hls::stream<srpt_xmit_entry_t> srpt_queue_grants;
    hls::stream<srpt_xmit_entry_t> srpt_queue_next;

    // Two outputs worth          remaining    granted      total
    srpt_queue_insert.write({1, PACKET_SIZE, PACKET_SIZE, PACKET_SIZE});

    // Load the insert into SRPT
    update_xmit_srpt_queue(srpt_queue_insert, srpt_queue_grants, srpt_queue_next);

    // Pop value from SRPT
    update_xmit_srpt_queue(srpt_queue_insert, srpt_queue_grants, srpt_queue_next);
    
    srpt_xmit_entry_t next;
    srpt_queue_next.read(next);

    update_xmit_srpt_queue(srpt_queue_insert, srpt_queue_grants, srpt_queue_next);

    TEST_T(next.rpc_id == 1 && next.remaining == PACKET_SIZE && srpt_queue_next.empty())
  }

  {
    std::cerr << "Single Packet Ungranted: ";
    hls::stream<srpt_xmit_entry_t> srpt_queue_insert;
    hls::stream<srpt_xmit_entry_t> srpt_queue_grants;
    hls::stream<srpt_xmit_entry_t> srpt_queue_next;

    // Two outputs worth
    srpt_queue_insert.write({1, 2*PACKET_SIZE, PACKET_SIZE, 2*PACKET_SIZE});

    // Load the insert into SRPT
    update_xmit_srpt_queue(srpt_queue_insert, srpt_queue_grants, srpt_queue_next);

    // Pop value from SRPT
    update_xmit_srpt_queue(srpt_queue_insert, srpt_queue_grants, srpt_queue_next);
    
    srpt_xmit_entry_t next;
    srpt_queue_next.read(next);

    // Should fail to pop the entry as granted bytes is 0
    update_xmit_srpt_queue(srpt_queue_insert, srpt_queue_grants, srpt_queue_next);

    next.print();
    
    TEST_T(next.remaining == 2*PACKET_SIZE && srpt_queue_next.empty())
  }

  {
    std::cerr << "Multi-Packet Partially Granted" << endl;

  }

  {
    std::cerr << "Multi-Packet Fully Granted" << endl;

  }

  {
    std::cerr << "Single-Packet Ungranted With Explicit Grant" << endl;

  }


  {
    std::cerr << "Multi-Packet Partially Granted With Explicit Grant" << endl;

  }
  return 0;
}


int main() {
  std::cerr << "****************************** START TEST BENCH ******************************" << endl;

  /* Functional Tests */
  if (test_srpt_xmit()) { return 1; }

  std::cerr << "******************************  END TEST BENCH  ******************************" << endl;

  return 0;
}
