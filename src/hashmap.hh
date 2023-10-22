#ifndef HASHMAP_H
#define HASHMAP_H

template<typename H, typename I>
struct entry_t {
    H hashpack;
    I id;
};

template<typename K, typename V, int MAX_SIZE, int INDEX_BITS>
struct cam_t {
    struct entry_t {
	K key;
	V value;
    };

    entry_t buffer[MAX_SIZE];

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

    void insert(K key, V value) {
#pragma HLS dependence variable=buffer inter false

#pragma HLS pipeline II=1
#pragma HLS array_partition variable=buffer type=complete
	buffer[write_head++] = (entry_t) {key, value};
    }
};

#endif
