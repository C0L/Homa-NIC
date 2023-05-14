// Local Variables:
// compile-command: "source /opt/Vitis_HLS/2022.2/settings64.sh && cd .. && make unit_test"
// End:


#include <iostream>
#include <cstdio>
//#include "ap_axi_sdata.h"
//#include "hls_stream.h"

#include "srptmgmt.hh"

#define TEST_T(x) if (x) { std::cerr << "PASS\n"; } else { std::cerr << "FAIL\n"; return 1;}

using namespace std;

int test_unit_xmit_srpt_queue() {
  std::cerr << "Unit Test: Xmit SRPT Queue" << endl;

  {
    srpt_queue_t<srpt_xmit_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Checking Initial \"size\" == 0: ";
    TEST_T(srpt_queue.size == 0)
  }

  {
    srpt_queue_t<srpt_xmit_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Checking Initial .empty() == true: ";
    TEST_T(srpt_queue.empty())
  }

  {
    srpt_queue_t<srpt_xmit_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Single Element Push/Pop: ";
    srpt_xmit_entry_t e0 = srpt_xmit_entry_t(0, 99, 0, 0);
    srpt_queue.push(e0);
    srpt_xmit_entry_t e1 = srpt_queue.head();
    srpt_queue.pop();
    TEST_T(e1 == e0) 
  }

  {
    srpt_queue_t<srpt_xmit_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Two Element Simple Ordering: ";
    srpt_xmit_entry_t e0 = srpt_xmit_entry_t(0, 99, 0, 0);
    srpt_xmit_entry_t e1 = srpt_xmit_entry_t(1, 1, 0, 0);
    srpt_queue.push(e0);
    srpt_queue.push(e1);

    srpt_xmit_entry_t e2 = srpt_queue.head();
    srpt_queue.pop();

    srpt_xmit_entry_t e3 = srpt_queue.head();
    srpt_queue.pop();

    TEST_T(e2 == e1 && e3 == e0)
  }

  {
    srpt_queue_t<srpt_xmit_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Two Element OOO: ";
    srpt_xmit_entry_t e0 = srpt_xmit_entry_t(0, 99, 0, 0);
    srpt_xmit_entry_t e1 = srpt_xmit_entry_t(1, 1, 0, 0);
    srpt_queue.push(e1);
    srpt_queue.push(e0);

    srpt_xmit_entry_t e2 = srpt_queue.head();
    srpt_queue.pop();

    srpt_xmit_entry_t e3 = srpt_queue.head();
    srpt_queue.pop();

    TEST_T(e2 == e1 && e3 == e0)
  }

  {
    bool pass = true;
    srpt_queue_t<srpt_xmit_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Misordered Flood: ";
    for (uint32_t i = 0; i < MAX_SRPT; ++i) {
      srpt_xmit_entry_t e = srpt_xmit_entry_t(i,i,0,0);
      srpt_queue.push(e);
    }

    for (uint32_t i = 0; i < MAX_SRPT; ++i) {
      srpt_xmit_entry_t e = srpt_xmit_entry_t(i,i,0,0);
      srpt_xmit_entry_t c = srpt_queue.head();
      srpt_queue.pop();
      if (!(c == e)) { pass = false; }
    }

    TEST_T(pass)
  }

  {
    bool pass = true;
    srpt_queue_t<srpt_xmit_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Misordered Exceed Bound: ";
    for (uint32_t i = 0; i < MAX_SRPT+1; ++i) {
      srpt_xmit_entry_t e = srpt_xmit_entry_t(i,i+100,0,0);
      srpt_queue.push(e);
    }

    for (uint32_t i = 0; i < MAX_SRPT; ++i) {
      srpt_xmit_entry_t e = srpt_xmit_entry_t(i,i+10,0,0);
      srpt_queue.push(e);
    }

    if (srpt_queue.get_size() != MAX_SRPT) pass = false;

    for (uint32_t i = 0; i < MAX_SRPT; ++i) {
      srpt_xmit_entry_t e = srpt_xmit_entry_t(i,i+10,0,0);
      srpt_xmit_entry_t c = srpt_queue.head();
      srpt_queue.pop();
      if (!(c == e)) { pass = false; }
    }

    TEST_T(pass)
  }
    
  return 0;
}

int test_unit_grant_srpt_queue() {
  std::cerr << "Unit Test: Grant SRPT Queue" << endl;

  {
    srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Checking Initial \"size\" == 0: ";
    TEST_T(srpt_queue.size == 0)
  }

  {
    srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Checking Initial .empty() == true: ";
    TEST_T(srpt_queue.empty())
  }

  {
    srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Single Element Push/Pop: ";
    srpt_grant_entry_t e0 = srpt_grant_entry_t(0, 99, 0, ACTIVE);
    srpt_queue.push(e0);
    srpt_grant_entry_t e1 = srpt_queue.head();
    srpt_queue.pop();
    TEST_T(e1 == e0) 
  }

  {
    srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Two Element Simple Ordering: ";
    srpt_grant_entry_t e0 = srpt_grant_entry_t(0, 99, 0, ACTIVE);
    srpt_grant_entry_t e1 = srpt_grant_entry_t(1, 1, 0, ACTIVE);
    srpt_queue.push(e0);
    srpt_queue.push(e1);

    srpt_grant_entry_t e2 = srpt_queue.head();
    srpt_queue.pop();

    srpt_grant_entry_t e3 = srpt_queue.head();
    srpt_queue.pop();

    TEST_T(e2 == e1 && e3 == e0)
  }

  {
    srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Two Element OOO: ";
    srpt_grant_entry_t e0 = srpt_grant_entry_t(0, 99, 0, ACTIVE);
    srpt_grant_entry_t e1 = srpt_grant_entry_t(1, 1, 0, ACTIVE);
    srpt_queue.push(e1);
    srpt_queue.push(e0);

    srpt_grant_entry_t e2 = srpt_queue.head();
    srpt_queue.pop();

    srpt_grant_entry_t e3 = srpt_queue.head();
    srpt_queue.pop();

    TEST_T(e2 == e1 && e3 == e0)
  }

  {
    bool pass = true;
    srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Misordered Flood: ";
    for (uint32_t i = 0; i < MAX_SRPT; ++i) {
      srpt_grant_entry_t e = srpt_grant_entry_t(i,i,0,ACTIVE);
      srpt_queue.push(e);
    }

    for (uint32_t i = 0; i < MAX_SRPT; ++i) {
      srpt_grant_entry_t e = srpt_grant_entry_t(i,i,0,ACTIVE);
      srpt_grant_entry_t c = srpt_queue.head();
      srpt_queue.pop();
      if (!(c == e)) { pass = false; }
    }

    TEST_T(pass)
  }

  {
    bool pass = true;
    srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Misordered Exceed Bound: ";
    for (uint32_t i = 0; i < MAX_SRPT+1; ++i) {
      srpt_grant_entry_t e = srpt_grant_entry_t(i,i+100,0,ACTIVE);
      srpt_queue.push(e);
    }

    for (uint32_t i = 0; i < MAX_SRPT; ++i) {
      srpt_grant_entry_t e = srpt_grant_entry_t(i,i+10,0,ACTIVE);
      srpt_queue.push(e);
    }

    if (srpt_queue.get_size() != MAX_SRPT) pass = false;

    for (uint32_t i = 0; i < MAX_SRPT; ++i) {
      srpt_grant_entry_t e = srpt_grant_entry_t(i,i+10,0,ACTIVE);
      srpt_grant_entry_t c = srpt_queue.head();
      srpt_queue.pop();
      if (!(c == e)) { pass = false; }
    }

    TEST_T(pass)
  }

  {
    bool pass = true;
    srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Block Msg: ";
    for (uint32_t i = 0; i < 10; ++i) {
      srpt_grant_entry_t e = srpt_grant_entry_t(0, i+9, 0, ACTIVE);
      srpt_queue.push(e);
    }

    srpt_grant_entry_t msg = srpt_grant_entry_t(0, 0, 0xDEADBEEF, MSG);
    srpt_queue.push(msg);

    for (uint32_t i = 0; i < 10; ++i) {
      srpt_grant_entry_t e = srpt_grant_entry_t(0, i+9, 0, BLOCKED);
      srpt_grant_entry_t c = srpt_queue.head();
      srpt_queue.pop();
      if (!(c == e)) { pass = false; }
    }

    TEST_T(pass)
  }

  {
    bool pass = true;
    srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Block Peer Msg: ";
    for (uint32_t i = 0; i < 10; ++i) {
      srpt_grant_entry_t e = srpt_grant_entry_t(i, i+9, 0, ACTIVE);
      srpt_queue.push(e);
    }

    srpt_grant_entry_t msg = srpt_grant_entry_t(0, 0, 0xDEADBEEF, MSG);
    srpt_queue.push(msg);

    srpt_queue.order();
    srpt_queue.order();

    for (uint32_t i = 1; i < 10; ++i) {
      srpt_grant_entry_t e = srpt_grant_entry_t(i, i+9, 0, ACTIVE);
      srpt_grant_entry_t c = srpt_queue.head();
      srpt_queue.pop();
      if (!(c == e)) { pass = false; }
    }

    srpt_grant_entry_t e = srpt_grant_entry_t(0, 9, 0, BLOCKED);
    srpt_grant_entry_t c = srpt_queue.head();
    srpt_queue.pop();
    if (!(c == e)) { pass = false; }

    TEST_T(pass)
  }

  {
    bool pass = true;
    srpt_queue_t<srpt_grant_entry_t, MAX_SRPT> srpt_queue;
    std::cerr << "Testing Block Double Msg: ";
    for (uint32_t i = 0; i < 10; ++i) {
      srpt_grant_entry_t e = srpt_grant_entry_t(0, i+9, 0, ACTIVE);
      srpt_queue.push(e);
    }

    srpt_grant_entry_t msg = srpt_grant_entry_t(0, 0, 0xDEADBEEF, MSG);
    srpt_queue.push(msg);

    for (uint32_t i = 0; i < 10; ++i) {
      srpt_grant_entry_t e = srpt_grant_entry_t(0, i+9, 0, BLOCKED);
      srpt_grant_entry_t c = srpt_queue.head();
      srpt_queue.pop();
      if (!(c == e)) { pass = false; }
    }

    TEST_T(pass)
  }
    
  return 0;
}

int main() {
  std::cerr << "****************************** START TEST BENCH ******************************" << endl;

  /* Unit Tests */
  if (test_unit_xmit_srpt_queue()) { return 1; }
  if (test_unit_grant_srpt_queue()) { return 1; }

  std::cerr << "******************************  END TEST BENCH  ******************************" << endl;

  return 0;
}



