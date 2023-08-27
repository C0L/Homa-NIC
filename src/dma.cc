#include "dma.hh"

#include <iostream>
#include <fstream>

using namespace std;

/**
 * dma_read() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_read(uint32_t action,
	      ap_uint<512> * maxi,
	      hls::stream<srpt_sendq_t> & dma_req_i,
	      hls::stream<dbuff_in_t> & dbuff_in_o) {
#pragma HLS pipeline II=1

    srpt_sendq_t dma_req;

#ifdef CSIM
    if (dma_req_i.read_nb(dma_req)) {
	ofstream trace_file;
	trace_file.open("../../../../traces/unscheduled_trace", ios::app);
	trace_file << 4 << std::endl;
	trace_file.close();
#endif

#ifdef COSIM
    if (action == 4) {
	dma_req_i.read(dma_req);
#endif

	// TODO revise this indexing? Why need byte indexing when already aligned??

	ap_uint<8> * byte_offset = (ap_uint<8> *) maxi;

	byte_offset += dma_req(SENDQ_OFFSET);

	dbuff_in_t dbuff_in;
	dbuff_in.data = *((ap_uint<512> *) byte_offset);

	dbuff_in.dbuff_id = dma_req(SENDQ_DBUFF_ID);
	dbuff_in.local_id = dma_req(SENDQ_RPC_ID);
	dbuff_in.offset = dma_req(SENDQ_OFFSET);
	dbuff_in.msg_len = dma_req(SENDQ_MSG_LEN);
	// dbuff_in.last = dma_req.last; // TODO unused
	dbuff_in_o.write(dbuff_in);
	std::cerr << "DMA READ\n";
    }
}

/**
 * dma_write() - Queue requests on the MAXI bus for RPC message data
 * @maxi - The MAXI interface connected to DMA space
 * @dma_requests__dbuff - 64B data chunks for storage in data space
 */
void dma_write(uint32_t action, ap_uint<512> * maxi,
	       hls::stream<dma_w_req_t> & dma_w_req_i) {

#pragma HLS pipeline II=1

   dma_w_req_t dma_req;

#ifdef CSIM
   if (dma_w_req_i.read_nb(dma_req)) {
	ofstream trace_file;
	trace_file.open("../../../../traces/unscheduled_trace", ios::app);
	trace_file << 5 << std::endl;
	trace_file.close();
#endif

#ifdef COSIM
    if (action == 5) {
       dma_w_req_i.read(dma_req);
#endif


       // offset += dma_req.offset;



       // TODO the data offset in is already unaligned??

       /***** STROBE STARTS AT 14 BECAUSE THAT IS THE FIRST CHUNK SIZE AND IT GOES AROUND CYCLICALLY AFTER THAT *****/

       // TODO there is a relationship between byte offset and strobeing.
       // Need to combine byte offset with strobing (ensure that the byte offset still only generates one request!!!!

// #define AXI_WRITE(strobe) case strobe: { *(((ap_uint<strobe*8> *) (maxi + (dma_req.offset / DBUFF_CHUNK_SIZE)))) = dma_req.data; break; }

// #define AXI_WRITE(strobe) case strobe: { *(((ap_uint<strobe*8> *) offset)) = dma_req.data; break; }

       char * test = (char *) &(dma_req.data);

       std::cerr << "DMA OFFSET: " << dma_req.offset << std::endl;
       // std::cerr << "DATA: " << test << std::endl;
       std::cerr << "DATA: ";
       for (int i = 0; i < 64; ++i) {
	   std::cerr << i << " " << test[i]; 
       }

       std::cerr << std::endl;

       ap_uint<8> * offset = (ap_uint<8> *) maxi;
       *((ap_uint<512> *) (offset + dma_req.offset)) = dma_req.data;

       // std::cerr << "STROBE " << dma_req.strobe << std::endl;

       // TODO This generates a lot of extra HW

       // TODO Try some OR operation to do the writes more efficiently?
       // TODO Maybe use the or operator of the ap_uints?

       // TODO Maybe the stobe is modified when the offset is non divisible???
       // YES! This appears to shift the offset (meaning multiple writes would be needed)
       // But multiple writes are not performed! So we lose like 14 bytes of data each time.

       // TODO this is only needed for OOO packets?

       // switch (dma_req.strobe) {
       // 	   AXI_WRITE(1) AXI_WRITE(2) AXI_WRITE(3)
       // 	   AXI_WRITE(4) AXI_WRITE(5) AXI_WRITE(6) AXI_WRITE(7) 
       // 	   AXI_WRITE(8) AXI_WRITE(9) AXI_WRITE(10) AXI_WRITE(11)
       // 	   AXI_WRITE(12) AXI_WRITE(13) AXI_WRITE(14) AXI_WRITE(15) 
       // 	   AXI_WRITE(16) AXI_WRITE(17) AXI_WRITE(18) AXI_WRITE(19)
       // 	   AXI_WRITE(20) AXI_WRITE(21) AXI_WRITE(22) AXI_WRITE(23) 
       // 	   AXI_WRITE(24) AXI_WRITE(25) AXI_WRITE(26) AXI_WRITE(27)
       // 	   AXI_WRITE(28) AXI_WRITE(29) AXI_WRITE(30) AXI_WRITE(31) 
       // 	   AXI_WRITE(32) AXI_WRITE(33) AXI_WRITE(34) AXI_WRITE(35)
       // 	   AXI_WRITE(36) AXI_WRITE(37) AXI_WRITE(38) AXI_WRITE(39) 
       // 	   AXI_WRITE(40) AXI_WRITE(41) AXI_WRITE(42) AXI_WRITE(43)
       // 	   AXI_WRITE(44) AXI_WRITE(45) AXI_WRITE(46) AXI_WRITE(47) 
       // 	   AXI_WRITE(48) AXI_WRITE(49) AXI_WRITE(50) AXI_WRITE(51)
       // 	   AXI_WRITE(52) AXI_WRITE(53) AXI_WRITE(54) AXI_WRITE(55) 
       // 	   AXI_WRITE(56) AXI_WRITE(57) AXI_WRITE(58) AXI_WRITE(59)
       // 	   AXI_WRITE(60) AXI_WRITE(61) AXI_WRITE(62) AXI_WRITE(63) 
       // }
   }
}
