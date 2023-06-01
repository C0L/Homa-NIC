#ifndef DATABUFF_H
#define DATABUFF_H

#include "homa.hh"
#include "net.hh"

void update_dbuff(hls::stream<dbuff_in_t> & dbuff_i,
		  stream_t<out_pkt_t, raw_stream_t> & out_pkt_s);


#endif
