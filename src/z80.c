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

static uint8_t readb(struct Z80* z80, int16_t const addr)
{
    return z80->mem_load(z80, addr);
}

static uint16_t readw(struct Z80* z80, int16_t const addr)
{
    return readb(z80, addr) + (readb(z80, addr + 1) << 8);
}

static void writeb(struct Z80* z80, int16_t const addr, uint8_t const value)
{
   z80->mem_store(z80, addr, value);
}

static void writew(struct Z80* z80, int16_t const addr, uint16_t const value)
{
    writeb(z80, addr, value & 0xff);
    writeb(z80, addr + 1, value >> 8);
}

static uint8_t instrb(struct Z80* z80)
{
    uint8_t b =  readb(z80, z80->pc++);
    // printf("0x%02x ", b);
    return b;
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
        ? 0
        : P_FLAG;
}

static uint8_t overflow(uint8_t const a, uint8_t const b, uint8_t const r)
{
    return (((a & S_FLAG) & (b & S_FLAG) & (~r & S_FLAG))
        || ((~a & S_FLAG) & (~b & S_FLAG) & (r & S_FLAG)))
        ? P_FLAG
        : 0;
}

static uint8_t halfcarry(uint8_t const a, uint8_t const b, uint8_t const r)
{
    return (((a & 0x08) & (b & 0x08) & (~r & 0x08))
        || ((~a & 0x08) & (~b & 0x08) & (r & 0x08)))
        ? H_FLAG
        : 0;
}

static uint8_t incb(struct Z80* z80, uint8_t const val)
{
    uint8_t result = val + 1;
    z80->f = szflags(result)
        | halfcarry(val, 1, result)
        | N_FLAG
        | xyflags(result)
        | (z80->f & C_FLAG)
        | (val == 0x7f ? P_FLAG : 0);
    return result;
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

static void subb(struct Z80* z80, uint8_t const val, uint8_t const carry)
{
    uint16_t const result = z80->a - val - carry;
    z80->f = szflags((uint8_t)result)
        | N_FLAG
        | xyflags(result)
        | ((result >> 8) & C_FLAG)
        | overflow(z80->a, val, (uint8_t)result)
        | halfcarry(z80->a, val, (uint8_t)result);
    z80->a = (uint8_t)result;
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
    z80->a |= val;
    z80->f = szflags(z80->a) | parity(z80->a) | xyflags(z80->a);
}

static void cp(struct Z80* z80, uint8_t const val)
{
    uint8_t a = z80->a;

    subb(z80, val, 0);
    z80->a = a;
}

static void rlca(struct Z80* z80)
{
    z80->f &= ~(H_FLAG | N_FLAG | C_FLAG);
    if (z80->a & 0x80)
        z80->f |= C_FLAG;
    z80->a = (z80->a << 1) | (z80->f & C_FLAG);
}

static void rrca(struct Z80* z80)
{
    z80->f &= ~(H_FLAG | N_FLAG | C_FLAG);
    if (z80->a & 0x01)
        z80->f |= C_FLAG;
    z80->a = (z80->a >> 1) | ((z80->a & C_FLAG) << 7);
}

static void push(struct Z80* z80, uint16_t const val)
{
    writeb(z80, --z80->sp, val >> 8);
    writeb(z80, --z80->sp, val & 0xFF);
}

static uint16_t pop(struct Z80* z80)
{
    uint8_t const l = readb(z80, z80->sp++);
    uint8_t const h = readb(z80, z80->sp++);
    return (h << 8) + l;
}

static void jp(struct Z80* z80, int test)
{
    uint16_t const addr = instrw(z80);
    if (test)
        z80->pc = addr;
}

static void jr(struct Z80* z80, int test)
{
    int8_t const offset = (int8_t)instrb(z80);
    if (test)
        z80->pc += offset;
}

static void call(struct Z80* z80, int test)
{
    uint16_t const addr = instrw(z80);
    if (test)
    {
        push(z80, z80->pc);
        z80->pc = addr;
    }
}

static uint8_t in(struct Z80* z80, uint8_t const port)
{
    return z80->port_load(z80, port);
}

static void out(struct Z80* z80, uint8_t const port, uint8_t const val)
{
    z80->port_store(z80, port, val);
}

static void ldi(struct Z80* z80)
{
    uint8_t byte = readb(z80, z80->hl);
    writeb(z80, z80->de, byte);
    ++z80->de;
    ++z80->hl;
    --z80->bc;
    
    z80->f &= ~(X_FLAG | H_FLAG | Y_FLAG | P_FLAG | N_FLAG);
    z80->f |= xyflags(byte);
    if (z80->bc == 0)
        z80->f |= P_FLAG;
}

static void ldir(struct Z80* z80)
{
    do {
        ldi(z80);
    } while (~z80->f & P_FLAG);
}

static void exec_index_instr(struct Z80* z80, uint8_t const sel, uint8_t const opcode)
{
    uint16_t* reg = sel == 0xdd ? &z80->ix : &z80->iy;

    switch (opcode)
    {
        case 0x21: *reg = instrw(z80); break; // ld i*, nn
        case 0x23: ++(*reg); break; // inc i*
        case 0x7e: z80->a = readw(z80, *reg + (int8_t)instrb(z80)); break; // ld a, (i* + d)
        case 0xe1: *reg = pop(z80); break; // pop i*
        case 0xe5: push(z80, *reg); break; // push i*
        case 0xe9: z80->pc = *reg; break; // jp (i*)

        default:
            printf("unimplemented opcode 0x%02x%02x at %x\n", sel, opcode, z80->pc - 1);
            assert(0);
            break;
    }
}

static void exec_ed_instr(struct Z80* z80, uint8_t const opcode)
{
    switch (opcode)
    {
        case 0x42: subw(z80, z80->bc, z80->f & C_FLAG); break; // sbc hl, bc
        case 0x47: z80->i = z80->a; break; // ld i, a
        case 0x52: subw(z80, z80->de, z80->f & C_FLAG); break; // sbc hl, de
        case 0x62: subw(z80, z80->hl, z80->f & C_FLAG); break; // sbc hl, hl
        case 0x72: subw(z80, z80->sp, z80->f & C_FLAG); break; // sbc hl, sp
        case 0x73: writew(z80, instrw(z80), z80->sp); break; // ld (nn), sp
        case 0x7b: z80->sp = readw(z80, instrw(z80)); break; // ld sp, (nn)
        case 0xb0: ldir(z80); break; // ldir

        default:
            printf("unimplemented opcode 0xed%02X at %x\n", opcode, z80->pc - 1);
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
        case 0x02: writeb(z80, z80->bc, z80->a); break; // ld (bc), a
        case 0x03: ++z80->bc; break; // inc bc
        case 0x04: z80->b = incb(z80, z80->b); break; // inc b
        case 0x05: z80->b = decb(z80, z80->b); break; // dec b
        case 0x06: z80->b = instrb(z80); break; // ld b, n
        case 0x07: rlca(z80); break; // rcla
        case 0x08: { uint16_t const af = z80->af; z80->af = z80->afp; z80->afp = af; break; } // ex af, af'
        case 0x09: addw(z80, z80->bc, 0); break; // add hl, bc
        case 0x0a: z80->a = readb(z80, z80->bc); break; // ld a, (bc)
        case 0x0b: --z80->bc; break; // dec bc
        case 0x0c: z80->c = incb(z80, z80->c); break; // inc c
        case 0x0d: z80->c = decb(z80, z80->c); break; // dec c
        case 0x0e: z80->c = instrb(z80); break; // ld c, n
        case 0x0f: rrca(z80); break; // rrca
        case 0x10: --z80->b; jr(z80, z80->b); break; // djnz d
        case 0x11: z80->de = instrw(z80); break; // ld de, nn
        case 0x12: writeb(z80, z80->de, z80->a); break; // ld (de), a
        case 0x13: ++z80->de; break; // inc de
        case 0x14: z80->d = incb(z80, z80->d); break; // inc d
        case 0x15: z80->d = decb(z80, z80->d); break; // dec d
        case 0x16: z80->d = instrb(z80); break; // ld d, n
        case 0x18: jr(z80, 1); break; // jr d
        case 0x19: addw(z80, z80->de, 0); break; // add hl, de
        case 0x1a: z80->a = readb(z80, z80->de); break; // ld a, (de)
        case 0x1b: --z80->de; break; // dec de
        case 0x1c: z80->e = incb(z80, z80->e); break; // inc e
        case 0x1d: z80->e = decb(z80, z80->e); break; // dec e
        case 0x1e: z80->e = instrb(z80); break; // ld e, n
        case 0x20: jr(z80, ~z80->f & Z_FLAG); break; // jr nz, d
        case 0x21: z80->hl = instrw(z80); break; // ld hl, nn
        case 0x22: writew(z80, instrw(z80), z80->hl); break; // ld (nn), hl
        case 0x23: ++z80->hl; break; // inc hl
        case 0x24: z80->h = incb(z80, z80->h); break; // inc h
        case 0x25: z80->h = decb(z80, z80->h); break; // dec h
        case 0x26: z80->h = instrb(z80); break; // ld h, n
        case 0x28: jr(z80, z80->f & Z_FLAG); break; // jr z, d
        case 0x29: addw(z80, z80->hl, 0); break; // add hl, hl
        case 0x2a: z80->hl = readb(z80, instrw(z80)); break; // ld hl, (nn)
        case 0x2b: --z80->hl; break; // dec hl
        case 0x2c: z80->l = incb(z80, z80->l); break; // inc l
        case 0x2d: z80->l = decb(z80, z80->l); break; // dec l
        case 0x2e: z80->l = instrb(z80); break; // ld l, n
        case 0x30: jr(z80, ~z80->f & C_FLAG); break; // jr nc, d
        case 0x31: z80->sp = instrw(z80); break; // ld sp, nn
        case 0x32: writeb(z80, instrw(z80), z80->a); break; // ld (nn), a
        case 0x33: ++z80->sp; break; // inc sp
        case 0x34: writeb(z80, z80->hl, incb(z80, readb(z80, z80->hl))); break; // inc (hl)
        case 0x35: writeb(z80, z80->hl, decb(z80, readb(z80, z80->hl))); break; // dec (hl)
        case 0x36: writeb(z80, z80->hl, instrb(z80)); break; // ld (hl), n
        case 0x38: jr(z80, z80->f & C_FLAG); break; // jr c, d
        case 0x3a: z80->a = readb(z80, instrw(z80)); break; // ld a, (nn)
        case 0x3c: z80->a = incb(z80, z80->a); break; // inc a
        case 0x3d: z80->a = decb(z80, z80->a); break; // dec a
        case 0x39: addw(z80, z80->sp, 0); break; // add hl, sp
        case 0x3b: --z80->sp; break; // dec sp
        case 0x3e: z80->a = instrb(z80); break; // ld a, n
        case 0x40: z80->b = z80->b; break; // ld b, b
        case 0x41: z80->b = z80->c; break; // ld b, c
        case 0x42: z80->b = z80->d; break; // ld b, d
        case 0x43: z80->b = z80->e; break; // ld b, e
        case 0x44: z80->b = z80->h; break; // ld b, h
        case 0x45: z80->b = z80->l; break; // ld b, l
        case 0x46: z80->b = readw(z80, z80->hl); break; // ld b, (hl)
        case 0x47: z80->b = z80->a; break; // ld b, a
        case 0x48: z80->c = z80->b; break; // ld c, b
        case 0x49: z80->c = z80->c; break; // ld c, c
        case 0x4a: z80->c = z80->d; break; // ld c, d
        case 0x4b: z80->c = z80->e; break; // ld c, e
        case 0x4c: z80->c = z80->h; break; // ld c, h
        case 0x4d: z80->c = z80->l; break; // ld c, l
        case 0x4e: z80->c = readw(z80, z80->hl); break; // ld c, (hl)
        case 0x4f: z80->c = z80->a; break; // ld c, a
        case 0x50: z80->d = z80->b; break; // ld d, b
        case 0x51: z80->d = z80->c; break; // ld d, c
        case 0x52: z80->d = z80->d; break; // ld d, d
        case 0x53: z80->d = z80->e; break; // ld d, e
        case 0x54: z80->d = z80->h; break; // ld d, h
        case 0x55: z80->d = z80->l; break; // ld d, l
        case 0x56: z80->d = readw(z80, z80->hl); break; // ld d, (hl)
        case 0x57: z80->d = z80->a; break; // ld d, a
        case 0x58: z80->e = z80->b; break; // ld e, b
        case 0x59: z80->e = z80->c; break; // ld e, c
        case 0x5a: z80->e = z80->d; break; // ld e, d
        case 0x5b: z80->e = z80->e; break; // ld e, e
        case 0x5c: z80->e = z80->h; break; // ld e, h
        case 0x5d: z80->e = z80->l; break; // ld e, l
        case 0x5e: z80->e = readw(z80, z80->hl); break; // ld e, (hl)
        case 0x5f: z80->e = z80->a; break; // ld e, a
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
        case 0x70: writeb(z80, z80->hl, z80->b); break; // ld (hl), b
        case 0x71: writeb(z80, z80->hl, z80->c); break; // ld (hl), c
        case 0x72: writeb(z80, z80->hl, z80->d); break; // ld (hl), d
        case 0x73: writeb(z80, z80->hl, z80->e); break; // ld (hl), e
        case 0x74: writeb(z80, z80->hl, z80->h); break; // ld (hl), h
        case 0x75: writeb(z80, z80->hl, z80->l); break; // ld (hl), l
        case 0x76: z80->halted = 1; break;
        case 0x77: writeb(z80, z80->hl, z80->a); break; // ld (hl), a
        case 0x78: z80->a = z80->b; break; // ld a, b
        case 0x79: z80->a = z80->c; break; // ld a, c
        case 0x7a: z80->a = z80->d; break; // ld a, d
        case 0x7b: z80->a = z80->e; break; // ld a, e
        case 0x7c: z80->a = z80->h; break; // ld a, h
        case 0x7d: z80->a = z80->l; break; // ld a, l
        case 0x7e: z80->a = readb(z80, z80->hl); break; // ld a, (hl)
        case 0x7f: z80->a = z80->a; break; // ld a, a
        case 0x80: addb(z80, z80->b, 0); break; // add a, b
        case 0xa0: and(z80, z80->b); break; // and b
        case 0xa1: and(z80, z80->c); break; // and c
        case 0xa2: and(z80, z80->d); break; // and d
        case 0xa3: and(z80, z80->e); break; // and e
        case 0xa4: and(z80, z80->h); break; // and h
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
        case 0xb4: or(z80, z80->h); break; // or h
        case 0xb5: or(z80, z80->l); break; // or l
        case 0xb6: or(z80, readw(z80, z80->hl)); break; // or (hl)
        case 0xb7: or(z80, z80->a); break; // or a
        case 0xb8: cp(z80, z80->b); break; // cp b
        case 0xb9: cp(z80, z80->c); break; // cp c
        case 0xba: cp(z80, z80->d); break; // cp d
        case 0xbb: cp(z80, z80->e); break; // cp e
        case 0xbc: cp(z80, z80->h); break; // cp h
        case 0xbd: cp(z80, z80->l); break; // cp l
        case 0xbe: cp(z80, readw(z80, z80->hl)); break; // cp (hl)
        case 0xbf: cp(z80, z80->a); break; // cp a
        case 0xc0: if (~z80->f & Z_FLAG) z80->pc = pop(z80); break; // ret nz
        case 0xc1: z80->bc = pop(z80); break; // pop bc
        case 0xc2: jp(z80, ~z80->f & Z_FLAG); break;  // jp nz, nn
        case 0xc3: z80->pc = instrw(z80); break; // jp nn
        case 0xc4: call(z80, ~z80->f & Z_FLAG); break; // call nz, nn
        case 0xc5: push(z80, z80->bc); break; // push bc
        case 0xc6: addb(z80, instrb(z80), 0); break; // add a, n
        case 0xc8: if (z80->f & Z_FLAG) z80->pc = pop(z80); break; // ret z
        case 0xc9: z80->pc = pop(z80); break; // ret
        case 0xca: jp(z80, z80->f & Z_FLAG); break;  // jp z, nn
        case 0xcc: call(z80, z80->f & Z_FLAG); break; // call z, nn
        case 0xcd: call(z80, 1); break; // call nn
        case 0xd0: if (~z80->f & C_FLAG) z80->pc = pop(z80); break; // ret nc
        case 0xd1: z80->de = pop(z80); break; // pop de
        case 0xd2: jp(z80, ~z80->f & C_FLAG); break;  // jp nc, nn
        case 0xd3: out(z80, instrb(z80), z80->a); break; // out (n), a
        case 0xd4: call(z80, ~z80->f & C_FLAG); break; // call nc, nn
        case 0xd5: push(z80, z80->de); break; // push de
        case 0xd8: if (z80->f & C_FLAG) z80->pc = pop(z80); break; // ret c
        case 0xd9: {
            uint16_t const bc = z80->bc;
            uint16_t const de = z80->de;
            uint16_t const hl = z80->hl;
            z80->bc = z80->bcp;
            z80->de = z80->dep;
            z80->hl = z80->hlp;
            z80->bcp = bc;
            z80->dep = de;
            z80->hlp = hl;
            break;
        }
        case 0xda: jp(z80, z80->f & C_FLAG); break;  // jp c, nn
        case 0xdb: z80->a = in(z80, instrb(z80)); break; // in a, (n)
        case 0xdc: call(z80, z80->f & C_FLAG); break; // call c, nn
        case 0xe0: if (~z80->f & P_FLAG) z80->pc = pop(z80); break; // ret po
        case 0xe1: z80->hl = pop(z80); break; // pop hl
        case 0xe2: jp(z80, ~z80->f & P_FLAG); break;  // jp po, nn
        case 0xe4: call(z80, ~z80->f & P_FLAG); break; // call po, nn
        case 0xe5: push(z80, z80->hl); break; // push hl
        case 0xe6: and(z80, instrb(z80)); break; // and n
        case 0xe8: if (z80->f & P_FLAG) z80->pc = pop(z80); break; // ret pe
        case 0xe9: z80->pc = z80->hl; break; // jp (hl)
        case 0xea: jp(z80, z80->f & P_FLAG); break;  // jp pe, nn
        case 0xeb: { uint16_t de = z80->de; z80->de = z80->hl; z80->hl = de; break; } // ex de, hl
        case 0xec: call(z80, z80->f & P_FLAG); break; // call p, nn
        case 0xed: exec_ed_instr(z80, instrb(z80)); break;
        case 0xf0: if (~z80->f & S_FLAG) z80->pc = pop(z80); break; // ret p
        case 0xf1: z80->af = pop(z80); break; // pop af
        case 0xf2: jp(z80, ~z80->f & S_FLAG); break;  // jp p, nn
        case 0xf3: z80->iff = 0; break; // di 
        case 0xf4: call(z80, ~z80->f & S_FLAG); break; // call p, nn
        case 0xf5: push(z80, z80->af); break; // push af
        case 0xf8: if (z80->f & S_FLAG) z80->pc = pop(z80); break; // ret m
        case 0xf9: z80->sp = z80->hl; break; // ld sp, hl
        case 0xfa: jp(z80, z80->f & S_FLAG); break;  // jp m, nn
        case 0xfb: z80->iff = 1; break; // ei
        case 0xfc: call(z80, z80->f & S_FLAG); break; // call m, nn
        case 0xfe: cp(z80, instrb(z80)); break; // cp n

        case 0xc7:
        case 0xcf:
        case 0xd7:
        case 0xdf:
        case 0xe7:
        case 0xef:
        case 0xf7:
        case 0xff:
           push(z80, z80->pc);
           z80->pc = 0x08 * ((opcode & 0x38) >> 8);
           break;

        case 0xdd:
        case 0xfd:
           exec_index_instr(z80, opcode, instrb(z80)); break; 

        default:
            printf("unimplemented opcode 0x%02x at 0x%04x\n", opcode, z80->pc - 1);
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
