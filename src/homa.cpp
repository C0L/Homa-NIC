#include <ap_int.h>
#include <hls_stream.h>
#include "ap_axi_sdata.h"

// Ethernet header
//struct eth_header_t {
//  ap_uint<48>     mac_dest;
//  ap_uint<48>     mac_src;
//  ap_uint<32>     tag;
//  ap_uint<16>     ethertype;
//}
//
//// Ethernet frame, (CRC included in 
//struct eth_frame_t {
//  eth_header_t  header;
//  ap_uint<12032> paylod;
//  ap_uint<1>     tlast;  // Dont use tlast in Vitis???
//};


// https://discuss.pynq.io/t/tutorial-pynq-dma-part-1-hardware-design/3133

// template<int D,int U,int TI,int TD> for ethernet packet? input the payload size?
//struct ap_axiu{
//
//ap_uint<D> data;
//
//ap_uint<(D\+7)/8> keep;
//
//ap_uint<(D\+7)/8> strb;
//
//ap_uint<U> user;
//
//ap_uint<1> last;
//
//ap_uint<TI> id;
//
//ap_uint<TD> dest;
//
//};

// AXI4-Stream is a protocol designed for transporting arbitrary unidirectional data. In an AXI4-Stream, TDATA width of bits is transferred per clock cycle. The transfer is started once the producer sends the TVALID signal and the consumer responds by sending the TREADY signal (once it has consumed the initial TDATA). At this point, the producer will start sending TDATA and TLAST (TUSER if needed to carry additional user-defined sideband data). TLAST signals the last byte of the stream. So the consumer keeps consuming the incoming TDATA until TLAST is asserted.

// https://docs.xilinx.com/r/en-US/ug1399-vitis-hls/AXI4-Stream-Interfaces-without-Side-Channels

// tlast corresponds to the TLAST signal of the AXI Stream prtocol. This indicates when the message is complete (the DMA transaction complete, the ethernet frame complete, etc). If all transfers were constant length we would not need this. They are not of course

// TODO why tlast
// TODO can we early exit if the packet is short?


//void example(hls::stream<ap_axis<32,2,5,6>> &A,
//hls::stream< ap_axis<32,2,5,6> > &B)

// TODO need to make sure using PPB
// A Ping Pong Buffer is a double buffer that is used to speed up a process that can overlap the I/O operation with the data processing operation. One buffer is used to hold a block of data so that a consumer process will see a complete (but old) version of the data, while in the other buffer a producer process is creating a new (partial) version of data. When the new block of data is complete and valid, the consumer and the producer processes will alternate access to the two buffers. As a result, the usage of a ping-pong buffer increases the overall throughput of a device and helps to prevent eventual bottlenecks. The key advantage of PIPOs is that the tool automatically matches the rate of production vs the rate of consumption and creates a channel of communication that is both high performance and is deadlock free.


typedef hls::axis<ap_uint<12000>, 0, 0, 0> pkt;

// Ethernet frame in -> Ethernet frame out
//void homa(hls::stream<pkt> & ingress, hls::stream<eth_frame_t> & egress, hls::stream<message> & messages ) {

void homa(hls::stream<pkt> & ingress, hls::stream<pkt> & egress) {
#pragma HLS INTERFACE axis port=ingress
#pragma HLS INTERFACE axis port=egress
#pragma HLS INTERFACE axis port=messages

  // persistant state here
  //ap_uint<256>[max_headers]

  //while( 1 ) {
    // Will block until tlast
  pkt tmp;
  ingress.read( tmp );
  tmp.data++;
  egress.write( tmp );
	//#pragam HLS DATAFLOW?

	//process_ingress();
	//process_egress();
	//process_messages();
      //}
    // use blocking read to get full packet

  // TODO can we cast the ethernet frame based on the size of payload?
  //ap_int<payload_size>
}
