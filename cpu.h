#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint8_t s:1;   // set if negative
    uint8_t z:1;   // set if zero
    uint8_t p:1;   // set if num bits is even
    uint8_t cy:1;  // set if carry bit
    uint8_t ac:1;  // set if carry bit in 4 bit sequence
    uint8_t pad:3; // extra padding to make 1 byte
} cpu_flags;

typedef struct {
    cpu_flags flags;
    uint8_t int_enable;
    uint16_t pc;
    uint16_t sp;

    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
} cpu8080;

cpu8080 reset8080();
int disassemble8080(uint8_t *memory, int pc);
void emulate8080(cpu8080* state, uint8_t *memory);

enum OP{ 
    NOP     = 0x00, 
    
    LXI_B   = 0x01, STAX_B  = 0x02, INX_B   = 0x03, INR_B   = 0x04, DCR_B   = 0x05, MVI_B   = 0x06, RLC     = 0x07, DAD_B   = 0x09, LDAX_B  = 0x0a, DCX_B   = 0x0b,
    LXI_D   = 0x11, STAX_D  = 0x12, INX_D   = 0x13, INR_D   = 0x14, DCR_D   = 0x15, MVI_D   = 0x16, RAL     = 0x17, DAD_D   = 0x19, LDAX_D  = 0x1a, DCX_D   = 0x1b,
    LXI_H   = 0x21, SHLD    = 0x22, INX_H   = 0x23, INR_H   = 0x24, DCR_H   = 0x25, MVI_H   = 0x26, DAA     = 0x27, DAD_H   = 0x29, LHLD    = 0x2a, DCX_H   = 0x2b,
    LXI_SP  = 0x31, STA     = 0x32, INX_SP  = 0x33, INR_M   = 0x34, DCR_M   = 0x35, MVI_M   = 0x36, STC     = 0x37, DAD_SP  = 0x39, LDA     = 0x3a, DCX_SP  = 0x3b,
    
    INR_C   = 0x0c, DCR_C   = 0x0d, MVI_C   = 0x0e, RRC     = 0x0f,
    INR_E   = 0x1c, DCR_E   = 0x1d, MVI_E   = 0x1e, RAR     = 0x1f,
    INR_L   = 0x2c, DCR_L   = 0x2d, MVI_L   = 0x2e, CMA     = 0x2f,
    INR_A   = 0x3c, DCR_A   = 0x3d, MVI_A   = 0x3e, CMC     = 0x3f,

    MOV_B_B = 0x40, MOV_B_C = 0x41, MOV_B_D = 0x42, MOV_B_E = 0x43, MOV_B_H = 0x44, MOV_B_L = 0x45, MOV_B_M = 0x46, MOV_B_A = 0x47,
    MOV_D_B = 0x50, MOV_D_C = 0x51, MOV_D_D = 0x52, MOV_D_E = 0x53, MOV_D_H = 0x54, MOV_D_L = 0x55, MOV_D_M = 0x56, MOV_D_A = 0x57,
    MOV_H_B = 0x60, MOV_H_C = 0x61, MOV_H_D = 0x62, MOV_H_E = 0x63, MOV_H_H = 0x64, MOV_H_L = 0x65, MOV_H_M = 0x66, MOV_H_A = 0x67,
    MOV_M_B = 0x70, MOV_M_C = 0x71, MOV_M_D = 0x72, MOV_M_E = 0x73, MOV_M_H = 0x74, MOV_M_L = 0x75, HLT     = 0x76, MOV_M_A = 0x77,
    
    MOV_C_B = 0x48, MOV_C_C = 0x49, MOV_C_D = 0x4a, MOV_C_E = 0x4b, MOV_C_H = 0x4c, MOV_C_L = 0x4d, MOV_C_M = 0x4e, MOV_C_A = 0x4f,
    MOV_E_B = 0x58, MOV_E_C = 0x59, MOV_E_D = 0x5a, MOV_E_E = 0x5b, MOV_E_H = 0x5c, MOV_E_L = 0x5d, MOV_E_M = 0x5e, MOV_E_A = 0x5f,
    MOV_L_B = 0x68, MOV_L_C = 0x69, MOV_L_D = 0x6a, MOV_L_E = 0x6b, MOV_L_H = 0x6c, MOV_L_L = 0x6d, MOV_L_M = 0x6e, MOV_L_A = 0x6f,
    MOV_A_B = 0x78, MOV_A_C = 0x79, MOV_A_D = 0x7a, MOV_A_E = 0x7b, MOV_A_H = 0x7c, MOV_A_L = 0x7d, MOV_A_M = 0x7e, MOV_A_A = 0x7f,

};