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
            buffer[id] = MAX_SIZE-1 - id;
         }
         size = MAX_SIZE-1;
      } else {
         size = 0;
      }
   }

   void push(T value) {
      size++;
      buffer[size] = value;
   }

   T pop() {
      T head = buffer[size];
      size--;
      return head;
   }

   bool empty() {
      return (size == 0);
   }
};

#endif
