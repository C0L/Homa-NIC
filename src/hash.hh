// https://github.com/torvalds/linux/blob/master/include/linux/hash.h

#define GOLDEN_RATIO_32 0x61C88647

  ap_uint<32> hash_32(ap_uint<32> val, unsigned int bits) {
  /* On some cpus multiply is faster, on others gcc will do shifts */
  ap_uint<32> hash = val * GOLDEN_RATIO_32;
  
  /* High bits are more random, so use them. */
  return hash >> (32 - bits);
}

