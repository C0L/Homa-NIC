.section ".boot"
.globl _start
_start:
	add     sp, x0, 0x8000 
	b       main
