// __asm__ __volatile__ (".byte 0x66, 0xf, 0x38, 0x15, 0xca" : "+x"(x) : "x"(m), "x"(y));

// https://stackoverflow.com/questions/77161122/how-to-build-a-bare-metal-app-for-aarch64-using-llvm

volatile int test = 0;
volatile int test2;

void main(){
    test = 0xdeadbeef;
    return;
    // unsigned int * a = (unsigned int *)argv[1];
    // return a[2];
}
