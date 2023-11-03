#ifndef MAP_H
#define MAP_H

#include "homa.hh"
#include "stack.hh"

#define SEED0 0x7BF6BF21
#define SEED1 0x9FA91FE9
#define SEED2 0xD0C8FBDF
#define SEED3 0xE6D0851C

template<typename K, typename V, int MAX_SIZE, int INDEX_BITS>
struct cam_t {
    entry_t<K,V> buffer[MAX_SIZE];

    ap_uint<INDEX_BITS> write_head;

    cam_t() {
	write_head = 0;
	for (int i = 0; i < MAX_SIZE; ++i) {
	    buffer[i].value = 0;
	}
    }

    V search(K key) {
#pragma HLS dependence variable=buffer inter false

#pragma HLS pipeline II=1
#pragma HLS array_partition variable=buffer type=complete

	for (int i = 0; i < MAX_SIZE; ++i) {
#pragma HLS unroll
	    if (buffer[i].key == key) {
		return buffer[i].value;
	    }
	}

	return 0;
    }

    void insert(entry_t<K,V> entry) {
#pragma HLS dependence variable=buffer inter false

#pragma HLS pipeline II=1
#pragma HLS array_partition variable=buffer type=complete
	buffer[write_head++] = entry;
    }
};

template<typename K, typename V>
struct hashmap_t {
    entry_t<K,V> tables[HASHTABLE_SIZE][3];

    V search(K query) {

// #pragma HLS bind_storage variable=tables type=RAM_1WNR
#pragma HLS dependence variable=tables inter WAR false
#pragma HLS dependence variable=tables inter RAW false

	ap_uint<HASHTABLE_INDEX> hash = simple_hash(query, SEED0);

	V result = 0;

	for (ap_uint<HASHTABLE_INDEX> tidx = 0; tidx < 3; tidx++) {
#pragma HLS unroll
	    entry_t<K,V> search = tables[hash][tidx];
	    if (query == search.key) result = search.value;
	}

	return result;
    }

    void insert(entry_t<K,V> insert) {

// #pragma HLS bind_storage variable=tables type=RAM_1WNR
#pragma HLS dependence variable=tables inter WAR false
#pragma HLS dependence variable=tables inter RAW false

	ap_uint<HASHTABLE_INDEX> hash = simple_hash(insert.key, SEED0);

	for (ap_uint<HASHTABLE_INDEX> tidx = 3-1; tidx > 1; tidx--) {
#pragma HLS unroll
	    tables[hash][tidx] = tables[hash][tidx-1];
	}

	tables[hash][0] = insert;
    }


    ap_uint<HASHTABLE_INDEX> simple_hash(K key, uint32_t seed) {
	ap_uint<512> * hashbits = (ap_uint<512>*) &key;
	ap_uint<HASHTABLE_INDEX> table_idx = ((*hashbits * seed) >> (32 - HASHTABLE_INDEX)) % HASHTABLE_SIZE;
	return table_idx;
    }

};

void c2h_header_hashmap(
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<entry_t<rpc_hashpack_t, local_id_t>> & new_rpcmap_entry,
    hls::stream<entry_t<peer_hashpack_t, peer_id_t>> & new_peermap_entry
    );

void c2h_header_cam(
    hls::stream<header_t> & c2h_header_i,
    hls::stream<header_t> & c2h_header_o,
    hls::stream<entry_t<rpc_hashpack_t, local_id_t>> & new_rpcmap_entry,
    hls::stream<entry_t<peer_hashpack_t, peer_id_t>> & new_peermap_entry
    );

#endif
