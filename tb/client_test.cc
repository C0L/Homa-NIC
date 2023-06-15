#include <cstdio>
#include <chrono>
#include <thread>

#include <sstream>
#include <iomanip>

#include "ap_axi_sdata.h"
#include "hls_stream.h"

#include "homa.hh"
#include "dma.hh"

using namespace std;

#define TEST_T(x) if (x) { std::cerr << "PASS\n"; } else { std::cerr << "FAIL\n"; return 1;}

void print_packet(hls::stream<raw_stream_t> & link_egress) {
  char packet[1522];

  raw_stream_t block;
  block.last = 0;

  int i = 0;
  while (!block.last) {
    std::cerr << "DATA READ\n";
    link_egress.read(block);
    std::cout << std::hex << block.data << std::endl;
    dbuff_chunk_t big_order;
    byte_order_flip(block.data, big_order);
    char * bd = ((char*)&big_order);
    //printf("%02x\n", bd[0]);
    //printf("%02x\n", bd[1]);
    memcpy(packet + (i * 64), bd, 64);
    i++;
  }
  //std::cout << "PACKET\n";
  //for (int i = 0; i < 1522; ++i) {
  //  printf("%02x", packet[i]);
  //}
  //std::cout << std::endl;


  std::cout << packet + 114 << std::endl;
}


bool test_recvmsg_client(homa_t * homa_cfg,
			 sendmsg_t * sendmsg,
			 recvmsg_t * recvmsg,
			 hls::stream<raw_stream_t> & link_ingress,
			 hls::stream<raw_stream_t> & link_egress,
			 char * maxi_in,
			 char * maxi_out) {

  ap_uint<128> saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
  ap_uint<128> daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);

  uint16_t sport;
  uint16_t dport;

  dport = 0xBEEF;
  sport = 0xFEEB;

  ap_uint<512> c0("AAAAAAAAAAAABBBBBBBBBBBB86DD600EEEEE007DFD00DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBAABCDEFABCDEFABCDABCDEFABCDEFABCDFEEBBEEFFFFFFFFFFFFF", 16);
  ap_uint<512> c1("FFFFA010FFFF0000FFFF00000000000000000000004100000000FFFFFFFF0000000000000041FFFFFFFFFFFFFFFFFFFFFFFFDEADBEEFDEADBEEFDEADBEEFDEAD", 16);
  ap_uint<512> c2("BEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDE00000000000000000000000000", 16);

  raw_stream_t chunk_0;
  chunk_0.data = c0;
  chunk_0.last = 0;

  raw_stream_t chunk_1;
  chunk_1.data = c1;
  chunk_1.last = 0;

  raw_stream_t chunk_2;
  chunk_2.data = c2;
  chunk_2.last = 1;

  // Offset in DMA space, receiver address, sender address, receiver port, sender port, RPC ID (0 for match-all)
  //recvmsg_t newrecvmsg = {0, daddr, saddr, dport, sport, 0, 1, 0, 0};
  recvmsg->buffout = 0;
  recvmsg->saddr = saddr;
  recvmsg->daddr = daddr;
  recvmsg->sport = sport;
  recvmsg->dport = dport;
  recvmsg->id = 0;
  recvmsg->valid = 1;

  // On board the recvmsg command
  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, (char *) maxi_out);

  recvmsg->valid = 0;

  // Wait for recv call to settle
  std::chrono::seconds dura(5);
  std::this_thread::sleep_for(dura);

  // Recieve the packet
  link_ingress.write(chunk_0);
  link_ingress.write(chunk_1);
  link_ingress.write(chunk_2);

  std::this_thread::sleep_for(dura);

  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, (char*) maxi_out);
  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, (char*) maxi_out);

  std::cout << "POST: " << std::hex << *((dbuff_chunk_t*) maxi_out) << std::endl;

  return 0;
}


bool test_sendmsg_client(homa_t * homa_cfg,
	  sendmsg_t * sendmsg,
	  recvmsg_t * recvmsg,
	  hls::stream<raw_stream_t> & link_ingress,
	  hls::stream<raw_stream_t> & link_egress,
	  char * maxi_in,
	  char * maxi_out) {

  homa_cfg->rtt_bytes = 60000;

  std::string str = "here is somed data string....sdjflkdsjflksjdflkdsjflkdsjflkdsjflskjdlfksjdlfksjdlfkjdslfkjdslkfjdslkvnknvknfmvn,mfndv,mfvmnsvmnfdkmvndfkvjnfdkjvnfdkjvndfkjnvkfdjnvkjfdnvknfdvknfdkvndfkjvnfdkjnvdkfjnvkjmmsdlkjflkdsjflskdjflksjdflksjflkdsjflksjdlfkj";

  strcpy(maxi_in, str.c_str());
  std::cout << maxi_in << std::endl;

  for (int i = 0; i < str.length(); ++i) {
    printf("%02x", maxi_in[i]);
  }
  std::cout << std::endl;

  ap_uint<128> saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
  ap_uint<128> daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);

  uint16_t sport;
  uint16_t dport;

  dport = 0xBEEF;
  sport = 0xFEEB;

  sendmsg->buffin = 0;
  //sendmsg->length = 65;
  //std::cerr << str.length();
  sendmsg->length = str.length();
  sendmsg->saddr = saddr;
  sendmsg->daddr = daddr;
  sendmsg->sport = sport;
  sendmsg->dport = dport;
  sendmsg->id = 0;
  sendmsg->completion_cookie = 0xFFFFFFFFFFFFFFFF;
  sendmsg->valid = 1;

  // Construct a new RPC to ingest  
  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, maxi_out);

  std::chrono::seconds dura(5);
  std::this_thread::sleep_for(dura);

  sendmsg->valid = 0;

  // Cycle DMA transfers
  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, maxi_out);

  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, maxi_out);

  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, maxi_out);

  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, maxi_out);
  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, maxi_out);
  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, maxi_out);
  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, maxi_out);
  homa(homa_cfg, sendmsg, recvmsg, link_ingress, link_egress, maxi_in, maxi_out);

  print_packet(link_egress);
  
  return 0;
}

int main() {
  std::cerr << "****************************** START TEST BENCH ******************************" << endl;

  hls::stream<raw_stream_t> link_ingress;
  hls::stream<raw_stream_t> link_egress;

  homa_t homa_cfg;

  sendmsg_t sendmsg;
  recvmsg_t recvmsg;

  char maxi_in[128*64];
  char maxi_out[128*64];

    if (test_sendmsg_client(&homa_cfg, &sendmsg, &recvmsg, link_ingress, link_egress, maxi_in, maxi_out)) { return 1; }

    //if (test_recvmsg_client(&homa_cfg, &sendmsg, &recvmsg, link_ingress, link_egress, maxi_in, maxi_out)) { return 1; }
  
  std::cerr << "******************************  END TEST BENCH  ******************************" << endl;

  return 0;
}
