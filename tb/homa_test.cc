#include <iostream>
#include "ap_axi_sdata.h"
#include "hls_stream.h"

#include "homa.hh"

using namespace std;

void test_unit_rpc_queue() {
  /** Check Initalization */
  {
    std::cerr << "Unit Test: RPC Queue Check Initialization" << endl;
    bool pass = true;
    rpc_queue_t rpc_queue();

    for (rpc_id_t id = 0; id < MAX_RPCS; ++id) {
      if (rpc_queue.pop() != id) pass = false;
    }

    if (passed) {
      std::err << "Result: PASSED" << endl;
    } else {
      std::err << "Result: FAILED" << endl;
    }
  }

  /** Check Size */
  {
    std::cerr << "Unit Test: RPC Queue Check Size" << endl;
    rpc_queue_t rpc_queue();

    for (rpc_id_t id = 0; id < MAX_RPCS; ++id) {
      rpc_queue.pop();
    }

    if (rpc_queue.pop() == 0) {
      std::err << "Result: PASSED" << endl;
    } else {
      std::err << "Result: FAILED" << endl;
    }
  }

  /** Push and Pop Single Value */
  {
    std::cerr << "Unit Test: RPC Queue push and pop single value" << endl;
    rpc_queue_t rpc_queue();

    for (rpc_id_t id = 0; id < MAX_RPCS; ++id) {
      rpc_queue.pop();
    }

    rpc_queue.push(99);

    if (rpc_queue.pop() == 99) {
      std::err << "Result: PASSED" << endl;
    } else {
      std::err << "Result: FAILED" << endl;
    }
  }



}

void test_unit_srpt_queue() {

}


int main() {
  std::cerr << "****************************** TEST BENCH ******************************" << endl;

  /** Unit Tests */
  test_unit_rpc_queue();
  // TODO queue test
  // TODO SRPT test


  return 1;
  /** Functional Tests */

  //hls::stream<pkt> input, output;
  //pkt tmp1, tmp2;

  //auto big = 0x1000000000000;
  //tmp1.data = big;
  //input.write(tmp1);

  //homa(input,output);

  //output.read(tmp2);

  //cout << "Size: " << tmp2.data << endl;

  //if (tmp2.data != big) {
  //  cout << "ERROR: results mismatch" << endl;
  //  return 1;
  //} else {
  //  cout << "Success: results match" << endl;
  //  return 0;
  //}
}


// TODO write homa_send()
// TODO write homa_rec()
