#ifndef DATABUFF_H
#define DATABUFF_H

#include "homa.hh"
#include "net.hh"

void update_dbuff(hls::stream<dbuff_in_t> & dbuff_i,
		  hls::stream<out_block_t> & chunk_dispatch__dbuff,
		  hls::stream<raw_stream_t> & link_egress);
		  


#endif
