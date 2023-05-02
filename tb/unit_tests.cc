// Local Variables:
// compile-command: "source /opt/Vitis_HLS/2022.2/settings64.sh && cd .. && make unit_test"
// End:


#include <iostream>
#include <cstdio>
//#include "ap_axi_sdata.h"
//#include "hls_stream.h"

#include "srptmgmt.hh"

using namespace std;

// #include "homa.hh"
// #include "dma.hh"
//#include "rpcmgmt.hh"
// #include "peer.hh"

int test_unit_grant_srpt_queue() {
    std::cerr << "Unit Test: ingress" << endl;
    return 0;
}

int test_unit_xmit_srpt_queue() {
  std::cerr << "Unit Test: Xmit SRPT Queue" << endl;
  srpt_xmit_queue_t srpt_queue;

  std::cerr << "Checking Initial \"size\" == 0: ";
  if (srpt_queue.size == 0) { std::cerr << "PASS"; } else { std::cerr << "FAIL"; return 1; }
  std::cerr << endl;
  
  std::cerr << "Checking Initial .empty() == true: ";
  if (srpt_queue.empty()) { std::cerr << "PASS"; } else { std::cerr << "FAIL"; return 1; }
  std::cerr << endl;

  std::cerr << "Testing Single Element Push/Pop: ";
  srpt_xmit_entry_t e0 = {0, 99};
  srpt_queue.push(e0);
  if (srpt_queue.pop() == e0) { std::cerr << "PASS"; } else { std::cerr << "FAIL"; return 1; }
  std::cerr << endl;

  std::cerr << "Testing Two Element Simple Ordering: ";
  srpt_xmit_entry_t e1 = {1, 1};
  srpt_queue.push(e0);
  srpt_queue.push(e1);
  if (srpt_queue.pop() == e1 && srpt_queue.pop() == e0) { std::cerr << "PASS"; } else { std::cerr << "FAIL"; return 1; }
  std::cerr << endl;

  std::cerr << "Testing Two Element OOO: ";
  srpt_queue.push(e1);
  srpt_queue.push(e0);
  if (srpt_queue.pop() == e1 && srpt_queue.pop() == e0) { std::cerr << "PASS"; } else { std::cerr << "FAIL"; return 1; }
  std::cerr << endl;

  return 0;
}

int main() {
  std::cerr << "****************************** START TEST BENCH ******************************" << endl;

  /* Unit Tests */

  if (test_unit_xmit_srpt_queue()) { return 1; }
      //if (!(test_unit_grant_srpt_queue()) return 1;

  /* Functional Tests */

  std::cerr << "******************************  END TEST BENCH  ******************************" << endl;

  return 0;
}



