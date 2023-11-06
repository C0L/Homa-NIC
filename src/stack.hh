#ifndef STACK_H
#define STACK_H

template<typename T, int MAX_SIZE>
struct stack_t {
    T buffer[MAX_SIZE];
    ap_uint<14> head;

    stack_t() {
	for (int id = 0; id < MAX_SIZE; ++id) {
	    buffer[id] = MAX_SIZE - id;
	}
	head = MAX_SIZE;
    }

    void push(T value) {
	buffer[head++] = value;
    }

    T pop() {
	T value = buffer[--head];
	return value;
    }

    bool empty() {
	return (head == 0);
    }
};

#endif
