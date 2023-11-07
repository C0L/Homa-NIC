obj-m += homanic.o

PWD := $(CURDIR)

CFLAGS_homanic.o := -mavx2

all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

app:
	g++ -mavx2 -std=c++17 application.cc time_trace.cc test_utils.cc -o app

clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf app
