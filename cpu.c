#include "cpu.h"

#define P2(n) n, n^1, n^1, n
#define P4(n) P2(n), P2(n^1), P2(n^1), P2(n)
#define P6(n) P4(n), P4(n^1), P4(n^1), P4(n)
#define FIND_PARITY P6(0), P6(1), P6(1), P6(0)

const uint8_t parityTable[256] = {FIND_PARITY};

void set_zspac_flags(cpu_flags* flags, uint8_t val){
    flags->z = (val & 0xff) == 0;
    flags->s = (val & 0x80) == 0x80; // 0b10000000 = 0x80 = 128 <- als bit 7 aanstaat dan mask dit bit en vergelijk met 128
    flags->p = parityTable[val] == 0;
    flags->ac = val > 0x0f;
}

void func_on_A(char operation, cpu8080* state, uint8_t val){

    uint16_t res;
    if (operation == '+'){
        uint16_t res = state->a + val;
    } else if (operation == '-'){
        uint16_t res = state->a - val;
    } else if (operation == '&'){
        uint16_t res = state->a & val;
    } else if (operation == '|'){
        uint16_t res = state->a | val;
    } else if (operation == '^'){
        uint16_t res = state->a ^ val;
    } else if (operation == '#'){ // add with carry
        uint16_t res = state->a + val + state->flags.cy;
    } else if (operation == '@'){ // subtract with carry
        uint16_t res = state->a - val - state->flags.cy;
    }

    state->flags.cy = res > 0xff;
    state->a = res & 0xff;
    set_zspac_flags(&state->flags, state->a);
}

cpu8080 reset8080(){
    cpu8080 cpu;

    cpu.pc = 0;
    cpu.sp = 0;

    cpu.b = 0;
    cpu.c = 0;
    cpu.d = 0;
    cpu.e = 0;
    cpu.h = 0;
    cpu.l = 0;
    cpu.a = 0;

    cpu.flags.z = 0;
    cpu.flags.s = 0;
    cpu.flags.p = 0;
    cpu.flags.ac = 0;
    cpu.flags.cy = 0;

    return cpu;
}

void emulate8080(cpu8080* state, uint8_t* memory){

    uint8_t *code = &memory[state->pc];

    switch(*code){
        case NOP: break;
// load bytes into register pair
        case LXI_B:{ state->b = code[2]; state->c = code[1]; state->pc += 2; } break;
        case LXI_D:{ state->d = code[2]; state->e = code[1]; state->pc += 2; } break;
        case LXI_H:{ state->h = code[2]; state->l = code[1]; state->pc += 2; } break;
        case LXI_SP:{ state->sp = (code[2] << 8) | code[1];  state->pc += 2; } break;
// store A in memory address of register pair
        case STAX_B: { memory[(state->b << 8) | state->c] = state->a; }break;
        case STAX_D: { memory[(state->d << 8) | state->e] = state->a; }break;
        case SHLD: {
            uint16_t addr = (code[2] << 8) | code[1];
            memory[addr] = state->l;
            memory[addr+1] = state->h;
            state->pc += 2;
        }break;
        case STA: { memory[(code[2] << 8) | code[1]] = state->a; state->pc += 2; }break;
// increment register pair
        case INX_B:{
            uint16_t res = ((state->b << 8) | state->c) + 1;
            state->b = (res & 0xff00) >> 8;
            state->c = (res & 0x00ff);
        } break;
        case INX_D:{
            uint16_t res = ((state->d << 8) | state->e) + 1;
            state->d = (res & 0xff00) >> 8;
            state->e = (res & 0x00ff);
        } break;
        case INX_H:{
            uint16_t res = ((state->h << 8) | state->l) + 1;
            state->h = (res & 0xff00) >> 8;
            state->l = (res & 0x00ff);
        } break;
        case INX_SP:{ state->sp++; } break;
// increment registers
        case INR_B:{ state->b++; set_zspac_flags(&state->flags, state->b);} break;
        case INR_D:{ state->d++; set_zspac_flags(&state->flags, state->d);} break;
        case INR_H:{ state->h++; set_zspac_flags(&state->flags, state->h);} break;
        case INR_C:{ state->c++; set_zspac_flags(&state->flags, state->c);} break;
        case INR_E:{ state->e++; set_zspac_flags(&state->flags, state->e);} break;
        case INR_L:{ state->l++; set_zspac_flags(&state->flags, state->l);} break;
        case INR_A:{ state->a++; set_zspac_flags(&state->flags, state->a);} break;
        case INR_M:{ 
            memory[(state->h << 8) | state->l]++;
            set_zspac_flags(&state->flags, memory[(state->h << 8) | state->l]);
        } break;
// decrement registers
        case DCR_B:{ state->b--; set_zspac_flags(&state->flags, state->b);} break;
        case DCR_D:{ state->d--; set_zspac_flags(&state->flags, state->d);} break;
        case DCR_H:{ state->h--; set_zspac_flags(&state->flags, state->h);} break;
        case DCR_C:{ state->c--; set_zspac_flags(&state->flags, state->c);} break;
        case DCR_E:{ state->e--; set_zspac_flags(&state->flags, state->e);} break;
        case DCR_L:{ state->l--; set_zspac_flags(&state->flags, state->l);} break;
        case DCR_A:{ state->a--; set_zspac_flags(&state->flags, state->a);} break;
        case DCR_M:{ 
            memory[(state->h << 8) | state->l]--;
            set_zspac_flags(&state->flags, memory[(state->h << 8) | state->l]);
        } break;
// move byte into register
        case MVI_B:{ state->b = code[1]; state->pc+=1; } break;
        case MVI_D:{ state->d = code[1]; state->pc+=1; } break;
        case MVI_H:{ state->h = code[1]; state->pc+=1; } break;
        case MVI_C:{ state->c = code[1]; state->pc+=1; } break;
        case MVI_E:{ state->e = code[1]; state->pc+=1; } break;
        case MVI_L:{ state->l = code[1]; state->pc+=1; } break;
        case MVI_A:{ state->a = code[1]; state->pc+=1; } break;
        case MVI_M:{ memory[(state->h << 8) | state->l] = code[1]; state->pc+=1; } break;
// shift operation on accumulator
        case RLC:{
            uint8_t x = state->a;
            state->a = ((x & 128) >> 7) | (x << 1);
            state->flags.cy = (x & 128) == 128;
        } break;
        case RRC:{
            uint8_t x = state->a;
            state->a = ((x & 1) << 7) | (x >> 1);
            state->flags.cy = (x & 1) == 1;
        } break;
        case RAL:{
            uint8_t x = state->a;
            state->a = state->flags.cy | (x << 1);
            state->flags.cy = (x & 128) == 128;
        } break;
        case RAR:{
            uint8_t x = state->a;
            state->a = state->flags.cy | (x >> 1);
            state->flags.cy = (x & 1) == 1;
        } break;
// add register pair to HL pair
        case DAD_B:{
            uint16_t HL = (state->h << 8) | state->l;
            uint16_t BC = (state->b << 8) | state->c;
            uint32_t res = HL + BC;
            state->flags.cy = res > 0xffff;
            state->h = (res & 0xff00) >> 8;
            state->l = res & 0xff;
        } break;
        case DAD_D:{
            uint16_t HL = (state->h << 8) | state->l;
            uint16_t DE = (state->d << 8) | state->e;
            uint32_t res = HL + DE;
            state->flags.cy = res > 0xffff;
            state->h = (res & 0xff00) >> 8;
            state->l = res & 0xff;
        } break;
        case DAD_H:{
            uint16_t HL = (state->h << 8) | state->l;
            uint32_t res = HL + HL;
            state->flags.cy = res > 0xffff;
            state->h = (res & 0xff00) >> 8;
            state->l = res & 0xff;
        } break;
        case DAD_SP:{
            uint16_t HL = (state->h << 8) | state->l;
            uint32_t res = HL + state->sp;
            state->flags.cy = res > 0xffff;
            state->h = (res & 0xff00) >> 8;
            state->l = res & 0xff;
        } break;
// read from address identified by register pair to A 
        case LDAX_B:{ state->a = memory[(state->b << 8) | state->c]; } break;
        case LDAX_D:{ state->a = memory[(state->d << 8) | state->e]; } break;
        case LHLD: {
            uint16_t addr = (code[2] << 8) | code[1];
            state->l = memory[addr];
            state->h = memory[addr+1];
            state->pc += 2;
        }
        case LDA:{ state->a = memory[(code[2] << 8) | code[1]]; } break;
// decrement register pair
        case DCX_B:{
            uint16_t res = ((state->b << 8) | state->c) - 1;
            state->b = (res & 0xff00) >> 8;
            state->c = (res & 0x00ff);
        } break;
        case DCX_D:{
            uint16_t res = ((state->d << 8) | state->e) - 1;
            state->d = (res & 0xff00) >> 8;
            state->e = (res & 0x00ff);
        } break;
        case DCX_H:{
            uint16_t res = ((state->h << 8) | state->l) - 1;
            state->h = (res & 0xff00) >> 8;
            state->l = (res & 0x00ff);
        } break;
        case DCX_SP:{ state->sp--; } break;
// set flags
        case STC: state->flags.cy = 1; break;
        case CMC: state->flags.cy = ~state->flags.cy; break;
// logic operations
        case CMA: state->a = ~state->a; break;
// move from register to register
        case MOV_B_B: break;
        case MOV_B_C: state->b = state->c; break;
        case MOV_B_D: state->b = state->d; break;
        case MOV_B_E: state->b = state->e; break;
        case MOV_B_H: state->b = state->h; break;
        case MOV_B_L: state->b = state->l; break;
        case MOV_B_M: state->b = memory[(state->h << 8) | state->l]; break;
        case MOV_B_A: state->b = state->a; break;
        case MOV_C_B: state->c = state->b; break;
        case MOV_C_C: break;
        case MOV_C_D: state->c = state->d; break;
        case MOV_C_E: state->c = state->e; break;
        case MOV_C_H: state->c = state->h; break;
        case MOV_C_L: state->c = state->l; break;
        case MOV_C_M: state->c = memory[(state->h << 8) | state->l]; break;
        case MOV_C_A: state->c = state->a; break;
        case MOV_D_B: state->d = state->b; break;
        case MOV_D_C: state->d = state->c; break;
        case MOV_D_D: break;
        case MOV_D_E: state->d = state->e; break;
        case MOV_D_H: state->d = state->h; break;
        case MOV_D_L: state->d = state->l; break;
        case MOV_D_M: state->d = memory[(state->h << 8) | state->l]; break;
        case MOV_D_A: state->d = state->a; break;
        case MOV_E_B: state->e = state->b; break;
        case MOV_E_C: state->e = state->c; break;
        case MOV_E_D: state->e = state->d; break;
        case MOV_E_E: break;
        case MOV_E_H: state->e = state->h; break;
        case MOV_E_L: state->e = state->l; break;
        case MOV_E_M: state->e = memory[(state->h << 8) | state->l]; break;
        case MOV_E_A: state->e = state->a; break;
        case MOV_H_B: state->h = state->b; break;
        case MOV_H_C: state->h = state->c; break;
        case MOV_H_D: state->h = state->d; break;
        case MOV_H_E: state->h = state->e; break;
        case MOV_H_H: break;
        case MOV_H_L: state->h = state->l; break;
        case MOV_H_M: state->h = memory[(state->h << 8) | state->l]; break;
        case MOV_H_A: state->h = state->a; break;
        case MOV_L_B: state->l = state->b; break;
        case MOV_L_C: state->l = state->c; break;
        case MOV_L_D: state->l = state->d; break;
        case MOV_L_E: state->l = state->e; break;
        case MOV_L_H: state->l = state->h; break;
        case MOV_L_L: break;
        case MOV_L_M: state->l = memory[(state->h << 8) | state->l]; break;
        case MOV_L_A: state->l = state->a; break;
        case MOV_M_B: memory[(state->h << 8) | state->l] = state->b; break;
        case MOV_M_C: memory[(state->h << 8) | state->l] = state->c; break;
        case MOV_M_D: memory[(state->h << 8) | state->l] = state->d; break;
        case MOV_M_E: memory[(state->h << 8) | state->l] = state->e; break;
        case MOV_M_H: memory[(state->h << 8) | state->l] = state->h; break;
        case MOV_M_L: memory[(state->h << 8) | state->l] = state->l; break;
        case MOV_M_A: memory[(state->h << 8) | state->l] = state->a; break;
        case MOV_A_B: state->a = state->b; break;
        case MOV_A_C: state->a = state->c; break;
        case MOV_A_D: state->a = state->d; break;
        case MOV_A_E: state->a = state->e; break;
        case MOV_A_H: state->a = state->h; break;
        case MOV_A_L: state->a = state->l; break;
        case MOV_A_M: state->a = memory[(state->h << 8) | state->l]; break;
        case MOV_A_A: break;
// add register to A
        case ADD_B: func_on_A('+', state, state->b); break;
        case ADD_C: func_on_A('+', state, state->c); break;
        case ADD_D: func_on_A('+', state, state->d); break;
        case ADD_E: func_on_A('+', state, state->e); break;
        case ADD_H: func_on_A('+', state, state->h); break;
        case ADD_L: func_on_A('+', state, state->l); break;
        case ADD_M: func_on_A('+', state, memory[state->h << 8 | state->l]); break;
        case ADD_A: func_on_A('+', state, state->a); break;
// subtract register from A
        case SUB_B: func_on_A('-', state, state->b); break;
        case SUB_C: func_on_A('-', state, state->c); break;
        case SUB_D: func_on_A('-', state, state->d); break;
        case SUB_E: func_on_A('-', state, state->e); break;
        case SUB_H: func_on_A('-', state, state->h); break;
        case SUB_L: func_on_A('-', state, state->l); break;
        case SUB_M: func_on_A('-', state, memory[state->h << 8 | state->l]); break;
        case SUB_A: func_on_A('-', state, state->a); break;
// AND register with A
        case ANA_B: func_on_A('&', state, state->b); break;
        case ANA_C: func_on_A('&', state, state->c); break;
        case ANA_D: func_on_A('&', state, state->d); break;
        case ANA_E: func_on_A('&', state, state->e); break;
        case ANA_H: func_on_A('&', state, state->h); break;
        case ANA_L: func_on_A('&', state, state->l); break;
        case ANA_M: func_on_A('&', state, memory[state->h << 8 | state->l]); break;
        case ANA_A: func_on_A('&', state, state->a); break;
// XOR register with A
        case XRA_B: func_on_A('^', state, state->b); break;
        case XRA_C: func_on_A('^', state, state->c); break;
        case XRA_D: func_on_A('^', state, state->d); break;
        case XRA_E: func_on_A('^', state, state->e); break;
        case XRA_H: func_on_A('^', state, state->h); break;
        case XRA_L: func_on_A('^', state, state->l); break;
        case XRA_M: func_on_A('^', state, memory[state->h << 8 | state->l]); break;
        case XRA_A: func_on_A('^', state, state->a); break;
// OR register with A
        case ORA_B: func_on_A('|', state, state->b); break;
        case ORA_C: func_on_A('|', state, state->c); break;
        case ORA_D: func_on_A('|', state, state->d); break;
        case ORA_E: func_on_A('|', state, state->e); break;
        case ORA_H: func_on_A('|', state, state->h); break;
        case ORA_L: func_on_A('|', state, state->l); break;
        case ORA_M: func_on_A('|', state, memory[state->h << 8 | state->l]); break;
        case ORA_A: func_on_A('|', state, state->a); break;        
// add with carry register to A
        case ADC_B: func_on_A('#', state, state->b); break;
        case ADC_C: func_on_A('#', state, state->c); break;
        case ADC_D: func_on_A('#', state, state->d); break;
        case ADC_E: func_on_A('#', state, state->e); break;
        case ADC_H: func_on_A('#', state, state->h); break;
        case ADC_L: func_on_A('#', state, state->l); break;
        case ADC_M: func_on_A('#', state, memory[state->h << 8 | state->l]); break;
        case ADC_A: func_on_A('#', state, state->a); break;
// subtract with carry register with A
        case SBB_B: func_on_A('@', state, state->b); break;
        case SBB_C: func_on_A('@', state, state->c); break;
        case SBB_D: func_on_A('@', state, state->d); break;
        case SBB_E: func_on_A('@', state, state->e); break;
        case SBB_H: func_on_A('@', state, state->h); break;
        case SBB_L: func_on_A('@', state, state->l); break;
        case SBB_M: func_on_A('@', state, memory[state->h << 8 | state->l]); break;
        case SBB_A: func_on_A('@', state, state->a); break;


        default: printf("Error: unhandled opcode\n"); break;
    }
    state->pc += 1;
}

int disassemble8080(uint8_t *memory, int pc){

    unsigned char *code = &memory[pc];
    int opbytes = 1;
    printf("0x%04x  ", pc);
    switch(*code){
        case 0x00: printf("NOP"); break;
        case 0x01: printf("LXI    B, #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0x02: printf("STAX   B"); break;
        case 0x03: printf("INX    B"); break;
        case 0x04: printf("INR    B"); break;
        case 0x05: printf("DCR    B"); break;
        case 0x06: printf("MVI    B, #%02x", code[1]); opbytes = 2; break;
        case 0x07: printf("RLC"); break;
        case 0x09: printf("DAD    B"); break;
        case 0x0a: printf("LDAX   B"); break;
        case 0x0b: printf("DCX    B"); break;
        case 0x0c: printf("INR    C"); break;
        case 0x0d: printf("DCR    C"); break;
        case 0x0e: printf("MVI    C, #%02x", code[1]); opbytes = 2; break;
        case 0x0f: printf("RRC"); break;
        case 0x11: printf("LXI    D, #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0x12: printf("STAX   D"); break;
        case 0x13: printf("INX    D"); break;
        case 0x14: printf("INR    D"); break;
        case 0x15: printf("DCR    D"); break;
        case 0x16: printf("MVI    D, #%02x", code[1]); opbytes = 2; break;
        case 0x17: printf("RAL"); break;
        case 0x19: printf("DAD    D"); break;
        case 0x1a: printf("LDAX   D"); break;
        case 0x1b: printf("DCX    D"); break;
        case 0x1c: printf("INR    E"); break;
        case 0x1d: printf("DCR    E"); break;
        case 0x1e: printf("MVI    D, #%02x", code[1]); opbytes = 2; break;
        case 0x1f: printf("RAR"); break;
        case 0x20: printf("RIM"); break;
        case 0x21: printf("LXI    H, #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0x22: printf("SHLD   #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0x23: printf("INX    H"); break;
        case 0x24: printf("INR    H"); break;
        case 0x25: printf("DCR    H"); break;
        case 0x26: printf("MVI    H, #%02x", code[1]); opbytes = 2; break;
        case 0x27: printf("DAA"); break;
        case 0x29: printf("DAD    H"); break;
        case 0x2a: printf("LHLD   #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0x2b: printf("DCX    H"); break;
        case 0x2c: printf("INR    L"); break;
        case 0x2d: printf("DCR    L"); break;
        case 0x2e: printf("MVI    L, #%02x", code[1]); opbytes = 2; break;
        case 0x2f: printf("CMA"); break;
        case 0x30: printf("SIM"); break; // here
        case 0x31: printf("LXI    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0x32: printf("STA    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0x33: printf("INX    SP"); break;
        case 0x34: printf("INR    M"); break;
        case 0x35: printf("DCR    M"); break;
        case 0x36: printf("MVI    M, #%02x", code[1]); opbytes = 2; break;
        case 0x37: printf("STC"); break; // gap
        case 0x39: printf("DAD    SP"); break;
        case 0x3a: printf("LDA    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0x3b: printf("DCX    SP"); break;
        case 0x3c: printf("INR    A"); break;
        case 0x3d: printf("DCR    A"); break;
        case 0x3e: printf("MVI    A, #%02x", code[1]); opbytes = 2; break;
        case 0x3f: printf("CMC"); break;
        case 0x40: printf("MOV    B, B"); break;
        case 0x41: printf("MOV    B, C"); break;
        case 0x42: printf("MOV    B, D"); break;
        case 0x43: printf("MOV    B, E"); break;
        case 0x44: printf("MOV    B, H"); break;
        case 0x45: printf("MOV    B, L"); break;
        case 0x46: printf("MOV    B, M"); break;
        case 0x47: printf("MOV    B, A"); break;

        case 0x48: printf("MOV    C, B"); break;
        case 0x49: printf("MOV    C, C"); break;
        case 0x4a: printf("MOV    C, D"); break;
        case 0x4b: printf("MOV    C, E"); break;
        case 0x4c: printf("MOV    C, H"); break;
        case 0x4d: printf("MOV    C, L"); break;
        case 0x4e: printf("MOV    C, M"); break;
        case 0x4f: printf("MOV    C, A"); break;

        case 0x50: printf("MOV    D, B"); break;
        case 0x51: printf("MOV    D, C"); break;
        case 0x52: printf("MOV    D, D"); break;
        case 0x53: printf("MOV    D, E"); break;
        case 0x54: printf("MOV    D, H"); break;
        case 0x55: printf("MOV    D, L"); break;
        case 0x56: printf("MOV    D, M"); break;
        case 0x57: printf("MOV    D, A"); break;

        case 0x58: printf("MOV    E, B"); break;
        case 0x59: printf("MOV    E, C"); break;
        case 0x5a: printf("MOV    E, D"); break;
        case 0x5b: printf("MOV    E, E"); break;
        case 0x5c: printf("MOV    E, H"); break;
        case 0x5d: printf("MOV    E, L"); break;
        case 0x5e: printf("MOV    E, M"); break;
        case 0x5f: printf("MOV    E, A"); break;

        case 0x60: printf("MOV    H, B"); break;
        case 0x61: printf("MOV    H, C"); break;
        case 0x62: printf("MOV    H, D"); break;
        case 0x63: printf("MOV    H, E"); break;
        case 0x64: printf("MOV    H, H"); break;
        case 0x65: printf("MOV    H, L"); break;
        case 0x66: printf("MOV    H, M"); break;
        case 0x67: printf("MOV    H, A"); break;

        case 0x68: printf("MOV    L, B"); break;
        case 0x69: printf("MOV    L, C"); break;
        case 0x6a: printf("MOV    L, D"); break;
        case 0x6b: printf("MOV    L, E"); break;
        case 0x6c: printf("MOV    L, H"); break;
        case 0x6d: printf("MOV    L, L"); break;
        case 0x6e: printf("MOV    L, M"); break;
        case 0x6f: printf("MOV    L, A"); break;

        case 0x70: printf("MOV    M, B"); break;
        case 0x71: printf("MOV    M, C"); break;
        case 0x72: printf("MOV    M, D"); break;
        case 0x73: printf("MOV    M, E"); break;
        case 0x74: printf("MOV    M, H"); break;
        case 0x75: printf("MOV    M, L"); break;
        case 0x76: printf("HLT"); break;
        case 0x77: printf("MOV    M, A"); break;

        case 0x78: printf("MOV    A, B"); break;
        case 0x79: printf("MOV    A, C"); break;
        case 0x7a: printf("MOV    A, D"); break;
        case 0x7b: printf("MOV    A, E"); break;
        case 0x7c: printf("MOV    A, H"); break;
        case 0x7d: printf("MOV    A, L"); break;
        case 0x7e: printf("MOV    A, M"); break;
        case 0x7f: printf("MOV    A, A"); break;

        case 0x80: printf("ADD    B"); break;
        case 0x81: printf("ADD    C"); break;
        case 0x82: printf("ADD    D"); break;
        case 0x83: printf("ADD    E"); break;
        case 0x84: printf("ADD    H"); break;
        case 0x85: printf("ADD    L"); break;
        case 0x86: printf("ADD    M"); break;
        case 0x87: printf("ADD    A"); break;

        case 0x88: printf("ADC    B"); break;
        case 0x89: printf("ADC    C"); break;
        case 0x8a: printf("ADC    D"); break;
        case 0x8b: printf("ADC    E"); break;
        case 0x8c: printf("ADC    H"); break;
        case 0x8d: printf("ADC    L"); break;
        case 0x8e: printf("ADC    M"); break;
        case 0x8f: printf("ADC    A"); break;

        case 0x90: printf("SUB    B"); break;
        case 0x91: printf("SUB    C"); break;
        case 0x92: printf("SUB    D"); break;
        case 0x93: printf("SUB    E"); break;
        case 0x94: printf("SUB    H"); break;
        case 0x95: printf("SUB    L"); break;
        case 0x96: printf("SUB    M"); break;
        case 0x97: printf("SUB    A"); break;

        case 0x98: printf("SBB    B"); break;
        case 0x99: printf("SBB    C"); break;
        case 0x9a: printf("SBB    D"); break;
        case 0x9b: printf("SBB    E"); break;
        case 0x9c: printf("SBB    H"); break;
        case 0x9d: printf("SBB    L"); break;
        case 0x9e: printf("SBB    M"); break;
        case 0x9f: printf("SBB    A"); break;

        case 0xa0: printf("ANA    B"); break;
        case 0xa1: printf("ANA    C"); break;
        case 0xa2: printf("ANA    D"); break;
        case 0xa3: printf("ANA    E"); break;
        case 0xa4: printf("ANA    H"); break;
        case 0xa5: printf("ANA    L"); break;
        case 0xa6: printf("ANA    M"); break;
        case 0xa7: printf("ANA    A"); break;

        case 0xa8: printf("XRA    B"); break;
        case 0xa9: printf("XRA    C"); break;
        case 0xaa: printf("XRA    D"); break;
        case 0xab: printf("XRA    E"); break;
        case 0xac: printf("XRA    H"); break;
        case 0xad: printf("XRA    L"); break;
        case 0xae: printf("XRA    M"); break;
        case 0xaf: printf("XRA    A"); break;

        case 0xb0: printf("ORA    B"); break;
        case 0xb1: printf("ORA    C"); break;
        case 0xb2: printf("ORA    D"); break;
        case 0xb3: printf("ORA    E"); break;
        case 0xb4: printf("ORA    H"); break;
        case 0xb5: printf("ORA    L"); break;
        case 0xb6: printf("ORA    M"); break;
        case 0xb7: printf("ORA    A"); break;

        case 0xb8: printf("CMP    B"); break;
        case 0xb9: printf("CMP    C"); break;
        case 0xba: printf("CMP    D"); break;
        case 0xbb: printf("CMP    E"); break;
        case 0xbc: printf("CMP    H"); break;
        case 0xbd: printf("CMP    L"); break;
        case 0xbe: printf("CMP    M"); break;
        case 0xbf: printf("CMP    A"); break;

        case 0xc0: printf("RNZ"); break;
        case 0xc1: printf("POP    B"); break;
        case 0xc2: printf("JNZ    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xc3: printf("JMP    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xc4: printf("CNZ    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xc5: printf("PUSH   B"); break;
        case 0xc6: printf("ADI    #%02x", code[1]); opbytes = 2; break;
        case 0xc7: printf("RST    0"); break;
        case 0xc8: printf("RZ"); break;
        case 0xc9: printf("RET"); break;
        case 0xca: printf("JZ     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        // gap
        case 0xcc: printf("CZ     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xcd: printf("CALL   #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xce: printf("ACI    #%02x", code[1]); opbytes = 2; break;
        case 0xcf: printf("RST    1"); break;
        case 0xd0: printf("RNC"); break;
        case 0xd1: printf("POP    D"); break;
        case 0xd2: printf("JNC    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xd3: printf("OUT    #%02x", code[1]); opbytes = 2; break;
        case 0xd4: printf("CNC    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xd5: printf("PUSH   D"); break;
        case 0xd6: printf("SUI    #%02x", code[1]); opbytes = 2; break;
        case 0xd7: printf("RST    2"); break;
        case 0xd8: printf("RC"); break;
        //gap
        case 0xda: printf("JC     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xdb: printf("IN     #%02x", code[1]); opbytes = 2; break;
        case 0xdc: printf("CC     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        //gap
        case 0xde: printf("SBI    #%02x", code[1]); opbytes = 2; break;
        case 0xdf: printf("RST    3"); break;
        case 0xe0: printf("RPO"); break;
        case 0xe1: printf("POP    H"); break;
        case 0xe2: printf("JPO    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xe3: printf("XTHL"); break;
        case 0xe4: printf("CPO    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xe5: printf("PUSH   H"); break;
        case 0xe6: printf("ANI    #%02x", code[1]); opbytes = 2; break;
        case 0xe7: printf("RST    4"); break;
        case 0xe8: printf("RPE"); break;
        case 0xe9: printf("PCHL"); break;
        case 0xea: printf("JPE    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xeb: printf("XCHG"); break;
        case 0xec: printf("CPE    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        //gap
        case 0xee: printf("XRI    #%02x", code[1]); opbytes = 2; break;
        case 0xef: printf("RST    5"); break;
        case 0xf0: printf("RP"); break;
        case 0xf1: printf("POP    PSW"); break;
        case 0xf2: printf("JP     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xf3: printf("DI"); break;
        case 0xf4: printf("CP     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xf5: printf("PUSH   PSW"); break;
        case 0xf6: printf("ORI    #%02x", code[1]); opbytes = 2; break;
        case 0xf7: printf("RST    6"); break;
        case 0xf8: printf("RM"); break;
        case 0xf9: printf("SPHL"); break;
        case 0xfa: printf("JM     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0xfb: printf("EI"); break;
        case 0xfc: printf("CM     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        //gap
        case 0xfe: printf("CPI    #%02x", code[1]); opbytes = 2; break;
        case 0xff: printf("RST    7"); break;
        default:
            printf("Error: unhandled opcode");
            break;
    }
    printf("\n");

    return opbytes;
}