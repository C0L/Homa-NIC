#include <iostream>
#include <cstdio>
#include "ap_axi_sdata.h"
#include "hls_stream.h"

#include "homa.hh"
#include "dma.hh"
#include "rpcmgmt.hh"
#include "peer.hh"

using namespace std;

void homa_init(homa_t & homa) {
  homa.rtt_bytes = 10000;
  homa.link_mbps = 10000;
  homa.num_priorities = HOMA_MAX_PRIORITIES;
  for (int i = 0; i < HOMA_MAX_PRIORITIES; i++)
    homa.priority_map[i] = i;
  homa.max_sched_prio = HOMA_MAX_PRIORITIES - 5;
  homa.unsched_cutoffs[HOMA_MAX_PRIORITIES-1] = 200;
  homa.unsched_cutoffs[HOMA_MAX_PRIORITIES-2] = 2800;
  homa.unsched_cutoffs[HOMA_MAX_PRIORITIES-3] = 15000;
  homa.unsched_cutoffs[HOMA_MAX_PRIORITIES-4] = HOMA_MAX_MESSAGE_LENGTH;
  homa.cutoff_version = 1;
  homa.duty_cycle = 800;
  homa.grant_threshold = 0;
  homa.max_overcommit = 8;
  homa.max_incoming = 0;
  homa.resend_ticks = 15;
  homa.resend_interval = 10;
  homa.timeout_resends = 5;
  homa.request_ack_ticks = 2;
  homa.reap_limit = 10;
  homa.dead_buffs_limit = 5000;
  homa.max_dead_buffs = 0;
  homa.timer_ticks = 0;
  homa.flags = 0;
}

void test_unit_proc_dma_ingress() {
  /**
   * Simple cacheable request
   */
  {
    std::cerr << "Unit Test: Single cacheable message dma ingress" << endl;

    bool passed = true;
    homa_t homa;
    homa_init(homa);

    hls::stream<dma_in_t> dma_ingress;

    ap_uint<512> * dma = new ap_uint<512>[31250];

    unsigned char addr[16] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
			      0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF};

    dma_in_t dma_in;
    dma_in.buffout = 1000000;
    dma_in.buffin  = 0;
    memcpy(dma_in.dest_addr.s6_addr, addr, 16);
    dma_in.length = ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER;
    dma_in.id = 0;
    dma_in.completion_cookie = 0xDEADBEEF;
    
    // Message the user wishes to send (1 packet)
    memset(dma, 0xDEADBEEF, ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER);
    
    dma_ingress.write(dma_in);

    proc_dma_ingress(&homa, dma_ingress, dma);

    // Check that a new RPC was popped
    if (rpc_stack.size != MAX_RPCS - 1) {
      std::cerr << "FAILURE: Invalid rpc_stack.size " << rpc_stack.size << endl;
      passed = false;
    }

    // Check that the RPC has the correct output offset
    if (rpcs[0].buffout != 1000000) {
      std::cerr << "FAILURE: Invalid rpcs[0].buffout " << rpcs[0].buffout << endl;
      passed = false;
    }

    // Check that the RPC has the correct input offset
    if (rpcs[0].buffin != 0) {
      std::cerr << "FAILURE: Invalid rpcs[0].buffin " << rpcs[0].buffin << endl;
      passed = false;
    }

    // Check that the state of the RPC was correctly set
    if (rpcs[0].state != homa_rpc_t::RPC_OUTGOING) {
      std::cerr << "FAILURE: Invalid rpcs[0].state " << rpcs[0].state << endl;
      passed = false;
    }

    // Check that the RPC has the correct input offset
    if (rpcs[0].flags != 0) { 
      std::cerr << "FAILURE: Invalid rpcs[0].flags " << rpcs[0].flags << endl;
      passed = false;
    }

    // Check that the RPC has the correct input offset
    if (rpcs[0].grants_in_progress != 0) { 
      std::cerr << "FAILURE: Invalid rpcs[0].flags " << rpcs[0].flags << endl;
      passed = false;
    }

    // Check target address
    if (memcmp(peers[rpcs[0].peer].addr.s6_addr, &addr, 16)) {
      passed = false;
      std::cerr << "FAILURE: Invalid peers[rpcs[0].peer].addr content" << endl;
    }

    // TODO check dport, or has that been removed?

    // Check the id is equal to the new RPC (1)
    if (rpcs[0].id != 1) {
      std::cerr << "FAILURE: Invalid rpcs[0].id " << rpcs[0].id << endl;
      passed = false;
    }

    // Check completion cookie was recorded
    if (rpcs[0].completion_cookie != 0xDEADBEEF) {
      std::cerr << "FAILURE: Invalid rpcs[0].completion_cookie " << rpcs[0].completion_cookie << endl;
      passed = false;
    }

    // Check if message to broadcast has correct length set
    if (rpcs[0].msgout.length != ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER) {
      std::cerr << "FAILURE: Invalid rpcs[0].msgout.length " << rpcs[0].msgout.length << endl;
      passed = false;
    }

    // Check if on chip cache correctly populated
    if (memcmp(dma, rpcs[0].msgout.message, HOMA_MESSAGE_CACHE)) {
      passed = false;
      std::cerr << "FAILURE: Invalid rpcs[0].msgout.message content " << endl;
    }

    delete[] dma;

    if (passed) {
      std::cerr << "Result: PASSED" << endl;
    } else {
      std::cerr << "Result: FAILED" << endl;
    }

  }
}


int main() {
  std::cerr << "****************************** START TEST BENCH ******************************" << endl;

  /** Unit Tests */
  test_unit_proc_dma_ingress();

  /** Functional Tests */

  std::cerr << "******************************  END TEST BENCH  ******************************" << endl;

  // TODO make this actually fail when tests don't pass
  return 0;
}



















// TODO not fully implemented
//void test_functional_send_message() {
//  std::cerr << "Functional Test: Simple Send Message" << endl;
//
//  /**
//   * Simple Send Message
//   *   A single packets worth of data is transfered to the Homa core
//   *   The core should package the message into a packet 
//   */
//  {
//    std::cerr << "Functional Test: Send Message" << endl;
//    bool passed = true;
//
//    /** Buffers for incoming and outgoing frames */
//    //raw_frame_t in_frame, out_frame;
//    //hls::stream<raw_frame_t> link_ingress, link_egress;
//
//    //user_input_t user_in;
//    //hls::stream<user_input_t> dma_ingress;
//
//    /** Message the user wishes to send (1 packet) */
//    //user_in.output_slot = 0;
//    //user_in.length = ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER;
//    //for (int i = 0; i < ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER; i+=4) *((int*) &user_in.message[i]) = 0xDEADBEEF;
//
//    //dma_ingress.write(user_in);
//
//    //homa(link_ingress, link_egress, dma_ingress, ddr_ram, dma_egress);
//
//    // TODO 
//    
//    if (passed) {
//      std::cerr << "Result: PASSED" << endl;
//    } else {
//      std::cerr << "Result: FAILED" << endl;
//    }
//  }
//}

//void test_unit_homa_rpc_new_client() {
//  /** Test new rpc client generation */
//  {
//    std::cerr << "Unit Test: homa_rpc_new_client()" << endl;
//    bool passed = true;
//
//    homa_rpc_id_t homa_rpc_id = homa_rpc_new_client(0, 0xDEADBEEFDEADBEEF);
//    
//    std::cerr << homa_rpc_id << endl;
//
//    if (passed) {
//      std::cerr << "Result: PASSED" << endl;
//    } else {
//      std::cerr << "Result: FAILED" << endl;
//    }
//  }
//}

//void test_unit_homa_peer_find() {
//  /** Test peer finding */
//  {
//    std::cerr << "Unit Test: homa_peer_find()" << endl;
//    bool passed = true;
//
//    if (passed) {
//      std::cerr << "Result: PASSED" << endl;
//    } else {
//      std::cerr << "Result: FAILED" << endl;
//    }
//  }
//}


//void test_unit_proc_dma_ingress() {
//  /**
//   * Single Cacheable DMA Ingress
//   * TODO add some description here 
//   */
//  {
//    std::cerr << "Unit Test: Cacheable DMA Ingress" << endl;
//    bool passed = true;
//
//    hls::stream<user_input_t> dma_ingress;
//
//    homa_rpc_t * rpcs = new homa_rpc_t[MAX_RPCS];
//    char * ddr_ram = new char[HOMA_MAX_MESSAGE_LENGTH * MAX_RPCS];
//    user_output_t * dma_egress = new user_output_t[MAX_RPCS];
//
//    rpc_stack_t rpc_stack;
//    srpt_queue_t srpt_queue;
//
//    // TODO create reusable function for parametizable artifical message generation
//    user_input_t user_input;
//    user_input.dport = 99;
//    user_input.output_slot = 0;
//    user_input.length = ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER;
//    for (int i = 0; i < ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER; i+=4) *((int*) &user_input.message[i]) = 0xDEADBEEF;
//
//    dma_ingress.write(user_input);
//
//    proc_dma_ingress(dma_ingress, rpcs, ddr_ram, dma_egress, rpc_stack, srpt_queue);
//
//    // TODO comment what we are checking
//    if (rpc_stack.size != MAX_RPCS - 1) passed = false;
//    if (srpt_queue.size != 1) passed = false;
//    if (srpt_queue.pop() != 0) passed = false;
//    if (rpcs[0].dport != 99) passed = false;
//    if (rpcs[0].homa_message_out.length != (ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER)) passed = false;
//    
//    for (int i = 0; i < ETHERNET_MAX_PAYLOAD - HOMA_MAX_HEADER-2; i+=4) {
//      if (*((int*) &rpcs[0].homa_message_out.message[i]) != 0xDEADBEEF) {
//	std::cerr << i << endl;
//	passed = false;
//      } 
//    }
//
//    if (rpcs[0].homa_message_in.dma_out != dma_egress) passed = false;
//    if (rpcs[0].homa_message_in.dma_out->rpc_id != 0) passed = false;
//
//    delete[] rpcs;
//    delete[] ddr_ram;
//    delete[] dma_egress;
//
//    if (passed) {
//      std::cerr << "Result: PASSED" << endl;
//    } else {
//      std::cerr << "Result: FAILED" << endl;
//    }
//  }
//
//  // TODO add tests when ingress is not full or there are no availible rpcs
//
//  // TODO add multiple message tests
//}
