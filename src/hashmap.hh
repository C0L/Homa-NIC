#ifndef HASHMAP_H
#define HASHMAP_H

#include "cam.hh"

#define SEED0 0x7BF6BF21
#define SEED1 0x9FA91FE9
#define SEED2 0xD0C8FBDF
#define SEED3 0xE6D0851C

template<typename H, typename I>
struct entry_t {
   H hashpack;
   I id;
};

template<typename E>
struct table_op_t {
   ap_uint<2> table_id;
   E entry;
};

template<typename H, typename I, int TABLE_SIZE, int TABLE_IDX, int PACK_SIZE, int CAM_SIZE>
struct hashmap_t {
   typedef entry_t<H,I> E;

   E table_0[TABLE_SIZE];
   E table_1[TABLE_SIZE];
   E table_2[TABLE_SIZE];
   E table_3[TABLE_SIZE];

   cam_t<H,table_op_t<E>,CAM_SIZE> ops;

   I search(H query) {
// #pragma HLS pipeline II=1

      I result = 0;

      table_op_t<E> cam_id;

      entry_t<H,I> search_0 = table_0[(ap_uint<TABLE_IDX>) simple_hash(query, SEED0)];
      entry_t<H,I> search_1 = table_1[(ap_uint<TABLE_IDX>) simple_hash(query, SEED1)];
      entry_t<H,I> search_2 = table_2[(ap_uint<TABLE_IDX>) simple_hash(query, SEED2)];
      entry_t<H,I> search_3 = table_3[(ap_uint<TABLE_IDX>) simple_hash(query, SEED3)];

      if (search_0.hashpack == query) result = search_0.id;
      if (search_1.hashpack == query) result = search_1.id;
      if (search_2.hashpack == query) result = search_2.id;
      if (search_3.hashpack == query) result = search_3.id;
      if (ops.search(query, cam_id)) result = cam_id.entry.id;

      return result;
   }

   void queue(E insert) {
// #pragma HLS pipeline II=1
      ops.push((table_op_t<E>) {0, insert});
   }

   void process() {

// #pragma HLS pipeline II=1
      if (!ops.empty()) {

         E out_entry;

         table_op_t<E> op = ops.pop();

         switch(op.table_id) {
            case 0: {

                       ap_uint<TABLE_IDX> hash = (ap_uint<TABLE_IDX>) simple_hash(op.entry.hashpack, SEED0);
                       out_entry = table_0[hash];
                       table_0[hash] = op.entry;
                       break;
                    }
            case 1: {

                       ap_uint<TABLE_IDX> hash = (ap_uint<TABLE_IDX>) simple_hash(op.entry.hashpack, SEED1);

                       out_entry = table_1[hash];
                       table_3[hash] = op.entry;
                       break;
                    }
            case 2: {

                       ap_uint<TABLE_IDX> hash = (ap_uint<TABLE_IDX>) simple_hash(op.entry.hashpack, SEED2);

                       out_entry = table_2[hash];
                       table_2[hash] = op.entry;
                       break;
                    }
            case 3: {

                       ap_uint<TABLE_IDX> hash = (ap_uint<TABLE_IDX>) simple_hash(op.entry.hashpack, SEED3);

                       out_entry = table_3[hash];                                            	
                       table_3[hash] = op.entry;
                       break;
                    }

            default: break;
         }

         if (!out_entry.id)
            ops.push((table_op_t<E>) {op.table_id++, op.entry});
      }
   }

   ap_uint<TABLE_IDX> simple_hash(H hashpack, uint32_t seed) {
      ap_uint<PACK_SIZE*32> * hashbits = (ap_uint<PACK_SIZE*32>*) &hashpack;
      ap_uint<TABLE_IDX> table_idx = ((*hashbits * seed) % TABLE_SIZE) >> (32 - TABLE_IDX);
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
