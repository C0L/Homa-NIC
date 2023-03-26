#include <iostream>
#include "ap_axi_sdata.h"
#include "hls_stream.h"
using namespace std;

//typedef hls::axis<ap_uint<12000>, 0, 0, 0> pkt;

void homa(hls::stream<pkt> & ingress, hls::stream<pkt> & egress);

// How to stream multipl packets?
int main() {
  hls::stream<pkt> input, output;
  pkt tmp1, tmp2;

  auto big = 0x1000000000000;
  tmp1.data = big;
  input.write(tmp1);

  homa(input,output);

  output.read(tmp2);

  cout << "Size: " << tmp2.data << endl;

  if (tmp2.data != big) {
    cout << "ERROR: results mismatch" << endl;
    return 1;
  } else {
    cout << "Success: results match" << endl;
    return 0;
  }
}


// TODO write homa_send()
// TODO write homa_rec()
