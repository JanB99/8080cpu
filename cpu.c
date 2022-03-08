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
        res = state->a + val;
    } else if (operation == '-'){
        res = state->a - val;
    } else if (operation == '&'){
        res = state->a & val;
    } else if (operation == '|'){
        res = state->a | val;
    } else if (operation == '^'){
        res = state->a ^ val;
    } else if (operation == '#'){ // add with carry
        res = state->a + val + state->flags.cy;
    } else if (operation == '@'){ // subtract with carry
        res = state->a - val - state->flags.cy;
    }

    state->flags.cy = res > 0xff;
    state->a = res & 0xff;
    set_zspac_flags(&state->flags, state->a);
}

void cond_jump(cpu8080* state, uint16_t addr, uint8_t condition){
    if (condition){
        state->pc = addr - 1; // end of switch statement pc gets incremented, this is to account for that
    } else {
        state->pc += 2;
    }
}

void cond_call(cpu8080* state, uint8_t* memory, uint16_t addr, uint8_t condition){
    if (condition){
        uint16_t ret = state->pc + 2;
        memory[state->sp - 1] = ret >> 8 & 0xff;  // mem sp-1 = pc HI
        memory[state->sp - 2] = ret & 0xff;       // mem sp-2 = pc LO
        state->sp-=2;
        state->pc = addr - 1;
    } else {
        state->pc += 2;
    }
}

void cond_ret(cpu8080* state, uint8_t* memory, uint8_t condition){
    if (condition){
        state->pc = ((memory[state->sp+1] << 8) | memory[state->sp]) - 1;
        printf("RET: %04x\tSP mem: %04x - %04x\n", state->pc, state->sp+1, state->sp);
        state->sp += 2;
    }
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
        case 0x10:
        case 0x20:
        case 0x30:
        case 0x08:
        case 0x18:
        case 0x28:
        case 0x38:
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
            state->a = (state->flags.cy >> 7) | (x << 1);
            state->flags.cy = (x & 128) == 128;
        } break;
        case RAR:{
            uint8_t x = state->a;
            state->a = (state->flags.cy << 7) | (x >> 1);
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
        } break;
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
        case ADI:   func_on_A('+', state, code[1]); state->pc++; break;
// subtract register from A
        case SUB_B: func_on_A('-', state, state->b); break;
        case SUB_C: func_on_A('-', state, state->c); break;
        case SUB_D: func_on_A('-', state, state->d); break;
        case SUB_E: func_on_A('-', state, state->e); break;
        case SUB_H: func_on_A('-', state, state->h); break;
        case SUB_L: func_on_A('-', state, state->l); break;
        case SUB_M: func_on_A('-', state, memory[state->h << 8 | state->l]); break;
        case SUB_A: func_on_A('-', state, state->a); break;
        case SUI:   func_on_A('-', state, code[1]); state->pc++; break;
// AND register with A
        case ANA_B: func_on_A('&', state, state->b); break;
        case ANA_C: func_on_A('&', state, state->c); break;
        case ANA_D: func_on_A('&', state, state->d); break;
        case ANA_E: func_on_A('&', state, state->e); break;
        case ANA_H: func_on_A('&', state, state->h); break;
        case ANA_L: func_on_A('&', state, state->l); break;
        case ANA_M: func_on_A('&', state, memory[state->h << 8 | state->l]); break;
        case ANA_A: func_on_A('&', state, state->a); break;
        case ANI:   func_on_A('&', state, code[1]); state->pc++; break;
// XOR register with A
        case XRA_B: func_on_A('^', state, state->b); break;
        case XRA_C: func_on_A('^', state, state->c); break;
        case XRA_D: func_on_A('^', state, state->d); break;
        case XRA_E: func_on_A('^', state, state->e); break;
        case XRA_H: func_on_A('^', state, state->h); break;
        case XRA_L: func_on_A('^', state, state->l); break;
        case XRA_M: func_on_A('^', state, memory[state->h << 8 | state->l]); break;
        case XRA_A: func_on_A('^', state, state->a); break;
        case XRI:   func_on_A('^', state, code[1]); state->pc++; break;
// OR register with A
        case ORA_B: func_on_A('|', state, state->b); break;
        case ORA_C: func_on_A('|', state, state->c); break;
        case ORA_D: func_on_A('|', state, state->d); break;
        case ORA_E: func_on_A('|', state, state->e); break;
        case ORA_H: func_on_A('|', state, state->h); break;
        case ORA_L: func_on_A('|', state, state->l); break;
        case ORA_M: func_on_A('|', state, memory[state->h << 8 | state->l]); break;
        case ORA_A: func_on_A('|', state, state->a); break;
        case ORI:   func_on_A('|', state, code[1]); state->pc++; break;    
// add with carry register to A
        case ADC_B: func_on_A('#', state, state->b); break;
        case ADC_C: func_on_A('#', state, state->c); break;
        case ADC_D: func_on_A('#', state, state->d); break;
        case ADC_E: func_on_A('#', state, state->e); break;
        case ADC_H: func_on_A('#', state, state->h); break;
        case ADC_L: func_on_A('#', state, state->l); break;
        case ADC_M: func_on_A('#', state, memory[state->h << 8 | state->l]); break;
        case ADC_A: func_on_A('#', state, state->a); break;
        case ACI:   func_on_A('#', state, code[1]); state->pc++; break;
// subtract with carry register with A
        case SBB_B: func_on_A('@', state, state->b); break;
        case SBB_C: func_on_A('@', state, state->c); break;
        case SBB_D: func_on_A('@', state, state->d); break;
        case SBB_E: func_on_A('@', state, state->e); break;
        case SBB_H: func_on_A('@', state, state->h); break;
        case SBB_L: func_on_A('@', state, state->l); break;
        case SBB_M: func_on_A('@', state, memory[state->h << 8 | state->l]); break;
        case SBB_A: func_on_A('@', state, state->a); break;
        case SBI:   func_on_A('@', state, code[1]); state->pc++; break;
// compare register to A
        case CMP_B: {
            uint16_t res = state->a - state->b;
            state->flags.cy = res > 0xff;
            set_zspac_flags(&state->flags, res & 0xff); 
        }; break;
        case CMP_C: {
            uint16_t res = state->a - state->c;
            state->flags.cy = res > 0xff;
            set_zspac_flags(&state->flags, res & 0xff); 
        }; break;
        case CMP_D: {
            uint16_t res = state->a - state->d;
            state->flags.cy = res > 0xff;
            set_zspac_flags(&state->flags, res & 0xff); 
        }; break;
        case CMP_E: {
            uint16_t res = state->a - state->e;
            state->flags.cy = res > 0xff;
            set_zspac_flags(&state->flags, res & 0xff); 
        }; break;
        case CMP_H: {
            uint16_t res = state->a - state->h;
            state->flags.cy = res > 0xff;
            set_zspac_flags(&state->flags, res & 0xff); 
        }; break;
        case CMP_L: {
            uint16_t res = state->a - state->l;
            state->flags.cy = res > 0xff;
            set_zspac_flags(&state->flags, res & 0xff); 
        }; break;
        case CMP_M: {
            uint16_t res = state->a - memory[state->h << 8 | state->l];
            state->flags.cy = res > 0xff;
            set_zspac_flags(&state->flags, res & 0xff); 
        }; break;
        case CMP_A: {
            uint16_t res = state->a - state->a;
            state->flags.cy = res > 0xff;
            set_zspac_flags(&state->flags, res & 0xff); 
        }; break;
        case CPI: {
            uint16_t res = state->a - code[1];
            state->flags.cy = res > 0xff;
            set_zspac_flags(&state->flags, res & 0xff);
            state->pc++;
        }; break;
// miscelaneous 
        case EI: state->interrupt_enable = 1; break;
        case DI: state->interrupt_enable = 0; break;
        case HLT: exit(0); break;
        case IN: state->pc++; break;
        case OUT: state->pc++; break;
// jumps
        case 0xcb:
        case JMP: cond_jump(state, code[2] << 8 | code[1], 1); break;
        case JZ: cond_jump(state, code[2] << 8 | code[1], state->flags.z == 1); break;
        case JNZ: cond_jump(state, code[2] << 8 | code[1], state->flags.z == 0); break;
        case JC: cond_jump(state, code[2] << 8 | code[1], state->flags.cy == 1); break;
        case JNC: cond_jump(state, code[2] << 8 | code[1], state->flags.cy == 0); break;
        case JPE: cond_jump(state, code[2] << 8 | code[1], state->flags.p == 1); break; 
        case JPO: cond_jump(state, code[2] << 8 | code[1], state->flags.p == 0); break; 
        case JP: cond_jump(state, code[2] << 8 | code[1], state->flags.s == 0); break;
        case JM: cond_jump(state, code[2] << 8 | code[1], state->flags.s == 1); break; 
        case PCHL: cond_jump(state, state->h << 8 | state->l, 1); break;
// Call
        case 0xdd:
        case 0xed:
        case 0xfd:
        case CALL: cond_call(state, memory, code[2] << 8 | code[1], 1); break;
        case CZ: cond_call(state, memory, code[2] << 8 | code[1], state->flags.z == 1); break;
        case CNZ: cond_call(state, memory, code[2] << 8 | code[1], state->flags.z == 0); break;
        case CC: cond_call(state, memory, code[2] << 8 | code[1], state->flags.cy == 1); break;
        case CNC: cond_call(state, memory, code[2] << 8 | code[1], state->flags.cy == 0); break;
        case CPE: cond_call(state, memory, code[2] << 8 | code[1], state->flags.p == 1); break;
        case CPO: cond_call(state, memory, code[2] << 8 | code[1], state->flags.p == 0); break;
        case CP: cond_call(state, memory, code[2] << 8 | code[1], state->flags.s == 0); break;
        case CM: cond_call(state, memory, code[2] << 8 | code[1], state->flags.s == 1); break;
        case RST_0: cond_call(state, memory, 0x00, 1); break;
        case RST_1: cond_call(state, memory, 0x08, 1); break;
        case RST_2: cond_call(state, memory, 0x10, 1); break;
        case RST_3: cond_call(state, memory, 0x18, 1); break;
        case RST_4: cond_call(state, memory, 0x20, 1); break;
        case RST_5: cond_call(state, memory, 0x28, 1); break;
        case RST_6: cond_call(state, memory, 0x30, 1); break;
        case RST_7: cond_call(state, memory, 0x38, 1); break;
// Return
        case 0xd9:
        case RET: cond_ret(state, memory, 1); break;
        case RZ:  cond_ret(state, memory, state->flags.z == 1); break;
        case RNZ: cond_ret(state, memory, state->flags.z == 0); break;
        case RC:  cond_ret(state, memory, state->flags.cy == 1); break;
        case RNC: cond_ret(state, memory, state->flags.cy == 0); break;
        case RPE: cond_ret(state, memory, state->flags.p == 1); break;
        case RPO: cond_ret(state, memory, state->flags.p == 0); break;
        case RP:  cond_ret(state, memory, state->flags.s == 0); break;
        case RM:  cond_ret(state, memory, state->flags.s == 1); break;
// push pop
        case PUSH_B: memory[state->sp-2] = state->c; memory[state->sp-1] = state->b; state->sp -= 2; break;
        case PUSH_D: memory[state->sp-2] = state->e; memory[state->sp-1] = state->d; state->sp -= 2; break;
        case PUSH_H: memory[state->sp-2] = state->l; memory[state->sp-1] = state->h; state->sp -= 2; break;
        case PUSH_PSW: {
            memory[state->sp-1] = state->a;
            uint8_t psw =   state->flags.z | 
                            state->flags.s << 1 | 
                            state->flags.cy << 2 |
                            state->flags.p << 3 |
                            state->flags.ac << 4;
            memory[state->sp-2] = psw;
            state->sp -= 2;
        } break;

        case POP_B: state->b = memory[state->sp+1]; state->c = memory[state->sp]; state->sp += 2; break;
        case POP_D: state->d = memory[state->sp+1]; state->e = memory[state->sp]; state->sp += 2; break;
        case POP_H: state->h = memory[state->sp+1]; state->l = memory[state->sp]; state->sp += 2; break;
        case POP_PSW: {
            state->a = memory[state->sp+1];
            state->flags.z = (memory[state->sp] & 0x01) == 0x01; 
            state->flags.s = (memory[state->sp] & 0x02) == 0x02;
            state->flags.cy = (memory[state->sp] & 0x04) == 0x04;
            state->flags.p = (memory[state->sp] & 0x08) == 0x08;
            state->flags.ac = (memory[state->sp] & 0x10) == 0x10;
        } break;

        case SPHL: state->sp = state->h << 8 | state->l; break;
        case XTHL: {
            uint16_t tmp = state->h << 8 | state->l;
            state->l = memory[state->sp];
            state->h = memory[state->sp+1];
            memory[state->sp] = (tmp >> 8) & 0xff;
            memory[state->sp+1] = tmp & 0xff;
        } break;
        case XCHG: {
            uint16_t tmp = state->h << 8 | state->l;
            state->l = state->e;
            state->h = state->d;
            state->d = (tmp >> 8) & 0xff;
            state->e = tmp & 0xff;
        } break;

        default: printf("Error: unhandled opcode: 0x%02x\n", code[0]); exit(0); break;
    }
    state->pc += 1;
}

int disassemble8080(uint8_t *memory, int pc, uint64_t counter){

    unsigned char *code = &memory[pc];
    int opbytes = 1;
    printf("0x%04x  ", pc);
    switch(*code){
        case NOP: printf("NOP"); break;
        case LXI_B: printf("LXI    B, #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case STAX_B: printf("STAX   B"); break;
        case INX_B: printf("INX    B"); break;
        case INR_B: printf("INR    B"); break;
        case DCR_B: printf("DCR    B"); break;
        case MVI_B: printf("MVI    B, #%02x", code[1]); opbytes = 2; break;
        case RLC: printf("RLC"); break;
        case DAD_B: printf("DAD    B"); break;
        case LDAX_B: printf("LDAX   B"); break;
        case DCX_B: printf("DCX    B"); break;
        case INR_C: printf("INR    C"); break;
        case DCR_C: printf("DCR    C"); break;
        case MVI_C: printf("MVI    C, #%02x", code[1]); opbytes = 2; break;
        case RRC: printf("RRC"); break;
        case LXI_D: printf("LXI    D, #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case STAX_D: printf("STAX   D"); break;
        case INX_D: printf("INX    D"); break;
        case INR_D: printf("INR    D"); break;
        case DCR_D: printf("DCR    D"); break;
        case MVI_D: printf("MVI    D, #%02x", code[1]); opbytes = 2; break;
        case RAL: printf("RAL"); break;
        case DAD_D: printf("DAD    D"); break;
        case LDAX_D: printf("LDAX   D"); break;
        case DCX_D: printf("DCX    D"); break;
        case INR_E: printf("INR    E"); break;
        case DCR_E: printf("DCR    E"); break;
        case MVI_E: printf("MVI    E, #%02x", code[1]); opbytes = 2; break;
        case RAR: printf("RAR"); break;
        case LXI_H: printf("LXI    H, #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case SHLD: printf("SHLD   #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case INX_H: printf("INX    H"); break;
        case INR_H: printf("INR    H"); break;
        case DCR_H: printf("DCR    H"); break;
        case MVI_H: printf("MVI    H, #%02x", code[1]); opbytes = 2; break;
        case DAA: printf("DAA"); break;
        case DAD_H: printf("DAD    H"); break;
        case LHLD: printf("LHLD   #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case DCX_H: printf("DCX    H"); break;
        case INR_L: printf("INR    L"); break;
        case DCR_L: printf("DCR    L"); break;
        case MVI_L: printf("MVI    L, #%02x", code[1]); opbytes = 2; break;
        case CMA: printf("CMA"); break;
        case LXI_SP: printf("LXI    SP, #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case STA: printf("STA    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case INX_SP: printf("INX    SP"); break;
        case INR_M: printf("INR    M"); break;
        case DCR_M: printf("DCR    M"); break;
        case MVI_M: printf("MVI    M, #%02x", code[1]); opbytes = 2; break;
        case STC: printf("STC"); break;
        case DAD_SP: printf("DAD    SP"); break;
        case LDA: printf("LDA    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case DCX_SP: printf("DCX    SP"); break;
        case INR_A: printf("INR    A"); break;
        case DCR_A: printf("DCR    A"); break;
        case MVI_A: printf("MVI    A, #%02x", code[1]); opbytes = 2; break;
        case CMC: printf("CMC"); break;
        case MOV_B_B: printf("MOV    B, B"); break;
        case MOV_B_C: printf("MOV    B, C"); break;
        case MOV_B_D: printf("MOV    B, D"); break;
        case MOV_B_E: printf("MOV    B, E"); break;
        case MOV_B_H: printf("MOV    B, H"); break;
        case MOV_B_L: printf("MOV    B, L"); break;
        case MOV_B_M: printf("MOV    B, M"); break;
        case MOV_B_A: printf("MOV    B, A"); break;

        case MOV_C_B: printf("MOV    C, B"); break;
        case MOV_C_C: printf("MOV    C, C"); break;
        case MOV_C_D: printf("MOV    C, D"); break;
        case MOV_C_E: printf("MOV    C, E"); break;
        case MOV_C_H: printf("MOV    C, H"); break;
        case MOV_C_L: printf("MOV    C, L"); break;
        case MOV_C_M: printf("MOV    C, M"); break;
        case MOV_C_A: printf("MOV    C, A"); break;

        case MOV_D_B: printf("MOV    D, B"); break;
        case MOV_D_C: printf("MOV    D, C"); break;
        case MOV_D_D: printf("MOV    D, D"); break;
        case MOV_D_E: printf("MOV    D, E"); break;
        case MOV_D_H: printf("MOV    D, H"); break;
        case MOV_D_L: printf("MOV    D, L"); break;
        case MOV_D_M: printf("MOV    D, M"); break;
        case MOV_D_A: printf("MOV    D, A"); break;

        case MOV_E_B: printf("MOV    E, B"); break;
        case MOV_E_C: printf("MOV    E, C"); break;
        case MOV_E_D: printf("MOV    E, D"); break;
        case MOV_E_E: printf("MOV    E, E"); break;
        case MOV_E_H: printf("MOV    E, H"); break;
        case MOV_E_L: printf("MOV    E, L"); break;
        case MOV_E_M: printf("MOV    E, M"); break;
        case MOV_E_A: printf("MOV    E, A"); break;

        case MOV_H_B: printf("MOV    H, B"); break;
        case MOV_H_C: printf("MOV    H, C"); break;
        case MOV_H_D: printf("MOV    H, D"); break;
        case MOV_H_E: printf("MOV    H, E"); break;
        case MOV_H_H: printf("MOV    H, H"); break;
        case MOV_H_L: printf("MOV    H, L"); break;
        case MOV_H_M: printf("MOV    H, M"); break;
        case MOV_H_A: printf("MOV    H, A"); break;

        case MOV_L_B: printf("MOV    L, B"); break;
        case MOV_L_C: printf("MOV    L, C"); break;
        case MOV_L_D: printf("MOV    L, D"); break;
        case MOV_L_E: printf("MOV    L, E"); break;
        case MOV_L_H: printf("MOV    L, H"); break;
        case MOV_L_L: printf("MOV    L, L"); break;
        case MOV_L_M: printf("MOV    L, M"); break;
        case MOV_L_A: printf("MOV    L, A"); break;

        case MOV_M_B: printf("MOV    M, B"); break;
        case MOV_M_C: printf("MOV    M, C"); break;
        case MOV_M_D: printf("MOV    M, D"); break;
        case MOV_M_E: printf("MOV    M, E"); break;
        case MOV_M_H: printf("MOV    M, H"); break;
        case MOV_M_L: printf("MOV    M, L"); break;
        case HLT: printf("HLT"); break;
        case MOV_M_A: printf("MOV    M, A"); break;

        case MOV_A_B: printf("MOV    A, B"); break;
        case MOV_A_C: printf("MOV    A, C"); break;
        case MOV_A_D: printf("MOV    A, D"); break;
        case MOV_A_E: printf("MOV    A, E"); break;
        case MOV_A_H: printf("MOV    A, H"); break;
        case MOV_A_L: printf("MOV    A, L"); break;
        case MOV_A_M: printf("MOV    A, M"); break;
        case MOV_A_A: printf("MOV    A, A"); break;

        case ADD_B: printf("ADD    B"); break;
        case ADD_C: printf("ADD    C"); break;
        case ADD_D: printf("ADD    D"); break;
        case ADD_E: printf("ADD    E"); break;
        case ADD_H: printf("ADD    H"); break;
        case ADD_L: printf("ADD    L"); break;
        case ADD_M: printf("ADD    M"); break;
        case ADD_A: printf("ADD    A"); break;

        case ADC_B: printf("ADC    B"); break;
        case ADC_C: printf("ADC    C"); break;
        case ADC_D: printf("ADC    D"); break;
        case ADC_E: printf("ADC    E"); break;
        case ADC_H: printf("ADC    H"); break;
        case ADC_L: printf("ADC    L"); break;
        case ADC_M: printf("ADC    M"); break;
        case ADC_A: printf("ADC    A"); break;

        case SUB_B: printf("SUB    B"); break;
        case SUB_C: printf("SUB    C"); break;
        case SUB_D: printf("SUB    D"); break;
        case SUB_E: printf("SUB    E"); break;
        case SUB_H: printf("SUB    H"); break;
        case SUB_L: printf("SUB    L"); break;
        case SUB_M: printf("SUB    M"); break;
        case SUB_A: printf("SUB    A"); break;

        case SBB_B: printf("SBB    B"); break;
        case SBB_C: printf("SBB    C"); break;
        case SBB_D: printf("SBB    D"); break;
        case SBB_E: printf("SBB    E"); break;
        case SBB_H: printf("SBB    H"); break;
        case SBB_L: printf("SBB    L"); break;
        case SBB_M: printf("SBB    M"); break;
        case SBB_A: printf("SBB    A"); break;

        case ANA_B: printf("ANA    B"); break;
        case ANA_C: printf("ANA    C"); break;
        case ANA_D: printf("ANA    D"); break;
        case ANA_E: printf("ANA    E"); break;
        case ANA_H: printf("ANA    H"); break;
        case ANA_L: printf("ANA    L"); break;
        case ANA_M: printf("ANA    M"); break;
        case ANA_A: printf("ANA    A"); break;

        case XRA_B: printf("XRA    B"); break;
        case XRA_C: printf("XRA    C"); break;
        case XRA_D: printf("XRA    D"); break;
        case XRA_E: printf("XRA    E"); break;
        case XRA_H: printf("XRA    H"); break;
        case XRA_L: printf("XRA    L"); break;
        case XRA_M: printf("XRA    M"); break;
        case XRA_A: printf("XRA    A"); break;

        case ORA_B: printf("ORA    B"); break;
        case ORA_C: printf("ORA    C"); break;
        case ORA_D: printf("ORA    D"); break;
        case ORA_E: printf("ORA    E"); break;
        case ORA_H: printf("ORA    H"); break;
        case ORA_L: printf("ORA    L"); break;
        case ORA_M: printf("ORA    M"); break;
        case ORA_A: printf("ORA    A"); break;

        case CMP_B: printf("CMP    B"); break;
        case CMP_C: printf("CMP    C"); break;
        case CMP_D: printf("CMP    D"); break;
        case CMP_E: printf("CMP    E"); break;
        case CMP_H: printf("CMP    H"); break;
        case CMP_L: printf("CMP    L"); break;
        case CMP_M: printf("CMP    M"); break;
        case CMP_A: printf("CMP    A"); break;

        case RNZ: printf("RNZ"); break;
        case POP_B: printf("POP    B"); break;
        case JNZ: printf("JNZ    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case JMP: printf("JMP    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case CNZ: printf("CNZ    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case PUSH_B: printf("PUSH   B"); break;
        case ADI: printf("ADI    #%02x", code[1]); opbytes = 2; break;
        case RST_0: printf("RST    0"); break;
        case RZ: printf("RZ"); break;
        case RET: printf("RET"); break;
        case JZ: printf("JZ     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        // gap
        case CZ: printf("CZ     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case CALL: printf("CALL   #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case ACI: printf("ACI    #%02x", code[1]); opbytes = 2; break;
        case RST_1: printf("RST    1"); break;
        case RNC: printf("RNC"); break;
        case POP_D: printf("POP    D"); break;
        case JNC: printf("JNC    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case OUT: printf("OUT    #%02x", code[1]); opbytes = 2; break;
        case CNC: printf("CNC    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case PUSH_D: printf("PUSH   D"); break;
        case SUI: printf("SUI    #%02x", code[1]); opbytes = 2; break;
        case RST_2: printf("RST    2"); break;
        case RC: printf("RC"); break;
        //gap
        case JC: printf("JC     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case IN: printf("IN     #%02x", code[1]); opbytes = 2; break;
        case CC: printf("CC     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        //gap
        case SBI: printf("SBI    #%02x", code[1]); opbytes = 2; break;
        case RST_3: printf("RST    3"); break;
        case RPO: printf("RPO"); break;
        case POP_H: printf("POP    H"); break;
        case JPO: printf("JPO    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case XTHL: printf("XTHL"); break;
        case CPO: printf("CPO    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case PUSH_H: printf("PUSH   H"); break;
        case ANI: printf("ANI    #%02x", code[1]); opbytes = 2; break;
        case RST_4: printf("RST    4"); break;
        case RPE: printf("RPE"); break;
        case PCHL: printf("PCHL"); break;
        case JPE: printf("JPE    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case XCHG: printf("XCHG"); break;
        case CPE: printf("CPE    #%02x%02x", code[2], code[1]); opbytes = 3; break;
        //gap
        case XRI: printf("XRI    #%02x", code[1]); opbytes = 2; break;
        case RST_5: printf("RST    5"); break;
        case RP: printf("RP"); break;
        case POP_PSW: printf("POP    PSW"); break;
        case JP: printf("JP     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case DI: printf("DI"); break;
        case CP: printf("CP     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case PUSH_PSW: printf("PUSH   PSW"); break;
        case ORI: printf("ORI    #%02x", code[1]); opbytes = 2; break;
        case RST_6: printf("RST    6"); break;
        case RM: printf("RM"); break;
        case SPHL: printf("SPHL"); break;
        case JM: printf("JM     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        case EI: printf("EI"); break;
        case CM: printf("CM     #%02x%02x", code[2], code[1]); opbytes = 3; break;
        //gap
        case CPI: printf("CPI    #%02x", code[1]); opbytes = 2; break;
        case RST_7: printf("RST    7"); break;
        default:
            printf("Error: unhandled opcode");
            break;
    }
    printf("\t %lld\n", counter);
    

    return opbytes;
}