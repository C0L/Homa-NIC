obj-m := homanicmodule.o
homanicmodule-y += homanic.o mb.o

PWD := $(CURDIR)

CFLAGS_homanic.o := -mavx2

all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

.PHONY: mb
mb:
	aarch64-none-elf-gcc -ffreestanding -nostartfiles -nostdlib -Qn -static -Wl,--strip-debug,-Bstatic,-T,mb.ld mbtest.c reset.s -o mb
# aarch64-none-elf-objcopy -O binary mb
# objcopy -I binary -O elf64-x86-64 -B i386:x86-64 mb mb.o



clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf app
