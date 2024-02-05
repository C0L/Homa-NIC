#ifndef FIFO_H
#define FIFO_H

template<typename T, int FIFO_SIZE>
struct fifo_t {
   T buffer[FIFO_SIZE];

   int read_head;

   fifo_t() {
      read_head = -1;
   }

   void insert(T value) {
// #pragma HLS array_partition variable=buffer type=complete

      for (int i = FIFO_SIZE-2; i >= 0; --i) {
#pragma HLS unroll
         buffer[i+1] = buffer[i];
      }

      buffer[0] = value;

      read_head++;
   }

   T remove() {
// #pragma HLS array_partition variable=buffer type=complete
      T val = buffer[read_head];

      read_head--;
      return val;
   }

   void dump() {
      for (int i = 0; i < 64; ++i) {
         std::cerr << buffer[i].offset << std::endl;
      }
   }

   T & head() {
      return buffer[read_head];
   }

   bool full() {
      return read_head == FIFO_SIZE-1;
   }

   int size() {
      return read_head;
   }

   bool empty() {
      return read_head == -1;
   }
};

#endif
