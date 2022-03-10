#include "cpu.h"

#define BYTE_PATTERN "%c%c%c%c%c%c%c%c"
#define TO_BITS(x)\
        (x & 0x80 ? '1' : '0'),\
        (x & 0x40 ? '1' : '0'),\
        (x & 0x20 ? '1' : '0'),\
        (x & 0x10 ? '1' : '0'),\
        (x & 0x08 ? '1' : '0'),\
        (x & 0x04 ? '1' : '0'),\
        (x & 0x02 ? '1' : '0'),\
        (x & 0x01 ? '1' : '0')\

static inline void byte_dump(const uint8_t *memory, long int size){
    for (size_t i = 0; i < size; i++){
        printf("%02x  ", memory[i]);
        if (i % 16 == 0) printf("\n");
    }
}

int main(int argc, char **argv){

    //FILE* file = fopen("resources/invaders", "r");
    
    FILE* file = fopen("resources/cpudiag.bin", "r");
    
    if (file == NULL){
        printf("Error while opening the file\n");
        exit(1);
    }

    fseek(file, 0L, SEEK_END);
    long int size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    cpu8080 state = reset8080();
    uint8_t memory[0xffff] = {0};

    uint8_t *buffer = &memory[0x100];
    fread(buffer, size, 1, file);
    
    //fread(memory, size, 1, file);
    fclose(file);

    //first ins is JMP to 0x100;
    memory[0] = 0xc3; // JMP
    memory[1] = 0x00; // lo byte
    memory[2] = 0x01; // hi byte

    memory[368] = 0x7;

    memory[0x59c] = 0xc3; //JMP    
    memory[0x59d] = 0xc2;    
    memory[0x59e] = 0x05;

    //byte_dump(memory, size);

    uint64_t counter = 0;
    printf("\nfile size: %ld\n--------------------\n", size);
    while(counter < 500){

        emulate8080(&state, memory);
        if (1) {
            disassemble8080(memory, state.pc, counter);
            printf("\t\tB: %02x  C: %02x  D: %02x  E: %02x  H: %02x  L: %02x  A: %02x", 
                    state.b, state.c, state.d, state.e, state.h, state.l, state.a);
            printf(" \tflags:  z%d  s%d  p%d  ac%d  cy%d", 
                    state.flags.z, state.flags.s, state.flags.p, state.flags.ac, state.flags.cy);
            printf(" \tpc: %04x   sp: %04x\n", state.pc, state.sp);
        }
        
        counter++;
    }
    return 0;
}