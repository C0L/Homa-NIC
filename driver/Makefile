obj-m := homanicmodule.o
homanicmodule-y += homanic.o firmware.o

PWD := $(CURDIR)

CFLAGS_homanic.o := -mavx2

all: 
	cp ../firmware/firmware.o .
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf app
