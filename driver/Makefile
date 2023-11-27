obj-m := homanicmodule.o
homanicmodule-y += homanic.o softprog.o

PWD := $(CURDIR)

CFLAGS_homanic.o := -mavx2

all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

.PHONY: softprog
softprog:
	# # aarch64-none-elf-gcc -mbig-endian -ffreestanding -nostartfiles -nostdlib -Qn -static -Wl,--strip-debug,-Bstatic,-T,mb.ld mbtest.c reset.s -o mb
	# aarch64-none-elf-gcc -ffreestanding -nostartfiles -nostdlib -Qn -static -Wl,--strip-debug,-Bstatic,-T,mb.ld mbtest.c reset.s -o mb
	# # aarch64-none-elf-gcc -mbig-endian -ffreestanding -nostartfiles -nostdlib -Qn -static -Wl,--strip-debug,-Bstatic,-T,mb.ld mbtest.c reset.s -o mb
	# # arm-none-eabi-gcc -ffreestanding -nostartfiles -nostdlib -Qn -static -Wl,--strip-debug,-Bstatic,-T,mb.ld mbtest.c reset.s -o mb
	# # arm-none-eabi-objcopy -O binary mb
	# aarch64-none-elf-objcopy -O binary mb
	# objcopy -I binary -O elf64-x86-64 -B i386:x86-64 mb mb.o

	# arm-none-eabi-gcc -fomit-frame-pointer -ffreestanding -nostartfiles -nostdlib -Qn -static -Wl,--strip-debug,-Bstatic,-T,mb.ld mbtest.c reset.s -o mb
	# arm-none-eabi-objcopy -O binary mb
	# objcopy -I binary -O elf64-x86-64 -B i386:x86-64 mb mb.o
	#
	riscv32-unknown-elf-gcc -ffreestanding -nostartfiles -nostdlib -Qn -static -Wl,--strip-debug,-Bstatic,-T,mb.ld mbtest.c reset.s -o softprog
	riscv32-unknown-elf-objcopy -O binary softprog
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 softprog softprog.o




clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf app
