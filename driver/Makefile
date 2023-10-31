obj-m += homanic.o

PWD := $(CURDIR)

CFLAGS_homanic.o := -mavx512f

all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

app:
	gcc -mavx512f application.c -o app

clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf app
