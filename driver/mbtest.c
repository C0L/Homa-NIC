// __asm__ __volatile__ (".byte 0x66, 0xf, 0x38, 0x15, 0xca" : "+x"(x) : "x"(m), "x"(y));

// https://stackoverflow.com/questions/77161122/how-to-build-a-bare-metal-app-for-aarch64-using-llvm

// volatile int test = 99;
volatile int test = 99;
__attribute__((naked, noreturn)) void main(){
    // volatile int * testp = &test;
    // test2 = 0xdeadbeef ;
    test = 0xdeadbeef;
    // volatile int test = 0xFFFFFFFF;
    // volatile int test = 0xdeadbeef;
    // while(1);
    // for (int i = 0; i < 64; ++i) *(testp + i) = 0xdeadbeef + i;
    // unsigned int * a = (unsigned int *)argv[1];
    // return a[2];
}
