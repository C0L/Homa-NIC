#include <iostream>
#include <cstdio>
#include "ap_axi_sdata.h"
#include "hls_stream.h"

#include "homa.hh"
#include "dma.hh"
#include "link.hh"

using namespace std;

void test_unit_rpc_stack() {
  /** Check Initalization */
  {
    std::cerr << "Unit Test: RPC Stack Check Initialization" << endl;
    bool passed = true;
    rpc_stack_t rpc_stack;

    for (int id = 0; id < MAX_RPCS; ++id) {
      if (rpc_stack.pop() != id) passed = false;
    }

    if (passed) {
      std::cerr << "Result: PASSED" << endl;
    } else {
      std::cerr << "Result: FAILED" << endl;
    }
  }

  /** Check Size */
  {
    std::cerr << "Unit Test: RPC Stack Check Size" << endl;
    rpc_stack_t rpc_stack;
    bool passed = true;

    for (int id = 0; id < MAX_RPCS; ++id) {
      if (rpc_stack.get_size() != MAX_RPCS - id) passed = false;
      rpc_stack.pop();
    }

    if (passed) {
      std::cerr << "Result: PASSED" << endl;
    } else {
      std::cerr << "Result: FAILED" << endl;
    }
  }

  /** Push and Pop */
  {
    std::cerr << "Unit Test: RPC Stack Push/Pop" << endl;
    rpc_stack_t rpc_stack;
    bool passed = true;

    for (int id = 0; id < MAX_RPCS; ++id) {
      rpc_stack.pop();
    }

    if (rpc_stack.get_size() != 0) passed = false;

    rpc_stack.push(99);
    rpc_stack.push(200);
    rpc_stack.push(949);
    rpc_stack.push(234);
    rpc_stack.push(1);
    rpc_stack.push(0);
    rpc_stack.push(1023);

    if (rpc_stack.pop() != 1023) passed = false;
    if (rpc_stack.pop() != 0) passed = false;
    if (rpc_stack.pop() != 1) passed = false;
    if (rpc_stack.pop() != 234) passed = false;
    if (rpc_stack.pop() != 949) passed = false;
    if (rpc_stack.pop() != 200) passed = false;
    if (rpc_stack.pop() != 99) passed = false;

    if (passed) {
      std::cerr << "Result: PASSED" << endl;
    } else {
      std::cerr << "Result: FAILED" << endl;
    }
  }
}

void test_unit_srpt_queue() {
  /** Push and Pop */
  {
    std::cerr << "Unit Test: SRPT Queue Single Push/Pop" << endl;
    bool passed = true;
    srpt_queue_t srpt_queue;

    srpt_queue.push(99, 0);

    if (srpt_queue.pop() != 99) passed = false;

    if (passed) {
      std::cerr << "Result: PASSED" << endl;
    } else {
      std::cerr << "Result: FAILED" << endl;
    }
  }

  /** Double Push and Pop */
  {
    std::cerr << "Unit Test: SRPT Queue Double Push/Pop" << endl;
    bool passed = true;
    srpt_queue_t srpt_queue;

    srpt_queue.push(2, 0);
    srpt_queue.push(1, 10);

    if (srpt_queue.pop() != 2) passed = false;
    if (srpt_queue.pop() != 1) passed = false;

    if (passed) {
      std::cerr << "Result: PASSED" << endl;
    } else {
      std::cerr << "Result: FAILED" << endl;
    }
  }

  /** Multi Push and Pop */
  {
    std::cerr << "Unit Test: SRPT Queue Multi Push/Pop" << endl;
    bool passed = true;
    srpt_queue_t srpt_queue;

    srpt_queue.push(10, 10);
    srpt_queue.push(1, 1);
    srpt_queue.push(2, 2);
    srpt_queue.push(18, 18);
    srpt_queue.push(3, 3);
    srpt_queue.push(11, 11);
    srpt_queue.push(9, 9);
    srpt_queue.push(4, 4);
    srpt_queue.push(5, 5);
    srpt_queue.push(13, 13);
    srpt_queue.push(23, 23);
    srpt_queue.push(14, 14);
    srpt_queue.push(15, 15);
    srpt_queue.push(7, 7);
    srpt_queue.push(8, 8);
    srpt_queue.push(16, 16);
    srpt_queue.push(17, 17);
    srpt_queue.push(20, 20);
    srpt_queue.push(21, 21);
    srpt_queue.push(19, 19);
    srpt_queue.push(6, 6);
    srpt_queue.push(24, 24);
    srpt_queue.push(12, 12);
    srpt_queue.push(22, 22);

    if (srpt_queue.pop() != 1) passed = false;
    if (srpt_queue.pop() != 2) passed = false;
    if (srpt_queue.pop() != 3) passed = false;
    if (srpt_queue.pop() != 4) passed = false;
    if (srpt_queue.pop() != 5) passed = false;
    if (srpt_queue.pop() != 6) passed = false;
    if (srpt_queue.pop() != 7) passed = false;
    if (srpt_queue.pop() != 8) passed = false;
    if (srpt_queue.pop() != 9) passed = false;
    if (srpt_queue.pop() != 10) passed = false;
    if (srpt_queue.pop() != 11) passed = false;
    if (srpt_queue.pop() != 12) passed = false;
    if (srpt_queue.pop() != 13) passed = false;
    if (srpt_queue.pop() != 14) passed = false;
    if (srpt_queue.pop() != 15) passed = false;
    if (srpt_queue.pop() != 16) passed = false;
    if (srpt_queue.pop() != 17) passed = false;
    if (srpt_queue.pop() != 18) passed = false;
    if (srpt_queue.pop() != 19) passed = false;
    if (srpt_queue.pop() != 20) passed = false;
    if (srpt_queue.pop() != 21) passed = false;
    if (srpt_queue.pop() != 22) passed = false;
    if (srpt_queue.pop() != 23) passed = false;

    if (passed) {
      std::cerr << "Result: PASSED" << endl;
    } else {
      std::cerr << "Result: FAILED" << endl;
    }
  }
}


void test_unit_proc_dma_ingress() {
  /**
   * Single Cacheable DMA Ingress
   * TODO add some description here 
   */
  {
    std::cerr << "Unit Test: Cacheable DMA Ingress" << endl;
    bool passed = true;

    hls::stream<user_input_t> dma_ingress;

    homa_rpc_t * rpcs = new homa_rpc_t[MAX_RPCS];
    char * ddr_ram = new char[HOMA_MAX_MESSAGE_LENGTH * MAX_RPCS];
    user_output_t * dma_egress = new user_output_t[MAX_RPCS];

    rpc_stack_t rpc_stack;
    srpt_queue_t srpt_queue;

    // TODO create reusable function for parametizable artifical message generation
    user_input_t user_input;
    user_input.dport = 99;
    user_input.output_slot = 0;
    user_input.length = ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER;
    for (int i = 0; i < ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER; i+=4) *((int*) &user_input.message[i]) = 0xDEADBEEF;

    dma_ingress.write(user_input);

    proc_dma_ingress(dma_ingress, rpcs, ddr_ram, dma_egress, rpc_stack, srpt_queue);

    // TODO comment what we are checking
    if (rpc_stack.size != MAX_RPCS - 1) passed = false;
    if (srpt_queue.size != 1) passed = false;
    if (srpt_queue.pop() != 0) passed = false;
    if (rpcs[0].dport != 99) passed = false;
    if (rpcs[0].homa_message_out.length != (ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER)) passed = false;
    
    for (int i = 0; i < ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER-2; i+=4) {
      if (*((int*) &rpcs[0].homa_message_out.message[i]) != 0xDEADBEEF) {
	std::cerr << i << endl;
	passed = false;
      } 
    }

    if (rpcs[0].homa_message_in.dma_out != dma_egress) passed = false;
    if (rpcs[0].homa_message_in.dma_out->rpc_id != 0) passed = false;

    delete[] rpcs;
    delete[] ddr_ram;
    delete[] dma_egress;

    if (passed) {
      std::cerr << "Result: PASSED" << endl;
    } else {
      std::cerr << "Result: FAILED" << endl;
    }
  }

  // TODO add tests when ingress is not full or there are no availible rpcs
}

// TODO not fully implemented
void test_functional_send_message() {
  std::cerr << "Functional Test: Simple Send Message" << endl;

  unsigned char ddr_ram[HOMA_MAX_MESSAGE_LENGTH * 1024];
  user_output_t dma_egress[HOMA_MAX_MESSAGE_LENGTH * 1024];

  /**
   * Simple Send Message
   *   A single packets worth of data is transfered to the Homa core
   *   The core should package the message into a packet 
   */
  {
    std::cerr << "Functional Test: Send Message" << endl;
    bool passed = true;

    /** Buffers for incoming and outgoing frames */
    //raw_frame_t in_frame, out_frame;
    //hls::stream<raw_frame_t> link_ingress, link_egress;

    //user_input_t user_in;
    //hls::stream<user_input_t> dma_ingress;

    /** Message the user wishes to send (1 packet) */
    //user_in.output_slot = 0;
    //user_in.length = ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER;
    //for (int i = 0; i < ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER; i+=4) *((int*) &user_in.message[i]) = 0xDEADBEEF;

    //dma_ingress.write(user_in);

    //homa(link_ingress, link_egress, dma_ingress, ddr_ram, dma_egress);

    // TODO 
    
    if (passed) {
      std::cerr << "Result: PASSED" << endl;
    } else {
      std::cerr << "Result: FAILED" << endl;
    }
  }
}

void test_functional_rec_message() {
}

int main() {
  std::cerr << "****************************** START TEST BENCH ******************************" << endl;

  /** Unit Tests */
  test_unit_rpc_stack();
  test_unit_srpt_queue();

  //test_unit_proc_link_egress();
  //test_unit_proc_link_ingress();
  test_unit_proc_dma_ingress();

  /** Functional Tests */
  //test_functional_send_message();

  std::cerr << "******************************  END TEST BENCH  ******************************" << endl;

  // TODO make this actually fail when tests don't pass
  return 0;
}
