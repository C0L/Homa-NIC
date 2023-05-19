#ifndef STACK_H
#define STACK_H

template<typename T, int MAX_SIZE>
struct stack_t {
  T buffer[MAX_SIZE];
  int size;

  stack_t(bool init) {
    if (init) {
      // Each RPC id begins as available 
      for (int id = 0; id < MAX_SIZE; ++id) {
	buffer[id] = id;
      }
      size = MAX_SIZE-1;
    } else {
      size = 0;
    }
  }

  void push(T value) {
    /* This must be pipelined for a caller function to be pipelined */
    #pragma HLS pipeline II=1
    size++;
    buffer[size] = value;
  }

  T pop() {
    /* This must be pipelined for a caller function to be pipelined */
    #pragma HLS pipeline II=1
    T head = buffer[size];
    size--;
    return head;
  }

  bool empty() {
    return (size == 0);
  }
};

#endif
