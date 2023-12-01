.PHONY: firmware 
homa-firmware:
	riscv32-unknown-elf-gcc -ffreestanding -nostartfiles -nostdlib -Qn -static -Wl,--strip-debug,-Bstatic,-T,picorv.ld homa.c reset.s -o firmware 
	riscv32-unknown-elf-objcopy -O binary firmware 
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 firmware firmware.o

clean: 
	rm -rf firmware
	rm -rf *.o
