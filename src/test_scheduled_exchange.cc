#include "homa.hh"

using namespace std;

#define TEST_T(x) if (x) { std::cerr << "PASS\n"; } else { std::cerr << "FAIL\n"; return 1;}


const std::string data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Velit aliquet sagittis id consectetur purus. Ut porttitor leo a diam sollicitudin. Tristique senectus et netus et malesuada fames ac turpis. Et ligula ullamcorper malesuada proin libero nunc. Sit amet tellus cras adipiscing enim eu turpis. Vel eros donec ac odio tempor orci dapibus. Quam viverra orci sagittis eu volutpat odio. Enim neque volutpat ac tincidunt vitae semper. Nunc sed velit dignissim sodales. Magna eget est lorem ipsum dolor sit amet consectetur adipiscing. Ut etiam sit amet nisl purus. Tristique senectus et netus et malesuada fames ac turpis. Viverra mauris in aliquam sem fringilla ut morbi tincidunt. Suspendisse interdum consectetur libero id faucibus nisl tincidunt eget. Ipsum a arcu cursus vitae.        At quis risus sed vulputate odio ut enim blandit volutpat. A erat nam at lectus urna duis convallis convallis. Gravida quis blandit turpis cursus in hac habitasse. Commodo elit at imperdiet dui accumsan sit amet nulla. Elit pellentesque habitant morbi tristique senectus et netus et. Vulputate eu scelerisque felis imperdiet proin fermentum. Mauris in aliquam sem fringilla ut. Morbi tincidunt ornare massa eget egestas purus. At tempor commodo ullamcorper a lacus vestibulum sed arcu. Sit amet nulla facilisi morbi tempus iaculis. Morbi tristique senectus et netus et malesuada fames ac turpis. Sagittis aliquam malesuada bibendum arcu. Vivamus arcu felis bibendum ut tristique et egestas quis. Cursus sit amet dictum sit amet justo donec. Porttitor rhoncus dolor purus non enim praesent elementum facilisis. Sagittis aliquam malesuada bibendum arcu vitae elementum.       Mattis vulputate enim nulla aliquet porttitor lacus luctus accumsan. Dictum varius duis at consectetur lorem donec massa sapien faucibus. Nisi quis eleifend quam adipiscing vitae. Suspendisse faucibus interdum posuere lorem ipsum dolor sit amet consectetur. Tincidunt arcu non sodales neque sodales ut etiam. Id volutpat lacus laoreet non curabitur gravida arcu ac. Nulla facilisi cras fermentum odio eu feugiat. Fames ac turpis egestas sed tempus urna. Tristique nulla aliquet enim tortor at auctor urna. Bibendum neque egestas congue quisque egestas diam in arcu. Pharetra diam sit amet nisl. Etiam non quam lacus suspendisse faucibus. Diam sit amet nisl suscipit adipiscing bibendum est. At ultrices mi tempus imperdiet nulla malesuada pellentesque. Auctor neque vitae tempus quam pellentesque. Facilisis leo vel fringilla est ullamcorper eget nulla facilisi etiam. Morbi tempus iaculis urna id volutpat lacus laoreet non. Id semper risus in hendrerit gravida. Tincidunt dui ut ornare lectus sit. Ac odio tempor orci     Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Velit aliquet sagittis id consectetur purus. Ut porttitor leo a diam sollicitudin. Tristique senectus et netus et malesuada fames ac turpis. Et ligula ullamcorper malesuada proin libero nunc. Sit amet tellus cras adipiscing enim eu turpis. Vel eros donec ac odio tempor orci dapibus. Quam viverra orci sagittis eu volutpat odio. Enim neque volutpat ac tincidunt vitae semper. Nunc sed velit dignissim sodales. Magna eget est lorem ipsum dolor sit amet consectetur adipiscing. Ut etiam sit amet nisl purus. Tristique senectus et netus et malesuada fames ac turpis. Viverra mauris in aliquam sem fringilla ut morbi tincidunt. Suspendisse interdum consectetur libero id faucibus nisl tincidunt eget. Ipsum a arcu cursus vitae.        At quis risus sed vulputate odio ut enim blandit volutpat. A erat nam at lectus urna duis convallis convallis. Gravida quis blandit turpis cursus in hac habitasse. Commodo elit at imperdiet dui accumsan sit amet nulla. Elit pellentesque habitant morbi tristique senectus et netus et. Vulputate eu scelerisque felis imperdiet proin fermentum. Mauris in aliquam sem fringilla ut. Morbi tincidunt ornare massa eget egestas purus. At tempor commodo ullamcorper a lacus vestibulum sed arcu. Sit amet nulla facilisi morbi tempus iaculis. Morbi tristique senectus et netus et malesuada fames ac turpis. Sagittis aliquam malesuada bibendum arcu. Vivamus arcu felis bibendum ut tristique et egestas quis. Cursus sit amet dictum sit amet justo donec. Porttitor rhoncus dolor purus non enim praesent elementum facilisis. Sagittis aliquam malesuada bibendum arcu vitae elementum.       Mattis vulputate enim nulla aliquet porttitor lacus luctus accumsan. Dictum varius duis at consectetur lorem donec massa sapien faucibus. Nisi quis eleifend quam adipiscing vitae. Suspendisse faucibus interdum posuere lorem ipsum dolor sit amet consectetur. Tincidunt arcu non sodales neque sodales ut etiam. Id volutpat lacus laoreet non curabitur gravida arcu ac. Nulla facilisi cras fermentum odio eu feugiat. Fames ac turpis egestas sed tempus urna. Tristique nulla aliquet enim tortor at auctor urna. Bibendum neque egestas congue quisque egestas diam in arcu. Pharetra diam sit amet nisl. Etiam non quam lacus suspendisse faucibus. Diam sit amet nisl suscipit adipiscing bibendum est. At ultrices mi tempus imperdiet nulla malesuada pellentesque. Auctor neque vitae tempus quam pellentesque. Facilisis leo vel fringilla est ullamcorper eget nulla facilisi etiam. Morbi tempus iaculis urna id volutpat lacus laoreet non. Id semper risus in hendrerit gravida. Tincidunt dui ut ornare lectus sit. Ac odio tempor orci";

extern "C"{
   int main() {
      std::cerr << "****************************** START TEST BENCH ******************************" << endl;

      hls::stream<raw_stream_t, STREAM_DEPTH> link_ingress;
      hls::stream<raw_stream_t, STREAM_DEPTH> link_egress;
      hls::stream<ap_uint<512>, STREAM_DEPTH> sendmsg_s;
      hls::stream<ap_uint<416>, STREAM_DEPTH> recvmsg_s;
      hls::stream<ap_uint<80>, STREAM_DEPTH>  dma_r_req_s;
      hls::stream<ap_uint<536>, STREAM_DEPTH> dma_resp_s;
      hls::stream<ap_uint<544>, STREAM_DEPTH> dma_w_req_s;

      // ap_uint<512> sendmsg;

      ap_uint<512> sendmsg;
      ap_uint<416> recvmsg;

      ap_uint<128> saddr("DCBAFEDCBAFEDCBADCBAFEDCBAFEDCBA", 16);
      ap_uint<128> daddr("ABCDEFABCDEFABCDABCDEFABCDEFABCD", 16);

      uint16_t sport;
      uint16_t dport;

      dport = 0xBEEF;
      sport = 0xFEEB;

      // Offset in DMA space, receiver address, sender address, receiver port, sender port, RPC ID (0 for match-all)

      recvmsg(RECVMSG_BUFFOUT) = 0;
      recvmsg(RECVMSG_SADDR) = saddr;
      recvmsg(RECVMSG_DADDR) = daddr;
      recvmsg(RECVMSG_SPORT) = sport;
      recvmsg(RECVMSG_DPORT) = dport;
      recvmsg(RECVMSG_ID)  = 0;
      sendmsg(RECVMSG_RTT) = 1;

      sendmsg(SENDMSG_BUFFIN) = 0;
      sendmsg(SENDMSG_LENGTH) = 2772;
      sendmsg(SENDMSG_SADDR) = saddr;
      sendmsg(SENDMSG_DADDR) = daddr;
      sendmsg(SENDMSG_SPORT) = sport;
      sendmsg(SENDMSG_DPORT) = dport;
      sendmsg(SENDMSG_ID) = 0;
      sendmsg(SENDMSG_CC) = 0xFFFFFFFFFFFFFFFF;
      sendmsg(SENDMSG_RTT) = 1;

      char maxi_in[128*64];
      char maxi_out[128*64];

      // Construct a new RPC to ingest  
      homa(sendmsg_s, recvmsg_s, dma_r_req_s, dma_resp_s, dma_w_req_s, link_ingress, link_egress);

      sendmsg_s.write(sendmsg);
      recvmsg_s.write(recvmsg);

      // std::cerr << "CALLED HOMA\n";

      strcpy(maxi_in, data.c_str());

      while (maxi_out[2772] == 0) {
         if (!link_egress.empty()) {
            // std::cerr << "WRITING TO LINK INGRESS\n";
            link_ingress.write(link_egress.read());
            // std::cerr << "WROTE TO LINK INGRESS\n";
         }

         if (!dma_r_req_s.empty()) {
            ap_uint<80> dma_r_req = dma_r_req_s.read(); 

            for (int i = 0; i < dma_r_req(DMA_R_REQ_LENGTH); ++i) {
               integral_t chunk = *((integral_t*) (maxi_in + ((ap_uint<32>) dma_r_req(DMA_R_REQ_OFFSET)) + i * 64));
               ap_uint<536> dbuff_in;
               dbuff_in(DBUFF_IN_DATA) = chunk;
               dbuff_in(DBUFF_IN_ID) = dma_r_req(DMA_R_REQ_DBUFF_ID);
               dbuff_in(DBUFF_IN_CHUNK) = dma_r_req(DMA_R_REQ_OFFSET) + i;
               dma_resp_s.write(dbuff_in);
            }
         }

         if (!dma_w_req_s.empty()) {
            // std::cerr << "READ WRITE DMA REQ\n";
            ap_uint<544> dma_w_req = dma_w_req_s.read();
            // std::cerr << "OFFSET " << ((ap_uint<32>) dma_w_req(DMA_W_REQ_OFFSET)) << std::endl;
            (*((integral_t*) (maxi_out + ((ap_uint<32>) dma_w_req(DMA_W_REQ_OFFSET))))) = dma_w_req(DMA_W_REQ_BLOCK);
         }
      }

      return memcmp(maxi_in, maxi_out, 2772);
   }
}
