#ifndef HASHMAP_H
#define HASHMAP_H

#define SEED0 0x7BF6BF21
#define SEED1 0x9FA91FE9
#define SEED2 0xD0C8FBDF
#define SEED3 0xE6D0851C
template<typename V>
struct fifo_t {
    V buffer[8];

    int read_head;

    fifo_t() {
	read_head = -1;

	for (int i = 0; i < 8; ++i) {
#pragma HLS unroll
	    buffer[i].entry.id = 0;
	}
    }

    void insert(V value) {
// #pragma HLS array_partition variable=buffer type=complete
	buffer[0] = value;
	for (int i = 8-2; i >= 0; --i) {
#pragma HLS unroll
	    buffer[i+1] = buffer[i];
	}

	read_head++;
    }

    V remove() {
// #pragma HLS array_partition variable=buffer type=complete
	
	V val = buffer[read_head--];
	return val;
    }
};

template<typename K, typename V, int MAX_SIZE>
struct cam_t {
    V buffer[MAX_SIZE];

    ap_uint<6> write_head;

    cam_t() {
	for (int i = 0; i < 8; ++i) {
	    buffer[i].entry.id = 0;
	}
    }

    bool search(K key, V & value) {
#pragma HLS pipeline II=1
#pragma HLS array_partition variable=buffer type=complete

	for (int i = 0; i < MAX_SIZE; ++i) {
#pragma HLS unroll
	    if (buffer[i].entry.hashpack == key) {
		value = buffer[i];
		return true;
		// TODO may eventually need to get all matches and return top one when there is a deletion process
	    }
	}

	return false;
    }

    void push(V op) {
#pragma HLS pipeline II=1
#pragma HLS array_partition variable=buffer type=complete

	buffer[write_head++] = op;
    }

//    V pop() {
//#pragma HLS pipeline II=1
//#pragma HLS array_partition variable=buffer type=complete
//
//	for (int i = 8-2; i >= 0; --i) {
//#pragma HLS unroll
//	    buffer[i+1] = buffer[i];
//	}
//
//	return buffer[4];
//    }
};

template<typename E, int TABLE_IDX>
struct table_op_t {
    ap_uint<2> table_id;
    E entry;
    ap_uint<TABLE_IDX> hash;
};

template<typename H, typename I>
struct entry_t {
    H hashpack;
    I id;
};

template<typename H, typename I, int TABLE_SIZE, int TABLE_IDX, int PACK_SIZE, int CAM_SIZE>
struct hashmap_t {
    typedef entry_t<H,I> E;

    E table_0[TABLE_SIZE];
    E table_1[TABLE_SIZE];
    E table_2[TABLE_SIZE];
    E table_3[TABLE_SIZE];

    cam_t<H,table_op_t<E, TABLE_IDX>,CAM_SIZE> cam;
    fifo_t<table_op_t<E>> ops;

    hashmap_t() {
	for (int i = 0; i < TABLE_SIZE; ++i) {
	    table_0[i].id = 0;
	    table_1[i].id = 0;
	    table_2[i].id = 0;
	    table_3[i].id = 0;
	}
    }

    I search(H query) {
#pragma HLS bind_storage variable=table_0 type=RAM_1WNR
#pragma HLS bind_storage variable=table_1 type=RAM_1WNR
#pragma HLS bind_storage variable=table_2 type=RAM_1WNR
#pragma HLS bind_storage variable=table_3 type=RAM_1WNR

#pragma HLS dependence variable=table_0 inter WAR false
#pragma HLS dependence variable=table_0 inter RAW false
#pragma HLS dependence variable=table_1 inter WAR false
#pragma HLS dependence variable=table_1 inter RAW false
#pragma HLS dependence variable=table_2 inter WAR false
#pragma HLS dependence variable=table_2 inter RAW false
#pragma HLS dependence variable=table_3 inter WAR false
#pragma HLS dependence variable=table_3 inter RAW false

#pragma HLS pipeline II=1

	I result = 0;

	table_op_t<E> cam_id;

	entry_t<H,I> search[4];
#pragma HLS array_partition variable=search type=complete
    
	search[0] = table_0[(ap_uint<TABLE_IDX>) simple_hash(query, SEED0)];
	search[1] = table_1[(ap_uint<TABLE_IDX>) simple_hash(query, SEED1)];
	search[2] = table_2[(ap_uint<TABLE_IDX>) simple_hash(query, SEED2)];
	search[3] = table_3[(ap_uint<TABLE_IDX>) simple_hash(query, SEED3)];

	// if (ops.search(query, cam_id)) return cam_id.entry.id;

	for (int i = 0; i < 4; ++i) {
#pragma HLS unroll
	    if (search[i].hashpack == query) return search[i].id;
	}

	return 0;
    }

    void insert(E insert) {
#pragma HLS bind_storage variable=table_0 type=RAM_1WNR
#pragma HLS bind_storage variable=table_1 type=RAM_1WNR
#pragma HLS bind_storage variable=table_2 type=RAM_1WNR
#pragma HLS bind_storage variable=table_3 type=RAM_1WNR
//
//#pragma HLS dependence variable=ops inter WAR false
//#pragma HLS dependence variable=ops inter RAW false
//
//#pragma HLS dependence variable=cam inter WAR false
//#pragma HLS dependence variable=cam inter RAW false
//
#pragma HLS dependence variable=table_0 inter WAR false
#pragma HLS dependence variable=table_0 inter RAW false
#pragma HLS dependence variable=table_1 inter WAR false
#pragma HLS dependence variable=table_1 inter RAW false
#pragma HLS dependence variable=table_2 inter WAR false
#pragma HLS dependence variable=table_2 inter RAW false
#pragma HLS dependence variable=table_3 inter WAR false
#pragma HLS dependence variable=table_3 inter RAW false

#pragma HLS pipeline II=1
	ops.insert((table_op_t<E>) {0, insert});
    }

    void process() {
#pragma HLS bind_storage variable=table_0 type=RAM_1WNR
#pragma HLS bind_storage variable=table_1 type=RAM_1WNR
#pragma HLS bind_storage variable=table_2 type=RAM_1WNR
#pragma HLS bind_storage variable=table_3 type=RAM_1WNR
//
//#pragma HLS dependence variable=ops inter WAR false
//#pragma HLS dependence variable=ops inter RAW false
//
//#pragma HLS dependence variable=cam inter WAR false
//#pragma HLS dependence variable=cam inter RAW false
//
#pragma HLS dependence variable=table_0 inter WAR false
#pragma HLS dependence variable=table_0 inter RAW false
#pragma HLS dependence variable=table_1 inter WAR false
#pragma HLS dependence variable=table_1 inter RAW false
#pragma HLS dependence variable=table_2 inter WAR false
#pragma HLS dependence variable=table_2 inter RAW false
#pragma HLS dependence variable=table_3 inter WAR false
#pragma HLS dependence variable=table_3 inter RAW false

#pragma HLS pipeline II=1

	E out_entry;
	out_entry.id = 0;
    
	table_op_t<E> op = ops.remove();

	if (op.entry.id != 0) {

	    switch(op.table_id) {
		case 0: {
		    // ap_uint<TABLE_IDX> hash = (ap_uint<TABLE_IDX>) simple_hash(op.entry.hashpack, SEED0);
		    // out_entry = table_0[hash];
		    table_0[hash] = op.entry;
		    break;
		}
		case 1: {
		    // ap_uint<TABLE_IDX> hash = (ap_uint<TABLE_IDX>) simple_hash(op.entry.hashpack, SEED1);
		    // out_entry = table_1[hash];
		    table_1[hash] = op.entry;
		    break;
		}
		case 2: {
		    // ap_uint<TABLE_IDX> hash = (ap_uint<TABLE_IDX>) simple_hash(op.entry.hashpack, SEED2);
		    // out_entry = table_2[hash];
		    table_2[hash] = op.entry;
		    break;
		}
		case 3: {
		    // ap_uint<TABLE_IDX> hash = (ap_uint<TABLE_IDX>) simple_hash(op.entry.hashpack, SEED3);
		    // out_entry = table_3[hash];                                            	
		    table_3[hash] = op.entry;
		    break;
		}

		default: break;
	    }
	
	    //if (out_entry.id != 0) {
	    //	// TODO insert new entry in CAM
	    //	cam.push((table_op_t<E>) {(op.table_id + 1) % 4, out_entry});
	    //	ops.insert((table_op_t<E>) {(op.table_id + 1) % 4, out_entry});
	    //}
	}
    }

    ap_uint<TABLE_IDX> simple_hash(H hashpack, uint32_t seed) {
#pragma HLS pipeline II=1
	ap_uint<512> * hashbits = (ap_uint<512>*) &hashpack;
	ap_uint<TABLE_IDX> table_idx = *hashbits;
	// ap_uint<TABLE_IDX> table_idx = ((*hashbits * seed) >> (32 - TABLE_IDX)) % TABLE_SIZE;
	return table_idx;
    }

    uint32_t murmur_32_scramble(uint32_t k) {
#pragma HLS pipeline II=1
	k *= 0xcc9e2d51;
	k = (k << 15) | (k >> 17);
	k *= 0x1b873593;
	return k;
    }

    uint32_t murmur3_32(H hashpack, int len, uint32_t seed) {
#pragma HLS pipeline II=1
	uint32_t * key = (uint32_t*) &hashpack;
	uint32_t h = seed;
	uint32_t k;

	for (int i = len; i; i--) {
#pragma HLS unroll
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
