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

int main(int argc, char **argv){

    FILE* file = fopen("resources/invaders", "r");
    
    if (file == NULL){
        printf("Error while opening the file\n");
        exit(1);
    }

    fseek(file, 0L, SEEK_END);
    long int size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    cpu8080 state = reset8080();
    uint8_t memory[0xffff] = {0};

    //memory = malloc(sizeof(char) * size);

    fread(memory, size, 1, file);
    fclose(file);

    uint64_t counter = 0;
    printf("\nfile size: %ld\n--------------------\n", size);
    while(1){
        disassemble8080(memory, state.pc, counter++);
        emulate8080(&state, memory);
    }
    printf("B: %02x  C: %02x  D: %02x  E: %02x  H: %02x  L: %02x  A: %02x", 
                state.b, state.c, state.d, state.e, state.h, state.l, state.a);
    printf(" \tflags:  z%d  s%d  p%d  ac%d  cy%d", 
                state.flags.z, state.flags.s, state.flags.p, state.flags.ac, state.flags.cy);
    printf(" \tpc: %04x   sp: %04x\n", state.pc, state.sp);

    // uint8_t testmem[10] = {LXI_B, 0x00, 0xff, ADD_B, ADD_B};

    // printf("\nfile size: %ld\n--------------------\n", size);
    // while(state.pc < 10){
    //     emulate8080(&state, testmem);
    //     printf("B: %02x  C: %02x  D: %02x  E: %02x  H: %02x  L: %02x  A: %02x", 
    //             state.b, state.c, state.d, state.e, state.h, state.l, state.a);
    //     printf(" \tflags:  z%d  s%d  p%d  ac%d  cy%d\n", 
    //             state.flags.z, state.flags.s, state.flags.p, state.flags.ac, state.flags.cy);
    // }
    // printf("\n");

    //free(memory);
    return 0;
}