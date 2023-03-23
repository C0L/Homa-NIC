#include <iostream>
#include "ap_axi_sdata.h"
#include "hls_stream.h"
using namespace std;

typedef hls::axis<ap_uint<12000>, 0, 0, 0> pkt;

void homa(hls::stream<pkt > & ingress, hls::stream<pkt> & egress);
// How to stream multipl packets?
int main() {
  hls::stream<pkt> input, output;
  pkt tmp1, tmp2;

  tmp1.data = 100;
  
  input.write(tmp1);
  homa(input,output);
  output.read(tmp2);

  if (tmp2.data != 100) {
    cout << "ERROR: results mismatch" << endl;
    return 1;
  } else {
    cout << "Success: results match" << endl;
    return 0;
  }
}
