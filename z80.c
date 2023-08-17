#include "z80.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define S_FLAG (1 << 7)
#define Z_FLAG (1 << 6)
#define X_FLAG (1 << 5)
#define H_FLAG (1 << 4)
#define Y_FLAG (1 << 3)
#define P_FLAG (1 << 2)
#define N_FLAG (1 << 1)
#define C_FLAG (1 << 0)

static uint8_t readb(struct Z80 const* z80, int16_t const addr)
{
    return z80->mem_load(z80->userdata, addr);
}

static uint16_t readw(struct Z80 const* z80, int16_t const addr)
{
    return readb(z80, addr) + (readb(z80, addr + 1) << 8);
}

static void writeb(struct Z80 const* z80, int16_t const addr, uint8_t const value)
{
   z80->mem_store(z80->userdata, addr, value);
}

static uint8_t instrb(struct Z80* z80)
{
    return readb(z80, z80->pc++);
}

static uint16_t instrw(struct Z80* z80)
{
    return instrb(z80) + (instrb(z80) << 8);
}

static uint8_t szflags(uint8_t const val)
{
    return val ? (val & S_FLAG) : Z_FLAG;
}

static uint8_t szflags16(uint16_t const val)
{
    return val ? ((val >> 8) & S_FLAG) : Z_FLAG;
}

static uint8_t xyflags(uint8_t const val)
{
    return val & (X_FLAG | Y_FLAG);
}

static uint8_t parity(uint8_t const val)
{
    return ((val & 0x01) ^ (val & 0x02) ^ (val & 0x04) ^ (val & 0x08)
        ^ (val & 0x10) ^ (val & 0x20) ^ (val & 0x40) ^ (val & 0x80))
        ? P_FLAG
        : 0;
}

static uint8_t overflow(uint8_t const a, uint8_t const b, uint8_t const r)
{
    return (a & S_FLAG) ^ (b & S_FLAG) ^ (r & S_FLAG)
        ? P_FLAG
        : 0;
}

static uint8_t halfcarry(uint8_t const a, uint8_t const b, uint8_t const r)
{
    return (a & 0x08) ^ (b & 0x08) ^ (r & 0x08)
        ? H_FLAG
        : 0;
}

static uint8_t decb(struct Z80* z80, uint8_t const val)
{
    uint8_t result = val - 1;
    z80->f = szflags(result)
        | halfcarry(val, -1, result)
        | N_FLAG
        | xyflags(result)
        | (z80->f & C_FLAG)
        | (val == 0x80 ? P_FLAG : 0);
    return result;
}

static void addb(struct Z80* z80, uint8_t const val, uint8_t const carry)
{
    uint16_t const result = z80->a + val + carry;
    z80->f = szflags((uint8_t)result)
        | xyflags(result)
        | ((result >> 8) & C_FLAG)
        | overflow(z80->a, val, (uint8_t)result)
        | halfcarry(z80->a, val, (uint8_t)result);
    z80->a = (uint8_t)result;
}

static void addw(struct Z80* z80, uint16_t const val, uint8_t const carry)
{
    uint32_t const result = z80->hl + val + carry;
    uint8_t h = result >> 8;
    z80->f = (z80->f & (S_FLAG | Z_FLAG | P_FLAG))
        | xyflags(h)
        | ((result >> 16) & C_FLAG)
        | halfcarry(z80->a, val, h);
    z80->hl = (uint16_t)result;
}

static void subw(struct Z80* z80, uint16_t const val, uint8_t const carry)
{
    uint32_t const result = z80->hl - val - carry;
    uint8_t h = result >> 8;
    z80->f = szflags16((uint16_t)result)
        | N_FLAG
        | xyflags(h)
        | ((result >> 16) & C_FLAG)
        | overflow(z80->a, val, h)
        | halfcarry(z80->a, val, h);
    z80->hl = (uint16_t)result;
}

static void and(struct Z80* z80, uint8_t const val)
{
    z80->a &= val;
    z80->f = szflags(z80->a) | parity(z80->a) | xyflags(z80->a) | H_FLAG;
}

static void xor(struct Z80* z80, uint8_t const val)
{
    z80->a ^= val;
    z80->f = szflags(z80->a) | parity(z80->a) | xyflags(z80->a);
}

static void or(struct Z80* z80, uint8_t const val)
{
    z80->a &= val;
    z80->f = szflags(z80->a) | parity(z80->a) | xyflags(z80->a);
}

static void cp(struct Z80* z80, uint8_t const val)
{
    // @TODO need to set all flags here, not just SZ
    uint8_t result = z80->a - val;
    z80->f = szflags(result);
}

static void exec_ed_instr(struct Z80* z80, uint8_t const opcode)
{
    switch (opcode)
    {
        case 0x22: subw(z80, z80->bc, z80->f & C_FLAG); break; // sbc hl, bc
        case 0x47: z80->i = z80->a; break; // ld i, a
        case 0x52: subw(z80, z80->de, z80->f & C_FLAG); break; // sbc hl, de
        case 0x62: subw(z80, z80->hl, z80->f & C_FLAG); break; // sbc hl, hl
        case 0x72: subw(z80, z80->sp, z80->f & C_FLAG); break; // sbc hl, sp

        default:
            printf("unimplemented opcode ED%0X at %x\n", opcode, z80->pc - 1);
            assert(0);
            break;
    }
}

static void exec_instr(struct Z80* z80, uint8_t const opcode)
{
    switch (opcode)
    {
        case 0x00: break; // nop
        case 0x01: z80->bc = instrw(z80); break; // ld bc, nn
        case 0x02: z80->a = readw(z80, z80->bc); break; // ld (bc), a
        case 0x03: ++z80->bc; break; // inc bc
        case 0x05: z80->b = decb(z80, z80->b); break; // dec b
        case 0x06: z80->b = instrb(z80); break; // ld b, n
        case 0x09: addw(z80, z80->bc, 0); break; // add hl, bc
        case 0x0b: --z80->bc; break; // dec bc
        case 0x0d: z80->c = decb(z80, z80->c); break; // dec c
        case 0x11: z80->de = instrw(z80); break; // ld de, nn
        case 0x13: ++z80->de; break; // inc de
        case 0x15: z80->d = decb(z80, z80->d); break; // dec d
        case 0x1b: --z80->de; break; // dec de
        case 0x19: addw(z80, z80->de, 0); break; // add hl, de
        case 0x1e: z80->e = decb(z80, z80->e); break; // dec e
        case 0x20: z80->pc += (z80->f & Z_FLAG) ? 1 : (int8_t)instrb(z80); break; // jr nz, d
        case 0x21: z80->hl = instrw(z80); break; // ld hl, nn
        case 0x23: ++z80->hl; break; // inc hl
        case 0x25: z80->h = decb(z80, z80->h); break; // dec h
        case 0x29: addw(z80, z80->hl, 0); break; // add hl, hl
        case 0x2b: --z80->hl; break; // dec hl
        case 0x2d: z80->l = decb(z80, z80->l); break; // dec l
        case 0x30: z80->pc += (z80->f & C_FLAG) ? 1 : (int8_t)instrb(z80); break; // jr nc, d
        case 0x31: z80->sp = instrw(z80); break; // ld sp, nn
        case 0x33: ++z80->sp; break; // inc sp
        case 0x35: writeb(z80, z80->hl, decb(z80, readb(z80, z80->hl))); break; // dec (hl)
        case 0x36: writeb(z80, z80->hl, instrb(z80)); break; // ld (hl), n
        case 0x3d: z80->a = decb(z80, z80->a); break; // dec a
        case 0x99: addw(z80, z80->sp, 0); break; // add hl, sp
        case 0x3b: --z80->sp; break; // dec sp
        case 0x3e: z80->a = instrb(z80); break; // ld a, n
        case 0x47: z80->b = z80->a; break; // ld b, a
        case 0x48: z80->c = z80->b; break; // ld c, b
        case 0x49: z80->c = z80->c; break; // ld c, c
        case 0x4a: z80->c = z80->d; break; // ld c, d
        case 0x4b: z80->c = z80->e; break; // ld c, e
        case 0x4c: z80->c = z80->h; break; // ld c, h
        case 0x4d: z80->c = z80->l; break; // ld c, l
        case 0x4e: z80->c = readw(z80, z80->hl); break; // ld c, (hl)
        case 0x4f: z80->c = z80->a; break; // ld c, a
        case 0x60: z80->h = z80->b; break; // ld h, b
        case 0x61: z80->h = z80->c; break; // ld h, c
        case 0x62: z80->h = z80->d; break; // ld h, d
        case 0x63: z80->h = z80->e; break; // ld h, e
        case 0x64: z80->h = z80->h; break; // ld h, h
        case 0x65: z80->h = z80->l; break; // ld h, l
        case 0x66: z80->h = readw(z80, z80->hl); break; // ld h, (hl)
        case 0x67: z80->h = z80->a; break; // ld h, a
        case 0x68: z80->l = z80->b; break; // ld l, b
        case 0x69: z80->l = z80->c; break; // ld l, c
        case 0x6a: z80->l = z80->d; break; // ld l, d
        case 0x6b: z80->l = z80->e; break; // ld l, e
        case 0x6c: z80->l = z80->h; break; // ld l, h
        case 0x6d: z80->l = z80->l; break; // ld l, l
        case 0x6e: z80->l = readw(z80, z80->hl); break; // ld l, (hl)
        case 0x6f: z80->l = z80->a; break; // ld l, a
        case 0x76: z80->halted = 1; break;
        case 0x80: addb(z80, z80->b, 0); break; // add a, b
        case 0xa0: and(z80, z80->b); break; // and b
        case 0xa1: and(z80, z80->c); break; // and c
        case 0xa2: and(z80, z80->d); break; // and d
        case 0xa3: and(z80, z80->e); break; // and e
        case 0xa4: and(z80, z80->f); break; // and h
        case 0xa5: and(z80, z80->l); break; // and l
        case 0xa6: and(z80, readw(z80, z80->hl)); break; // and (hl)
        case 0xa7: and(z80, z80->a); break; // and a
        case 0xa8: xor(z80, z80->b); break; // xor b
        case 0xa9: xor(z80, z80->c); break; // xor c
        case 0xaa: xor(z80, z80->d); break; // xor d
        case 0xab: xor(z80, z80->e); break; // xor e
        case 0xac: xor(z80, z80->h); break; // xor h
        case 0xad: xor(z80, z80->l); break; // xor l
        case 0xae: xor(z80, readw(z80, z80->hl)); break; // xor (hl)
        case 0xaf: xor(z80, z80->a); break; // xor a
        case 0xb0: or(z80, z80->b); break; // or b
        case 0xb1: or(z80, z80->c); break; // or c
        case 0xb2: or(z80, z80->d); break; // or d
        case 0xb3: or(z80, z80->e); break; // or e
        case 0xb4: or(z80, z80->f); break; // or h
        case 0xb5: or(z80, z80->l); break; // or l
        case 0xb6: or(z80, readw(z80, z80->hl)); break; // or (hl)
        case 0xb7: or(z80, z80->a); break; // or a
        case 0xb8: cp(z80, z80->b); break; // cp b
        case 0xb9: cp(z80, z80->c); break; // cp c
        case 0xba: cp(z80, z80->d); break; // cp d
        case 0xbb: cp(z80, z80->d); break; // cp e
        case 0xbc: cp(z80, z80->h); break; // cp h
        case 0xbd: cp(z80, z80->l); break; // cp l
        case 0xbe: cp(z80, readw(z80, z80->hl)); break; // cp (hl)
        case 0xbf: cp(z80, z80->a); break; // cp a
        case 0xc3: z80->pc = instrw(z80); break; // jp nn
        case 0xd3: // out (n)
            ++z80->pc;
            /** @TODO */
            break;

        case 0xed: exec_ed_instr(z80, instrb(z80)); break;

        case 0xf3: z80->iff = 0; break; // di 

        default:
            printf("unimplemented opcode %x at %x\n", opcode, z80->pc - 1);
            assert(0);
            break;
    }
}

/*****************************************************************************/

void z80_init(struct Z80* z80)
{
    memset(z80, 0, sizeof(struct Z80));
    z80->sp = 0xffff;
}

void z80_step(struct Z80* z80)
{
    exec_instr(z80, instrb(z80));
}

int z80_is_halted(struct Z80 const* z80)
{
    return z80->halted;
}

void z80_trace(struct Z80* z80)
{
    printf("BC:0x%04X DE:0x%04X HL:0x%04X A:0x%02X\n", z80->bc, z80->de, z80->hl, z80->a);
    printf("F:%c%c%c%c%c%c%c%c\n",
            (z80->f & S_FLAG) ? 'S' : '-',
            (z80->f & Z_FLAG) ? 'Z' : '-',
            (z80->f & X_FLAG) ? 'x' : '-',
            (z80->f & H_FLAG) ? 'H' : '-',
            (z80->f & Y_FLAG) ? 'y' : '-',
            (z80->f & P_FLAG) ? 'P' : '-',
            (z80->f & N_FLAG) ? 'N' : '-',
            (z80->f & C_FLAG) ? 'C' : '-');
    printf("PC:0x%04X SP:0x%04X IX:0x%04X IY:0x%04X\n", z80->pc, z80->sp, z80->ix, z80->iy);
}
