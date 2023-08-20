#ifndef HASHMAP_H
#define HASHMAP_H

// #include "cam.hh"

#define SEED0 0x7BF6BF21
#define SEED1 0x9FA91FE9
#define SEED2 0xD0C8FBDF
#define SEED3 0xE6D0851C

template<typename H, typename I>
struct entry_t {
    H hashpack;
    I id;
};

template<typename H, typename I, int BUCKETS, int BUCKET_SIZE, int TABLE_IDX, int PACK_SIZE>
struct hashmap_t {
    typedef entry_t<H,I> E;

    E tables[BUCKETS][BUCKET_SIZE];

    I search(H query) {

#pragma HLS dependence variable=tables inter WAR false
#pragma HLS dependence variable=tables inter RAW false

	ap_uint<TABLE_IDX> hash = simple_hash(query, SEED0);

	I result = 0;

	for (ap_uint<TABLE_IDX> tidx = 0; tidx < BUCKET_SIZE; tidx++) {
#pragma HLS unroll
	    entry_t<H,I> search = tables[hash][tidx];
	    if (query == search.hashpack) result = search.id;
	}

	return result;
    }

    void insert (E insert) {

#pragma HLS dependence variable=tables inter WAR false
#pragma HLS dependence variable=tables inter RAW false

	ap_uint<TABLE_IDX> hash = simple_hash(insert.hashpack, SEED0);

	for (ap_uint<TABLE_IDX> tidx = BUCKET_SIZE-1; tidx > 1; tidx--) {
#pragma HLS unroll
	    tables[hash][tidx] = tables[hash][tidx-1];
	}

	tables[hash][0] = insert;
    }

    ap_uint<TABLE_IDX> simple_hash(H hashpack, uint32_t seed) {
	ap_uint<PACK_SIZE*32> * hashbits = (ap_uint<PACK_SIZE*32>*) &hashpack;
	ap_uint<TABLE_IDX> table_idx = ((*hashbits * seed) % BUCKETS) >> (32 - TABLE_IDX);
	return table_idx;
    }

    uint32_t murmur_32_scramble(uint32_t k) {
// #pragma HLS pipeline II=1
	k *= 0xcc9e2d51;
	k = (k << 15) | (k >> 17);
	k *= 0x1b873593;
	return k;
    }

    uint32_t murmur3_32(H hashpack, int len, uint32_t seed) {
// #pragma HLS pipeline II=1
	uint32_t * key = (uint32_t*) &hashpack;
	uint32_t h = seed;
	uint32_t k;

	for (int i = len; i; i--) {
// #pragma HLS unroll
	    k = *key;
	    key += 1;
	    h ^= murmur_32_scramble(k);
	    h = (h << 13) | (h >> 19);
	    h = h * 5 + 0xe6546b64;
	}

	h ^= murmur_32_scramble(k);

	/* Finalize. */
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
    }
};

#endif
