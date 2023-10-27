#ifndef HASHMAP_H
#define HASHMAP_H

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

//template<typename K, typename V>
//struct hashmap_t {
//
//    entry_t<K,V> table_0[HASHTABLE_SIZE];
//    entry_t<K,V> table_1[HASHTABLE_SIZE];
//    entry_t<K,V> table_2[HASHTABLE_SIZE];
//    entry_t<K,V> table_3[HASHTABLE_SIZE];
//
//    hashmap_t() {
//	for (int i = 0; i < HASHTABLE_SIZE; ++i) {
//	    table_0[i].value = 0;
//	    table_1[i].value = 0;
//	    table_2[i].value = 0;
//	    table_3[i].value = 0;
//	}
//    }
//
//    V search(K query) {
//
//	V result = 0;
//
//	entry_t<K,V> search[4];
//
//#pragma HLS array_partition type=complete variable=search
//
//	search[0] = table_0[simple_hash(query, SEED0)];
//	search[1] = table_1[simple_hash(query, SEED1)];
//	search[2] = table_2[simple_hash(query, SEED2)];
//	search[3] = table_3[simple_hash(query, SEED3)];
//
//	// if (ops.search(query, cam_id)) result = cam_id.entry.id;
//
//	for (int i = 0; i < 4; ++i) {
//#pragma HLS unroll
//	    if (search[i].key == query) result = search[i].value;
//	}
//
//	return result;
//    }
//
//    bool insert(entry_t<K,V> & entry) {
//#pragma HLS dependence variable=table_0 inter RAW false
//#pragma HLS dependence variable=table_1 inter RAW false
//#pragma HLS dependence variable=table_2 inter RAW false
//#pragma HLS dependence variable=table_3 inter RAW false
//#pragma HLS dependence variable=table_0 inter WAR false
//#pragma HLS dependence variable=table_1 inter WAR false
//#pragma HLS dependence variable=table_2 inter WAR false
//#pragma HLS dependence variable=table_3 inter WAR false
//
//	entry_t<K,V> eviction;
//
//	switch(entry.op) {
//	    case 0: {
//		ap_uint<HASHTABLE_INDEX> hash = simple_hash(entry.key, SEED0);
//		eviction = table_0[hash];
//		table_0[hash] = entry;
//		break;
//	    }
//	    case 1: {
//		ap_uint<HASHTABLE_INDEX> hash = simple_hash(entry.key, SEED1);
//		eviction = table_1[hash];
//		table_1[hash] = entry;
//		break;
//	    }
//	    case 2: {
//		ap_uint<HASHTABLE_INDEX> hash = simple_hash(entry.key, SEED2);
//		eviction = table_2[hash];
//		table_2[hash] = entry;
//		break;
//	    }
//	    case 3: {
//		ap_uint<HASHTABLE_INDEX> hash = simple_hash(entry.key, SEED3);
//		eviction = table_3[hash];                                            	
//		table_3[hash] = entry;
//		break;
//	    }
//		
//	    default: break;
//	}
//	
//	if (eviction.value != 0) {
//	    entry = eviction;
//	    entry.op++;
//	    return true;
//	} else {
//	    return false;
//	}
//    }
//
//    ap_uint<HASHTABLE_INDEX> simple_hash(K key, uint32_t seed) {
//	ap_uint<512> * hashbits = (ap_uint<512>*) &key;
//	ap_uint<HASHTABLE_INDEX> table_idx = (*hashbits);
//	// ap_uint<HASHTABLE_INDEX> table_idx = ((*hashbits * seed) >> (32 - HASHTABLE_INDEX)) % HASHTABLE_SIZE;
//	return table_idx;
//    }
//};


template<typename K, typename V>
struct hashmap_t {
    entry_t<K,V> tables[HASHTABLE_SIZE][16];

    V search(K query) {

#pragma HLS bind_storage variable=tables type=RAM_1WNR
#pragma HLS dependence variable=tables inter WAR false
#pragma HLS dependence variable=tables inter RAW false

	ap_uint<HASHTABLE_INDEX> hash = simple_hash(query, SEED0);

	V result = 0;

	for (ap_uint<HASHTABLE_INDEX> tidx = 0; tidx < 16; tidx++) {
#pragma HLS unroll
	    entry_t<K,V> search = tables[hash][tidx];
	    if (query == search.key) result = search.value;
	}

	return result;
    }

    void insert(entry_t<K,V> insert) {

#pragma HLS bind_storage variable=tables type=RAM_1WNR
#pragma HLS dependence variable=tables inter WAR false
#pragma HLS dependence variable=tables inter RAW false

	ap_uint<HASHTABLE_INDEX> hash = simple_hash(insert.key, SEED0);

	for (ap_uint<HASHTABLE_INDEX> tidx = 16-1; tidx > 1; tidx--) {
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

#endif
