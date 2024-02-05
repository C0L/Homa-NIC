#ifndef PMAP_H
#define PMAP_H

#include "homa.hh"

void packetmap(hls::stream<header_t> & header_in_i, 
	       hls::stream<header_t> & header_in_o);

#endif
