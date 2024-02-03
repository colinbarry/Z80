#include "z80/z80.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define S_FLAG (1 << 7)
#define Z_FLAG (1 << 6)
#define X_FLAG (1 << 5)
#define H_FLAG (1 << 4)
#define Y_FLAG (1 << 3)
#define P_FLAG (1 << 2)
#define N_FLAG (1 << 1)
#define C_FLAG (1 << 0)

/* clang-format off */

static const uint8_t opcode_cycles[256]
    = {4, 10, 7, 6, 4, 4, 7, 4, 4, 11, 7, 6, 4, 4, 7, 4,
       8, 10, 7, 6, 4, 4, 7, 4, 12, 11, 7, 6, 4, 4, 7, 4,
       7, 10, 16, 6, 4, 4, 7, 4, 7, 11, 16, 6, 4, 4, 7, 4,
       7, 10, 16, 6, 11, 11, 10, 4, 7, 11, 13, 6, 4, 4, 7, 4,
       4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
       4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
       4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
       7, 7, 7, 7, 7, 7, 4, 7, 4, 4, 4, 4, 4, 4, 7, 4,
       4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
       4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
       4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
       4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
       5, 10, 10, 10, 10, 11, 7, 11, 5, 10, 10, 0, 10, 17, 7, 11,
       5, 10, 10, 11, 10, 11, 7, 11, 5, 4, 10, 11, 10, 0, 7, 11,
       5, 10, 10, 19, 10, 11, 7, 11, 5, 4, 10, 4, 10, 0, 7, 11,
       5, 10, 10, 4, 10, 11, 7, 11, 5, 6, 10, 4, 10, 0, 7, 11};

static const uint8_t ed_opcode_cycles[256]
    = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
       12, 12, 15, 20, 8, 14, 8, 9, 12, 12, 15, 20, 8, 14, 8, 9,
       12, 12, 15, 20, 8, 8, 8, 9, 12, 12, 15, 20, 8, 14, 8, 9,
       12, 12, 15, 20, 8, 8, 8, 18, 12, 12, 15, 20, 8, 8, 8, 18,
       12, 12, 15, 20, 8, 8, 8, 8, 12, 12, 15, 20, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
       16, 16, 16, 16, 8, 8, 8, 8, 16, 16, 16, 16, 8, 8, 8, 8,
       16, 16, 16, 16, 8, 8, 8, 8, 16, 16, 16, 16, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
       16, 16, 16, 16, 8, 8, 8, 8, 16, 16, 16, 16, 8, 8, 8, 8,
       16, 16, 16, 16, 8, 8, 8, 8, 16, 16, 16, 16, 8, 8, 8, 8};

static const uint8_t index_opcode_cycles[256]
    = {8, 8, 8, 8, 8, 8, 11, 8, 8, 15, 8, 8, 8, 8, 11, 8,
       8, 8, 8, 8, 8, 8, 11, 8, 8, 15, 8, 8, 8, 8, 11, 8,
       8, 14, 20, 10, 8, 8, 11, 8, 8, 15, 20, 10, 8, 8, 11, 8,
       8, 8, 8, 8, 23, 23, 19, 8, 8, 15, 8, 8, 8, 8, 11, 8,
       8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
       8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
       8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
       19, 19, 19, 19, 19, 19, 8, 19, 8, 8, 8, 8, 8, 8, 19, 8,
       8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
       8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
       8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
       8, 8, 8, 8, 8, 8, 19, 8, 8, 8, 8, 8, 8, 8, 19, 8,
       8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
       8, 14, 8, 23, 8, 15, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
       8, 8, 8, 8, 8, 8, 8, 8, 8, 10, 8, 8, 8, 8, 8, 8};

/* clang-format on */

static uint8_t set(uint8_t byte, uint8_t bits)
{
    return byte | bits;
}

static uint8_t reset(uint8_t byte, uint8_t bits)
{
    return byte & ~bits;
}

static uint8_t mask(uint8_t byte, uint8_t bits, int condition)
{
    return condition ? set(byte, bits) : reset(byte, bits);
}

static uint8_t readb(struct Z80* z80, uint16_t const addr)
{
    return z80->mem_load(z80, addr);
}

static uint16_t readw(struct Z80* z80, uint16_t const addr)
{
    return readb(z80, addr) + (readb(z80, addr + 1) << 8);
}

static void writeb(struct Z80* z80, uint16_t const addr, uint8_t const value)
{
    z80->mem_store(z80, addr, value);
}

static void writew(struct Z80* z80, uint16_t const addr, uint16_t const value)
{
    writeb(z80, addr, value & 0xff);
    writeb(z80, addr + 1, value >> 8);
}

static uint8_t instrb(struct Z80* z80)
{
    return readb(z80, z80->pc++);
}

/** Reads a displacement byte from the current instruction.
 * */
static int8_t dispb(struct Z80* z80)
{
    return (int8_t)readb(z80, z80->pc++);
}

static uint16_t instrw(struct Z80* z80)
{
    return instrb(z80) + (instrb(z80) << 8);
}

static uint8_t szflags(uint8_t const val)
{
    return val ? (val & S_FLAG) : Z_FLAG;
}

static uint8_t xyflags(uint8_t const val)
{
    return val & (X_FLAG | Y_FLAG);
}

static uint8_t parity(uint8_t const val)
{
    return ((val & 0x01) ^ ((val & 0x02) >> 1) ^ ((val & 0x04) >> 2)
            ^ ((val & 0x08) >> 3) ^ ((val & 0x10) >> 4) ^ ((val & 0x20) >> 5)
            ^ ((val & 0x40) >> 6) ^ ((val & 0x80) >> 7))
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

static uint8_t halfcarry(uint8_t const a, uint8_t const b, uint8_t c)
{
    return ((a & 0x0f) + (b & 0x0f) + c) & 0x10 ? H_FLAG : 0;
}

static uint8_t addb(struct Z80* z80, uint8_t arg1, uint8_t const arg2, uint8_t const carry)
{
    uint16_t const result = arg1 + arg2 + carry;
    z80->f = szflags((uint8_t)result) | xyflags(result)
             | ((result >> 8) & C_FLAG) | overflow(arg1, arg2, (uint8_t)result)
             | halfcarry(arg1, arg2, carry);
    return (uint8_t)result;
}

static uint16_t addw(struct Z80* z80, uint16_t const arg1, uint16_t const arg2)
{
    uint8_t flags = z80->f;
    uint8_t l = addb(z80, arg1 & 0xFF, arg2 & 0xFF, 0);
    uint8_t h = addb(z80, arg1 >> 8, arg2 >> 8, z80->f & C_FLAG);

    z80->f &= ~(S_FLAG | Z_FLAG | P_FLAG | N_FLAG);
    z80->f |= (flags & (Z_FLAG | S_FLAG | P_FLAG));

    return (h << 8) + l;
}

static uint16_t addcw(struct Z80* z80,
                      uint16_t const arg1,
                      uint16_t const arg2,
                      uint8_t const carry)
{
    uint8_t l = addb(z80, arg1 & 0xFF, arg2 & 0xFF, carry);
    uint8_t h = addb(z80, arg1 >> 8, arg2 >> 8, z80->f & C_FLAG);

    uint16_t result = (h << 8) + l;
    if (result == 0)
    {
        z80->f |= Z_FLAG;
    }
    else
    {
        z80->f &= ~Z_FLAG;
    }

    return result;
}

static uint8_t subb(struct Z80* z80, uint8_t arg1, uint8_t const arg2, uint8_t const carry)
{
    uint16_t const result = addb(z80, arg1, ~arg2, !carry);
    z80->f ^= (C_FLAG | H_FLAG);
    z80->f |= N_FLAG;
    return (uint8_t)result;
}

static uint16_t subcw(struct Z80* z80,
                      uint16_t const arg1,
                      uint16_t const arg2,
                      uint8_t const carry)
{
    uint16_t result = addcw(z80, arg1, ~arg2, !carry);
    z80->f ^= (C_FLAG | H_FLAG);
    z80->f |= N_FLAG;
    return result;
}

static uint8_t incb(struct Z80* z80, uint8_t const val)
{
    uint8_t result = val + 1;
    z80->f = szflags(result) | halfcarry(val, 1, 0) | xyflags(result)
             | (z80->f & C_FLAG) | (val == 0x7f ? P_FLAG : 0);
    return result;
}

static uint8_t decb(struct Z80* z80, uint8_t const val)
{
    uint8_t result = val - 1;
    z80->f = szflags(result) | (halfcarry(val, -1, 0) ^ H_FLAG) | N_FLAG
             | xyflags(result) | (z80->f & C_FLAG) | (val == 0x80 ? P_FLAG : 0);
    return result;
}

static void and (struct Z80 * z80, uint8_t const val)
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
    subb(z80, z80->a, val, 0);
    z80->f &= ~(X_FLAG | Y_FLAG);
    z80->f |= xyflags(val);
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
    {
        z80->pc += offset;
        z80->cycles += 5;
    }
}

static void call(struct Z80* z80)
{
    uint16_t const addr = instrw(z80);
    push(z80, z80->pc);
    z80->pc = addr;
}

static void callc(struct Z80* z80, int test)
{
    uint16_t const addr = instrw(z80);
    if (test)
    {
        push(z80, z80->pc);
        z80->pc = addr;
        z80->cycles += 7;
    }
}

static void retc(struct Z80* z80, int test)
{
    if (test)
    {
        z80->pc = pop(z80);
        z80->cycles += 6;
    }
}

static uint8_t in(struct Z80* z80, uint16_t const port)
{
    return z80->port_load(z80, port);
}

static void out(struct Z80* z80, uint16_t const port, uint8_t const val)
{
    z80->port_store(z80, port, val);
}

static void ldd(struct Z80* z80)
{
    uint8_t const byte = readb(z80, z80->hl);
    uint8_t const result = z80->a + byte;
    writeb(z80, z80->de, byte);
    --z80->de;
    --z80->hl;
    --z80->bc;

    z80->f &= ~(X_FLAG | H_FLAG | Y_FLAG | P_FLAG | N_FLAG);
    z80->f |= ((result & 0x02) << 4) | result & 0x08;
    if (z80->bc > 0)
        z80->f |= P_FLAG;
}

static void lddr(struct Z80* z80)
{
    ldd(z80);
    if (z80->f & P_FLAG)
    {
        z80->pc -= 2;
        z80->cycles += 5;
    }
}

static void ldi(struct Z80* z80)
{
    uint8_t const byte = readb(z80, z80->hl);
    uint8_t const result = z80->a + byte;
    writeb(z80, z80->de, byte);
    ++z80->de;
    ++z80->hl;
    --z80->bc;

    z80->f &= ~(X_FLAG | H_FLAG | Y_FLAG | P_FLAG | N_FLAG);
    z80->f |= ((result & 0x02) << 4) | result & 0x08;
    if (z80->bc > 0)
        z80->f |= P_FLAG;
}

static void ldir(struct Z80* z80)
{
    ldi(z80);
    if (z80->f & P_FLAG)
    {
        z80->pc -= 2;
        z80->cycles += 5;
    }
}

static void ini(struct Z80* z80)
{
    uint8_t const byte = in(z80, z80->bc);
    uint16_t const tmp = byte + ((z80->c + 1) & 0xff);

    writeb(z80, z80->hl, byte);
    --z80->b;
    ++z80->hl;

    z80->f = parity((tmp & 0x07) ^ z80->b) | xyflags(z80->b) | szflags(z80->b);

    if (tmp & 0x100)
        z80->f |= C_FLAG | H_FLAG;

    if (z80->f & S_FLAG)
        z80->f |= N_FLAG;
}

static void inir(struct Z80* z80)
{
    ini(z80);
    if (z80->b)
    {
        z80->pc -= 2;
        z80->cycles += 5;
    }
}

static void ind(struct Z80* z80)
{
    uint8_t const byte = in(z80, z80->bc);
    uint16_t const tmp = byte + ((z80->c - 1) & 0xff);

    writeb(z80, z80->hl, byte);
    --z80->b;
    --z80->hl;

    z80->f = parity((tmp & 0x07) ^ z80->b) | xyflags(z80->b) | szflags(z80->b);

    if (tmp & 0x100)
        z80->f |= C_FLAG | H_FLAG;

    if (z80->f & S_FLAG)
        z80->f |= N_FLAG;
}

static void indr(struct Z80* z80)
{
    ind(z80);
    if (z80->b)
    {
        z80->pc -= 2;
        z80->cycles += 5;
    }
}

static void outi(struct Z80* z80)
{
    uint8_t const byte = readb(z80, z80->hl);
    out(z80, z80->bc, byte);

    --z80->b;
    ++z80->hl;
    uint16_t const tmp = byte + z80->l;

    z80->f = parity((tmp & 0x07) ^ z80->b) | xyflags(z80->b) | szflags(z80->b);

    if (tmp & 0x100)
        z80->f |= C_FLAG | H_FLAG;

    if (byte & S_FLAG)
        z80->f |= N_FLAG;
}

static void otir(struct Z80* z80)
{
    outi(z80);
    if (z80->b)
    {
        z80->pc -= 2;
        z80->cycles += 5;
    }
}

static void outd(struct Z80* z80)
{
    uint8_t const byte = readb(z80, z80->hl);
    out(z80, z80->bc, byte);

    --z80->b;
    --z80->hl;
    uint16_t const tmp = byte + z80->l;

    z80->f = parity((tmp & 0x07) ^ z80->b) | xyflags(z80->b) | szflags(z80->b);

    if (tmp & 0x100)
        z80->f |= C_FLAG | H_FLAG;

    if (byte & S_FLAG)
        z80->f |= N_FLAG;
}

static void otdr(struct Z80* z80)
{
    outd(z80);
    if (z80->b)
    {
        z80->pc -= 2;
        z80->cycles += 5;
    }
}

static void cpd(struct Z80* z80)
{
    uint8_t const flags = z80->f;
    uint8_t const val = readb(z80, z80->hl);
    uint8_t result = subb(z80, z80->a, val, 0);
    if (z80->f & H_FLAG)
        --result;

    --z80->hl;
    --z80->bc;

    z80->f &= ~(P_FLAG | C_FLAG | X_FLAG | Y_FLAG);
    z80->f |= (flags & C_FLAG);

    if (z80->bc > 0)
        z80->f |= P_FLAG;

    z80->f |= (result & Y_FLAG);
    if (result & 0x02)
        z80->f |= X_FLAG;
}

static void cpdr(struct Z80* z80)
{
    cpd(z80);
    if ((z80->f & P_FLAG) && (~z80->f & Z_FLAG))
    {
        z80->pc -= 2;
        z80->cycles += 5;
    }
}

static void cpi(struct Z80* z80)
{
    uint8_t const flags = z80->f;
    uint8_t const val = readb(z80, z80->hl);
    uint8_t result = subb(z80, z80->a, val, 0);
    if (z80->f & H_FLAG)
        --result;

    ++z80->hl;
    --z80->bc;

    z80->f &= ~(P_FLAG | C_FLAG | X_FLAG | Y_FLAG);
    z80->f |= (flags & C_FLAG);

    if (z80->bc > 0)
        z80->f |= P_FLAG;

    z80->f |= (result & Y_FLAG);
    if (result & 0x02)
        z80->f |= X_FLAG;
}

static void cpir(struct Z80* z80)
{
    cpi(z80);
    if ((z80->f & P_FLAG) && (~z80->f & Z_FLAG))
    {
        z80->pc -= 2;
        z80->cycles += 5;
    }
}

static uint8_t rlc(struct Z80* z80, uint8_t val)
{
    z80->f = 0;
    if (val & 0x80)
        z80->f |= C_FLAG;
    val = (val << 1) | (z80->f & C_FLAG);
    z80->f |= xyflags(val) | parity(val) | szflags(val);

    return val;
}

static void rlca(struct Z80* z80)
{
    const uint8_t flags = z80->f;
    z80->a = rlc(z80, z80->a);
    z80->f &= ~(S_FLAG | Z_FLAG | P_FLAG);
    z80->f |= flags & (S_FLAG | Z_FLAG | P_FLAG);
}

static uint8_t rrc(struct Z80* z80, uint8_t val)
{
    z80->f = 0;
    if (val & 0x01)
        z80->f |= C_FLAG;
    val = (val >> 1) | ((z80->f & C_FLAG) << 7);
    z80->f |= xyflags(val) | parity(val) | szflags(val);

    return val;
}

static void rrca(struct Z80* z80)
{
    const uint8_t flags = z80->f;
    z80->a = rrc(z80, z80->a);
    z80->f &= ~(S_FLAG | Z_FLAG | P_FLAG);
    z80->f |= flags & (S_FLAG | Z_FLAG | P_FLAG);
}

static uint8_t rl(struct Z80* z80, uint8_t val)
{
    uint8_t carry = (val & 0x80) ? 0x01 : 0x00;
    val = (val << 1) | (z80->f & C_FLAG);
    z80->f = xyflags(val) | parity(val) | szflags(val) | carry;

    return val;
}

static void rla(struct Z80* z80)
{
    const uint8_t flags = z80->f;
    z80->a = rl(z80, z80->a);
    z80->f &= ~(S_FLAG | Z_FLAG | P_FLAG);
    z80->f |= flags & (S_FLAG | Z_FLAG | P_FLAG);
}

static uint8_t rr(struct Z80* z80, uint8_t val)
{
    uint8_t carry = val & 0x01;
    val = (val >> 1) | ((z80->f & C_FLAG) << 7);
    z80->f = xyflags(val) | parity(val) | szflags(val) | carry;

    return val;
}

static void rra(struct Z80* z80)
{
    const uint8_t flags = z80->f;
    z80->a = rr(z80, z80->a);
    z80->f &= ~(S_FLAG | Z_FLAG | P_FLAG);
    z80->f |= flags & (S_FLAG | Z_FLAG | P_FLAG);
}

static uint8_t sla(struct Z80* z80, uint8_t val)
{
    uint8_t carry = (val & 0x80) ? 0x01 : 0x00;
    val = val << 1;
    z80->f = xyflags(val) | parity(val) | szflags(val) | carry;

    return val;
}

static uint8_t sra(struct Z80* z80, uint8_t val)
{
    uint8_t carry = val & 0x01;
    val = (val >> 1) | (val & 0x80);
    z80->f = xyflags(val) | parity(val) | szflags(val) | carry;

    return val;
}

static uint8_t sll(struct Z80* z80, uint8_t val)
{
    uint8_t carry = (val & 0x80) ? 0x01 : 0x00;
    val = (val << 1) | 0x01;
    z80->f = xyflags(val) | parity(val) | szflags(val) | carry;

    return val;
}

static uint8_t srl(struct Z80* z80, uint8_t val)
{
    uint8_t carry = val & 0x01;
    val = (val >> 1);
    z80->f = xyflags(val) | parity(val) | szflags(val) | carry;

    return val;
}

static void rrd(struct Z80* z80)
{
    uint8_t const val = readb(z80, z80->hl);
    uint8_t const memh = (val & 0xf0) >> 4;
    uint8_t const meml = val & 0xf;
    uint8_t const al = z80->a & 0xf;

    writeb(z80, z80->hl, (al << 4) | memh);
    z80->a = (z80->a & 0xf0) | meml;

    z80->f = szflags(z80->a) | xyflags(z80->a) | (z80->f & C_FLAG) | parity(z80->a);
}

static void rld(struct Z80* z80)
{
    uint8_t const val = readb(z80, z80->hl);
    uint8_t const memh = (val & 0xf0) >> 4;
    uint8_t const meml = val & 0xf;
    uint8_t const al = z80->a & 0xf;

    writeb(z80, z80->hl, (meml << 4) | al);
    z80->a = (z80->a & 0xf0) | memh;

    z80->f = szflags(z80->a) | xyflags(z80->a) | (z80->f & C_FLAG) | parity(z80->a);
}

static void daa(struct Z80* z80)
{
    uint8_t val = z80->a;
    if (z80->f & N_FLAG)
    {
        if ((z80->a & 0x0F) > 0x09 || (z80->f & H_FLAG))
        {
            val -= 0x06;
        }

        if (z80->a > 0x99 || (z80->f & C_FLAG))
        {
            val -= 0x60;
        }
    }
    else
    {
        if ((z80->a & 0x0F) > 0x09 || (z80->f & H_FLAG))
        {
            val += 0x06;
        }

        if (z80->a > 0x99 || (z80->f & C_FLAG))
        {
            val += 0x60;
        }
    }

    z80->f &= C_FLAG | N_FLAG;
    z80->f &= ~(X_FLAG | Y_FLAG);
    z80->f |= (z80->a > 0x99) ? C_FLAG : 0;
    z80->f |= szflags(val) | parity(val) | xyflags(val);
    z80->f |= (z80->a ^ val) & H_FLAG;
    z80->a = val;
}

static void cpl(struct Z80* z80)
{
    z80->a = ~z80->a;
    z80->f &= ~(X_FLAG | Y_FLAG);
    z80->f |= (H_FLAG | N_FLAG) | (z80->a & (X_FLAG | Y_FLAG));
}

static void ccf(struct Z80* z80)
{
    uint8_t prev_carry = (z80->f & C_FLAG) << 4;
    z80->f ^= C_FLAG;
    z80->f &= ~(N_FLAG | H_FLAG | X_FLAG | Y_FLAG);
    z80->f |= prev_carry | (z80->a & (X_FLAG | Y_FLAG));
}

static void scf(struct Z80* z80)
{
    z80->f &= ~(H_FLAG | N_FLAG | X_FLAG | Y_FLAG);
    z80->f |= C_FLAG | (z80->a & (X_FLAG | Y_FLAG));
}

static uint8_t inbc(struct Z80* z80)
{
    uint8_t const carry = z80->f & C_FLAG;
    uint8_t const val = in(z80, ((uint16_t)z80->b << 8) | z80->c);
    z80->f = szflags(val) | xyflags(val) | parity(val) | carry;
    return val;
}

static void incr(struct Z80* z80)
{
    z80->r = (z80->r & 0x80) | ((z80->r & 0x7F) + 1);
}

static void exec_instr(struct Z80* z80, uint8_t const opcode);

static void handle_interrupts(struct Z80* z80, uint8_t const data)
{
    z80->halted = 0;

    if (z80->iff1)
    {
        z80->iff1 = 0;

        switch (z80->interrupt_mode)
        {
            case 0: {
                z80->cycles += 11;
                exec_instr(z80, data);
                break;
            }

            case 1: {
                z80->cycles += 13;
                push(z80, z80->pc);
                z80->pc = 0x38;
                break;
            }

            case 2: {
                z80->cycles += 19;
                push(z80, z80->pc);
                z80->pc = readw(z80, ((z80->i << 8) | data) & 0xFFFE);
                break;
            }
        }
    }
}

static void exec_indexcb_instr(struct Z80* z80, uint8_t const sel)
{
    uint16_t* reg = sel == 0xdd ? &z80->ix : &z80->iy;
    uint16_t const addr = *reg + (int8_t)instrb(z80);
    uint8_t const opcode = instrb(z80);

    uint8_t val = readw(z80, addr);

    uint8_t const op = opcode >> 6;
    uint8_t const type = (opcode >> 3) & 0x7;
    uint8_t const dest = opcode & 0x07;
    switch (op)
    {
        case 0x00: // rotation
        {
            switch (type)
            {
                case 0x00: val = rlc(z80, val); break; // rlc
                case 0x01: val = rrc(z80, val); break; // rrc
                case 0x02: val = rl(z80, val); break;  // rl
                case 0x03: val = rr(z80, val); break;  // rr
                case 0x04: val = sla(z80, val); break; // sla
                case 0x05: val = sra(z80, val); break; // sra
                case 0x06: val = sll(z80, val); break; // sll
                case 0x07: val = srl(z80, val); break; // srl
            }
            break;
        }
        case 0x01: // bit
        {
            val = val & (1 << type);
            z80->f = szflags(val) | xyflags(val) | H_FLAG | (z80->f & C_FLAG);
            if (z80->f & Z_FLAG)
            {
                z80->f |= P_FLAG;
            }
            break;
        }
        case 0x02: // reset
        {
            val &= ~(1 << type);
            break;
        }
        case 0x03: // set
        {
            val |= (1 << type);
            break;
        }
    }

    if (op != 0x01)
    {
        switch (dest)
        {
            case 0x00: z80->b = val; break;
            case 0x01: z80->c = val; break;
            case 0x02: z80->d = val; break;
            case 0x03: z80->e = val; break;
            case 0x04: z80->h = val; break;
            case 0x05: z80->l = val; break;
            case 0x06: writeb(z80, addr, val); break;
            case 0x07: z80->a = val; break;
        }
    }

    z80->cycles += (op == 0x01 ? 20 : 23);
}

static void exec_index_instr(struct Z80* z80, uint8_t const sel, uint8_t const opcode)
{
    uint16_t* reg = sel == 0xdd ? &z80->ix : &z80->iy;
    uint8_t* l = (uint8_t*)reg;
    uint8_t* h = l + 1;

    incr(z80);

    switch (opcode)
    {
        case 0x09: *reg = addw(z80, *reg, z80->bc); break; // add i*, bc
        case 0x19: *reg = addw(z80, *reg, z80->de); break; // add i*, de
        case 0x21: *reg = instrw(z80); break;              // ld i*, nn
        case 0x22: writew(z80, instrw(z80), *reg); break;  // ld (nn), i*
        case 0x23: ++(*reg); break;                        // inc i*
        case 0x24: *h = incb(z80, *h); break;              // inc i*h
        case 0x25: *h = decb(z80, *h); break;              // dec i*h
        case 0x26: *h = instrb(z80); break;                // ld i*h, n
        case 0x29: *reg = addw(z80, *reg, *reg); break;    // add i*, i*
        case 0x2a: *reg = readw(z80, instrw(z80)); break;  // ld i*, (nn)
        case 0x2b: --(*reg); break;                        // dec i*
        case 0x2c: *l = incb(z80, *l); break;              // inc i*l
        case 0x2d: *l = decb(z80, *l); break;              // dec i*l
        case 0x2e: *l = instrb(z80); break;                // ld i*l, n
        case 0x34: {                                       // inc (i* + d)
            uint16_t addr = *reg + dispb(z80);
            writeb(z80, addr, incb(z80, readw(z80, addr)));
            break;
        }
        case 0x35: { // dec (i* + d)
            uint16_t addr = *reg + dispb(z80);
            writeb(z80, addr, decb(z80, readw(z80, addr)));
            break;
        }
        case 0x36: {
            int8_t d = dispb(z80);
            writeb(z80, *reg + d, instrb(z80));
            break;
        }                                                  // ld (i* + d), n
        case 0x39: *reg = addw(z80, *reg, z80->sp); break; // add i*, sp

        case 0x44: z80->b = *reg >> 8; break;   // ld b, i*h
        case 0x45: z80->b = *reg & 0xff; break; // ld b, i*l
        case 0x46:
            z80->b = readw(z80, *reg + dispb(z80));
            break;                              // ld b, (i* + d)
        case 0x4c: z80->c = *reg >> 8; break;   // ld c, i*h
        case 0x4d: z80->c = *reg & 0xff; break; // ld c, i*l
        case 0x4e:
            z80->c = readw(z80, *reg + dispb(z80));
            break;                              // ld c, (i* + d)
        case 0x54: z80->d = *reg >> 8; break;   // ld d, i*h
        case 0x55: z80->d = *reg & 0xff; break; // ld d, i*l
        case 0x56:
            z80->d = readw(z80, *reg + dispb(z80));
            break;                              // ld d, (i* + d)
        case 0x5c: z80->e = *reg >> 8; break;   // ld e, i*h
        case 0x5d: z80->e = *reg & 0xff; break; // ld e, i*l
        case 0x5e:
            z80->e = readw(z80, *reg + dispb(z80));
            break;                     // ld e, (i* + d)
        case 0x60: *h = z80->b; break; // ld i*h, b
        case 0x61: *h = z80->c; break; // ld i*h, c
        case 0x62: *h = z80->d; break; // ld i*h, d
        case 0x63: *h = z80->e; break; // ld i*h, e
        case 0x64: *h = *h; break;     // ld i*h, i*h
        case 0x65: *h = *l; break;     // ld i*h, i*l
        case 0x66:
            z80->h = readw(z80, *reg + dispb(z80));
            break;                     // ld h, (i* + d)
        case 0x67: *h = z80->a; break; // ld i*h, a
        case 0x68: *l = z80->b; break; // ld i*l, b
        case 0x69: *l = z80->c; break; // ld i*l, c
        case 0x6a: *l = z80->d; break; // ld i*l, d
        case 0x6b: *l = z80->e; break; // ld i*l, e
        case 0x6c: *l = *h; break;     // ld i*l, i*l
        case 0x6d: *l = *l; break;     // ld i*l, i*l
        case 0x6e:
            z80->l = readw(z80, *reg + dispb(z80));
            break;                     // ld l, (i* + d)
        case 0x6f: *l = z80->a; break; // ld i*l, a
        case 0x70:
            writeb(z80, *reg + dispb(z80), z80->b);
            break; // ld (i* + d), b
        case 0x71:
            writeb(z80, *reg + dispb(z80), z80->c);
            break; // ld (i* + d), c
        case 0x72:
            writeb(z80, *reg + dispb(z80), z80->d);
            break; // ld (i* + d), d
        case 0x73:
            writeb(z80, *reg + dispb(z80), z80->e);
            break; // ld (i* + d), e
        case 0x74:
            writeb(z80, *reg + dispb(z80), z80->h);
            break; // ld (i* + d), h
        case 0x75:
            writeb(z80, *reg + dispb(z80), z80->l);
            break; // ld (i* + d), l
        case 0x77:
            writeb(z80, *reg + dispb(z80), z80->a);
            break; // ld (i* + d), a
        case 0x7e:
            z80->a = readw(z80, *reg + dispb(z80));
            break;                              // ld a, (i* + d)
        case 0x7c: z80->a = *reg >> 8; break;   // ld a, (i*h)
        case 0x7d: z80->a = *reg & 0xff; break; // ld a, (i*l)
        case 0x84:
            z80->a = addb(z80, z80->a, *reg >> 8, 0);
            break;                                             // add a, i*h
        case 0x85: z80->a = addb(z80, z80->a, *reg, 0); break; // add a, i*l
        case 0x86:
            z80->a = addb(z80, z80->a, readw(z80, *reg + dispb(z80)), 0);
            break; // add a, (i* + d)
        case 0x8c:
            z80->a = addb(z80, z80->a, *reg >> 8, z80->f & C_FLAG);
            break; // adc a, i*h
        case 0x8d:
            z80->a = addb(z80, z80->a, *reg, z80->f & C_FLAG);
            break; // adc a, i*l
        case 0x8e:
            z80->a = addb(z80, z80->a, readw(z80, *reg + dispb(z80)), z80->f & C_FLAG);
            break; // adc a, (i* + d)
        case 0x94:
            z80->a = subb(z80, z80->a, *reg >> 8, 0);
            break;                                             // sub a, i*h
        case 0x95: z80->a = subb(z80, z80->a, *reg, 0); break; // sub a, i*l
        case 0x96:
            z80->a = subb(z80, z80->a, readw(z80, *reg + dispb(z80)), 0);
            break; // sub a, (i* + d)
        case 0x9c:
            z80->a = subb(z80, z80->a, *reg >> 8, z80->f & C_FLAG);
            break; // sbc a, i*h
        case 0x9d:
            z80->a = subb(z80, z80->a, *reg, z80->f & C_FLAG);
            break; // sbc a, i*l
        case 0x9e:
            z80->a = subb(z80, z80->a, readw(z80, *reg + dispb(z80)), z80->f & C_FLAG);
            break;                             // sbc a, (i* + d)
        case 0xa4: and(z80, *reg >> 8); break; // and i*h
        case 0xa5: and(z80, *reg); break;      // and i*l
        case 0xa6:
            and(z80, readw(z80, *reg + dispb(z80)));
            break;                             // and (i* + d)
        case 0xac: xor(z80, *reg >> 8); break; // xor i*h
        case 0xad: xor(z80, *reg); break;      // xor i*l
        case 0xae:
            xor(z80, readw(z80, *reg + dispb(z80)));
            break;                             // xor (i* + d)
        case 0xb4: or (z80, *reg >> 8); break; // or i*h
        case 0xb5: or (z80, *reg); break;      // or i*l
        case 0xb6:
            or (z80, readw(z80, *reg + dispb(z80)));
            break;                                                // or (i* + d)
        case 0xbc: cp(z80, *reg >> 8); break;                     // cp i*h
        case 0xbd: cp(z80, *reg); break;                          // cp i*l
        case 0xbe: cp(z80, readw(z80, *reg + dispb(z80))); break; // cp (i* + d)
        case 0xe1: *reg = pop(z80); break;                        // pop i*
        case 0xe3: {
            uint16_t tmp = *reg;
            *reg = readw(z80, z80->sp);
            writew(z80, z80->sp, tmp);
            break;
        }                                  // ex (sp), i*
        case 0xe5: push(z80, *reg); break; // push i*
        case 0xe9: z80->pc = *reg; break;  // jp (i*)
        case 0xf9: z80->sp = *reg; break;  // ld sp, i*

        case 0xcb: exec_indexcb_instr(z80, sel); break;

        default:
            z80->cycles += 4;
            exec_instr(z80, opcode);
            return; // nop
    }

    z80->cycles += index_opcode_cycles[opcode];
}

static void exec_cb_instr(struct Z80* z80, uint8_t const opcode)
{
    uint8_t const op = opcode >> 6;
    uint8_t const type = (opcode >> 3) & 0x7;
    uint8_t const dest = opcode & 0x07;
    uint8_t val;

    incr(z80);

    switch (dest)
    {
        case 0x00: val = z80->b; break;
        case 0x01: val = z80->c; break;
        case 0x02: val = z80->d; break;
        case 0x03: val = z80->e; break;
        case 0x04: val = z80->h; break;
        case 0x05: val = z80->l; break;
        case 0x06: val = readb(z80, z80->hl); break;
        case 0x07: val = z80->a; break;
    }

    switch (op)
    {
        case 0x00: // rotation
        {
            switch (type)
            {
                case 0x00: val = rlc(z80, val); break; // rlc
                case 0x01: val = rrc(z80, val); break; // rrc
                case 0x02: val = rl(z80, val); break;  // rl
                case 0x03: val = rr(z80, val); break;  // rr
                case 0x04: val = sla(z80, val); break; // sla
                case 0x05: val = sra(z80, val); break; // sra
                case 0x06: val = sll(z80, val); break; // sll
                case 0x07: val = srl(z80, val); break; // srl
            }
            break;
        }
        case 0x01: // bit
        {
            val = val & (1 << type);
            z80->f = szflags(val) | xyflags(val) | H_FLAG | (z80->f & C_FLAG);
            if (z80->f & Z_FLAG)
            {
                z80->f |= P_FLAG;
            }
            break;
        }
        case 0x02: // reset
        {
            val &= ~(1 << type);
            break;
        }
        case 0x03: // set
        {
            val |= (1 << type);
            break;
        }
    }

    if (op != 0x01)
    {
        switch (dest)
        {
            case 0x00: z80->b = val; break;
            case 0x01: z80->c = val; break;
            case 0x02: z80->d = val; break;
            case 0x03: z80->e = val; break;
            case 0x04: z80->h = val; break;
            case 0x05: z80->l = val; break;
            case 0x06: writeb(z80, z80->hl, val); break;
            case 0x07: z80->a = val; break;
        }

        z80->cycles += (dest == 0x06 ? 15 : 8);
    }
    else
    {
        z80->cycles += (dest == 0x06 ? 12 : 8);
    }
}

static void exec_ed_instr(struct Z80* z80, uint8_t const opcode)
{
    incr(z80);

    switch (opcode)
    {
        case 0x40: z80->b = inbc(z80); break;        // in b, (c)
        case 0x41: out(z80, z80->bc, z80->b); break; // out (c), b
        case 0x42:
            z80->hl = subcw(z80, z80->hl, z80->bc, z80->f & C_FLAG);
            break;                                           // sbc hl, bc
        case 0x43: writew(z80, instrw(z80), z80->bc); break; // ld (nn), bc
        case 0x44: z80->a = subb(z80, 0, z80->a, 0); break;  // neg
        case 0x45:
        case 0x55:
        case 0x65:
        case 0x75:
        case 0x7d:
            z80->pc = pop(z80);
            z80->iff1 = z80->iff2;
            break; // retn
        case 0x46:
        case 0x66: z80->interrupt_mode = 0; break;   // im 0
        case 0x47: z80->i = z80->a; break;           // ld i, a
        case 0x48: z80->c = inbc(z80); break;        // in c, (c)
        case 0x49: out(z80, z80->bc, z80->c); break; // out (c), c
        case 0x4a:
            z80->hl = addcw(z80, z80->hl, z80->bc, z80->f & C_FLAG);
            break;                                           // adc hl, bc
        case 0x4b: z80->bc = readw(z80, instrw(z80)); break; // ld bc, (nn)
        case 0x4d:
        case 0x5d:
        case 0x6d: z80->pc = pop(z80); break; // reti
        case 0x4e:
        case 0x6e: z80->interrupt_mode = 0; break;   // im 0/1 [undoc]
        case 0x4f: z80->r = z80->a; break;           // ld r, a
        case 0x50: z80->d = inbc(z80); break;        // in d, (c)
        case 0x51: out(z80, z80->bc, z80->d); break; // out (c), d
        case 0x52:
            z80->hl = subcw(z80, z80->hl, z80->de, z80->f & C_FLAG);
            break;                                           // sbc hl, de
        case 0x53: writew(z80, instrw(z80), z80->de); break; // ld (nn), de
        case 0x4c:
        case 0x54:
        case 0x5c:
        case 0x64:
        case 0x6c:
        case 0x74:
        case 0x7c: z80->a = subb(z80, 0, z80->a, 0); break; // neg [undoc]
        case 0x56:
        case 0x76: z80->interrupt_mode = 1; break; // im 1
        case 0x57:
            z80->a = z80->i;
            z80->f &= C_FLAG;
            z80->f |= szflags(z80->a) | xyflags(z80->a);
            break;                                   // ld a, i
        case 0x58: z80->e = inbc(z80); break;        // in e, (c)
        case 0x59: out(z80, z80->bc, z80->e); break; // out (c), e
        case 0x5a:
            z80->hl = addcw(z80, z80->hl, z80->de, z80->f & C_FLAG);
            break;                                           // adc hl, de
        case 0x5b: z80->de = readw(z80, instrw(z80)); break; // ld de, (nn)
        case 0x5e:
        case 0x7e: z80->interrupt_mode = 2; break; // im 2
        case 0x5f:
            z80->a = z80->r;
            z80->f &= C_FLAG;
            z80->f |= szflags(z80->a) | xyflags(z80->a);
            break;                                   // ld a, r
        case 0x60: z80->h = inbc(z80); break;        // in h, (c)
        case 0x61: out(z80, z80->bc, z80->h); break; // out (c), h
        case 0x62:
            z80->hl = subcw(z80, z80->hl, z80->hl, z80->f & C_FLAG);
            break;                                           // sbc hl, hl
        case 0x63: writew(z80, instrw(z80), z80->hl); break; // ld (nn), hl
        case 0x67: rrd(z80); break;                          // rrd
        case 0x68: z80->l = inbc(z80); break;                // in l, (c)
        case 0x69: out(z80, z80->bc, z80->l); break;         // out (c), l
        case 0x6a:
            z80->hl = addcw(z80, z80->hl, z80->hl, z80->f & C_FLAG);
            break;                                           // adc hl, hl
        case 0x6b: z80->hl = readw(z80, instrw(z80)); break; // ld hl, (nn)
        case 0x6f: rld(z80); break;                          // rld
        case 0x70: inbc(z80); break;                         // in (c)
        case 0x71: out(z80, z80->bc, 0); break;              // out (c), 0
        case 0x72:
            z80->hl = subcw(z80, z80->hl, z80->sp, z80->f & C_FLAG);
            break;                                           // sbc hl, sp
        case 0x73: writew(z80, instrw(z80), z80->sp); break; // ld (nn), sp
        case 0x77:
        case 0x7f: break;                            // nop
        case 0x78: z80->a = inbc(z80); break;        // in a, (c)
        case 0x79: out(z80, z80->bc, z80->a); break; // out (c), a
        case 0x7a:
            z80->hl = addcw(z80, z80->hl, z80->sp, z80->f & C_FLAG);
            break;                                           // adc hl, sp
        case 0x7b: z80->sp = readw(z80, instrw(z80)); break; // ld sp, (nn)
        case 0xa0: ldi(z80); break;                          // ldi
        case 0xa1: cpi(z80); break;                          // cpi
        case 0xa2: ini(z80); break;
        case 0xa3: outi(z80); break; // outi
        case 0xa8: ldd(z80); break;  // ldd
        case 0xa9: cpd(z80); break;  // cpd
        case 0xaa: ind(z80); break;  // ind
        case 0xab: outd(z80); break; // outd
        case 0xb0: ldir(z80); break; // ldir
        case 0xb1: cpir(z80); break; // cpir
        case 0xb2: inir(z80); break; // inir
        case 0xb3: otir(z80); break; // otir
        case 0xb8: lddr(z80); break; // lddr
        case 0xb9: cpdr(z80); break; // cpdr
        case 0xba: indr(z80); break; // indr
        case 0xbb: otdr(z80); break; // otdr

        default:
            printf("unimplemented opcode 0xed%02X at %x\n", opcode, z80->pc - 1);
            assert(0);
            exit(EXIT_FAILURE);
            break;
    }

    z80->cycles += ed_opcode_cycles[opcode];
}

static void exec_instr(struct Z80* z80, uint8_t const opcode)
{
    switch (opcode)
    {
        case 0x00: break;                               // nop
        case 0x01: z80->bc = instrw(z80); break;        // ld bc, nn
        case 0x02: writeb(z80, z80->bc, z80->a); break; // ld (bc), a
        case 0x03: ++z80->bc; break;                    // inc bc
        case 0x04: z80->b = incb(z80, z80->b); break;   // inc b
        case 0x05: z80->b = decb(z80, z80->b); break;   // dec b
        case 0x06: z80->b = instrb(z80); break;         // ld b, n
        case 0x07: rlca(z80); break;                    // rcla
        case 0x08: {
            uint16_t const af = z80->af;
            z80->af = z80->afp;
            z80->afp = af;
            break;
        }                                                        // ex af, af'
        case 0x09: z80->hl = addw(z80, z80->hl, z80->bc); break; // add hl, bc
        case 0x0a: z80->a = readb(z80, z80->bc); break;          // ld a, (bc)
        case 0x0b: --z80->bc; break;                             // dec bc
        case 0x0c: z80->c = incb(z80, z80->c); break;            // inc c
        case 0x0d: z80->c = decb(z80, z80->c); break;            // dec c
        case 0x0e: z80->c = instrb(z80); break;                  // ld c, n
        case 0x0f: rrca(z80); break;                             // rrca
        case 0x10:
            --z80->b;
            jr(z80, z80->b);
            break;                                               // djnz d
        case 0x11: z80->de = instrw(z80); break;                 // ld de, nn
        case 0x12: writeb(z80, z80->de, z80->a); break;          // ld (de), a
        case 0x13: ++z80->de; break;                             // inc de
        case 0x14: z80->d = incb(z80, z80->d); break;            // inc d
        case 0x15: z80->d = decb(z80, z80->d); break;            // dec d
        case 0x16: z80->d = instrb(z80); break;                  // ld d, n
        case 0x17: rla(z80); break;                              // rla
        case 0x18: jr(z80, 1); break;                            // jr d
        case 0x19: z80->hl = addw(z80, z80->hl, z80->de); break; // add hl, de
        case 0x1a: z80->a = readb(z80, z80->de); break;          // ld a, (de)
        case 0x1b: --z80->de; break;                             // dec de
        case 0x1c: z80->e = incb(z80, z80->e); break;            // inc e
        case 0x1d: z80->e = decb(z80, z80->e); break;            // dec e
        case 0x1e: z80->e = instrb(z80); break;                  // ld e, n
        case 0x1f: rra(z80); break;                              // rra
        case 0x20: jr(z80, ~z80->f & Z_FLAG); break;             // jr nz, d
        case 0x21: z80->hl = instrw(z80); break;                 // ld hl, nn
        case 0x22: writew(z80, instrw(z80), z80->hl); break;     // ld (nn), hl
        case 0x23: ++z80->hl; break;                             // inc hl
        case 0x24: z80->h = incb(z80, z80->h); break;            // inc h
        case 0x25: z80->h = decb(z80, z80->h); break;            // dec h
        case 0x26: z80->h = instrb(z80); break;                  // ld h, n
        case 0x27: daa(z80); break;                              // daa
        case 0x28: jr(z80, z80->f & Z_FLAG); break;              // jr z, d
        case 0x29: z80->hl = addw(z80, z80->hl, z80->hl); break; // add hl, hl
        case 0x2a: z80->hl = readw(z80, instrw(z80)); break;     // ld hl, (nn)
        case 0x2b: --z80->hl; break;                             // dec hl
        case 0x2c: z80->l = incb(z80, z80->l); break;            // inc l
        case 0x2d: z80->l = decb(z80, z80->l); break;            // dec l
        case 0x2e: z80->l = instrb(z80); break;                  // ld l, n
        case 0x2f: cpl(z80); break;                              // cpl
        case 0x30: jr(z80, ~z80->f & C_FLAG); break;             // jr nc, d
        case 0x31: z80->sp = instrw(z80); break;                 // ld sp, nn
        case 0x32: writeb(z80, instrw(z80), z80->a); break;      // ld (nn), a
        case 0x33: ++z80->sp; break;                             // inc sp
        case 0x34:
            writeb(z80, z80->hl, incb(z80, readb(z80, z80->hl)));
            break; // inc (hl)
        case 0x35:
            writeb(z80, z80->hl, decb(z80, readb(z80, z80->hl)));
            break;                                               // dec (hl)
        case 0x36: writeb(z80, z80->hl, instrb(z80)); break;     // ld (hl), n
        case 0x37: scf(z80); break;                              // scf
        case 0x38: jr(z80, z80->f & C_FLAG); break;              // jr c, d
        case 0x39: z80->hl = addw(z80, z80->hl, z80->sp); break; // add hl, sp
        case 0x3a: z80->a = readb(z80, instrw(z80)); break;      // ld a, (nn)
        case 0x3b: --z80->sp; break;                             // dec sp
        case 0x3c: z80->a = incb(z80, z80->a); break;            // inc a
        case 0x3d: z80->a = decb(z80, z80->a); break;            // dec a
        case 0x3e: z80->a = instrb(z80); break;                  // ld a, n
        case 0x3f: ccf(z80); break;                              // ccf
        case 0x40: z80->b = z80->b; break;                       // ld b, b
        case 0x41: z80->b = z80->c; break;                       // ld b, c
        case 0x42: z80->b = z80->d; break;                       // ld b, d
        case 0x43: z80->b = z80->e; break;                       // ld b, e
        case 0x44: z80->b = z80->h; break;                       // ld b, h
        case 0x45: z80->b = z80->l; break;                       // ld b, l
        case 0x46: z80->b = readw(z80, z80->hl); break;          // ld b, (hl)
        case 0x47: z80->b = z80->a; break;                       // ld b, a
        case 0x48: z80->c = z80->b; break;                       // ld c, b
        case 0x49: z80->c = z80->c; break;                       // ld c, c
        case 0x4a: z80->c = z80->d; break;                       // ld c, d
        case 0x4b: z80->c = z80->e; break;                       // ld c, e
        case 0x4c: z80->c = z80->h; break;                       // ld c, h
        case 0x4d: z80->c = z80->l; break;                       // ld c, l
        case 0x4e: z80->c = readw(z80, z80->hl); break;          // ld c, (hl)
        case 0x4f: z80->c = z80->a; break;                       // ld c, a
        case 0x50: z80->d = z80->b; break;                       // ld d, b
        case 0x51: z80->d = z80->c; break;                       // ld d, c
        case 0x52: z80->d = z80->d; break;                       // ld d, d
        case 0x53: z80->d = z80->e; break;                       // ld d, e
        case 0x54: z80->d = z80->h; break;                       // ld d, h
        case 0x55: z80->d = z80->l; break;                       // ld d, l
        case 0x56: z80->d = readw(z80, z80->hl); break;          // ld d, (hl)
        case 0x57: z80->d = z80->a; break;                       // ld d, a
        case 0x58: z80->e = z80->b; break;                       // ld e, b
        case 0x59: z80->e = z80->c; break;                       // ld e, c
        case 0x5a: z80->e = z80->d; break;                       // ld e, d
        case 0x5b: z80->e = z80->e; break;                       // ld e, e
        case 0x5c: z80->e = z80->h; break;                       // ld e, h
        case 0x5d: z80->e = z80->l; break;                       // ld e, l
        case 0x5e: z80->e = readw(z80, z80->hl); break;          // ld e, (hl)
        case 0x5f: z80->e = z80->a; break;                       // ld e, a
        case 0x60: z80->h = z80->b; break;                       // ld h, b
        case 0x61: z80->h = z80->c; break;                       // ld h, c
        case 0x62: z80->h = z80->d; break;                       // ld h, d
        case 0x63: z80->h = z80->e; break;                       // ld h, e
        case 0x64: z80->h = z80->h; break;                       // ld h, h
        case 0x65: z80->h = z80->l; break;                       // ld h, l
        case 0x66: z80->h = readw(z80, z80->hl); break;          // ld h, (hl)
        case 0x67: z80->h = z80->a; break;                       // ld h, a
        case 0x68: z80->l = z80->b; break;                       // ld l, b
        case 0x69: z80->l = z80->c; break;                       // ld l, c
        case 0x6a: z80->l = z80->d; break;                       // ld l, d
        case 0x6b: z80->l = z80->e; break;                       // ld l, e
        case 0x6c: z80->l = z80->h; break;                       // ld l, h
        case 0x6d: z80->l = z80->l; break;                       // ld l, l
        case 0x6e: z80->l = readw(z80, z80->hl); break;          // ld l, (hl)
        case 0x6f: z80->l = z80->a; break;                       // ld l, a
        case 0x70: writeb(z80, z80->hl, z80->b); break;          // ld (hl), b
        case 0x71: writeb(z80, z80->hl, z80->c); break;          // ld (hl), c
        case 0x72: writeb(z80, z80->hl, z80->d); break;          // ld (hl), d
        case 0x73: writeb(z80, z80->hl, z80->e); break;          // ld (hl), e
        case 0x74: writeb(z80, z80->hl, z80->h); break;          // ld (hl), h
        case 0x75: writeb(z80, z80->hl, z80->l); break;          // ld (hl), l
        case 0x76:
            z80->halted = 1;
            break;                                               // halt
        case 0x77: writeb(z80, z80->hl, z80->a); break;          // ld (hl), a
        case 0x78: z80->a = z80->b; break;                       // ld a, b
        case 0x79: z80->a = z80->c; break;                       // ld a, c
        case 0x7a: z80->a = z80->d; break;                       // ld a, d
        case 0x7b: z80->a = z80->e; break;                       // ld a, e
        case 0x7c: z80->a = z80->h; break;                       // ld a, h
        case 0x7d: z80->a = z80->l; break;                       // ld a, l
        case 0x7e: z80->a = readb(z80, z80->hl); break;          // ld a, (hl)
        case 0x7f: z80->a = z80->a; break;                       // ld a, a
        case 0x80: z80->a = addb(z80, z80->a, z80->b, 0); break; // add a, b
        case 0x81: z80->a = addb(z80, z80->a, z80->c, 0); break; // add a, c
        case 0x82: z80->a = addb(z80, z80->a, z80->d, 0); break; // add a, d
        case 0x83: z80->a = addb(z80, z80->a, z80->e, 0); break; // add a, e
        case 0x84: z80->a = addb(z80, z80->a, z80->h, 0); break; // add a, h
        case 0x85: z80->a = addb(z80, z80->a, z80->l, 0); break; // add a, l
        case 0x86:
            z80->a = addb(z80, z80->a, readb(z80, z80->hl), 0);
            break;                                               // add a, (hl)
        case 0x87: z80->a = addb(z80, z80->a, z80->a, 0); break; // add a, a
        case 0x88:
            z80->a = addb(z80, z80->a, z80->b, z80->f & C_FLAG);
            break; // adc a, b
        case 0x89:
            z80->a = addb(z80, z80->a, z80->c, z80->f & C_FLAG);
            break; // adc a, c
        case 0x8a:
            z80->a = addb(z80, z80->a, z80->d, z80->f & C_FLAG);
            break; // adc a, d
        case 0x8b:
            z80->a = addb(z80, z80->a, z80->e, z80->f & C_FLAG);
            break; // adc a, e
        case 0x8c:
            z80->a = addb(z80, z80->a, z80->h, z80->f & C_FLAG);
            break; // adc a, h
        case 0x8d:
            z80->a = addb(z80, z80->a, z80->l, z80->f & C_FLAG);
            break; // adc a, l
        case 0x8e:
            z80->a = addb(z80, z80->a, readb(z80, z80->hl), z80->f & C_FLAG);
            break; // adc a, (hl)
        case 0x8f:
            z80->a = addb(z80, z80->a, z80->a, z80->f & C_FLAG);
            break;                                               // adc a, a
        case 0x90: z80->a = subb(z80, z80->a, z80->b, 0); break; // sub a, b
        case 0x91: z80->a = subb(z80, z80->a, z80->c, 0); break; // sub a, c
        case 0x92: z80->a = subb(z80, z80->a, z80->d, 0); break; // sub a, d
        case 0x93: z80->a = subb(z80, z80->a, z80->e, 0); break; // sub a, e
        case 0x94: z80->a = subb(z80, z80->a, z80->h, 0); break; // sub a, h
        case 0x95: z80->a = subb(z80, z80->a, z80->l, 0); break; // sub a, l
        case 0x96:
            z80->a = subb(z80, z80->a, readb(z80, z80->hl), 0);
            break;                                               // sub a, (hl)
        case 0x97: z80->a = subb(z80, z80->a, z80->a, 0); break; // sub a, a
        case 0x98:
            z80->a = subb(z80, z80->a, z80->b, z80->f & C_FLAG);
            break; // sbc a, b
        case 0x99:
            z80->a = subb(z80, z80->a, z80->c, z80->f & C_FLAG);
            break; // sbc a, c
        case 0x9a:
            z80->a = subb(z80, z80->a, z80->d, z80->f & C_FLAG);
            break; // sbc a, d
        case 0x9b:
            z80->a = subb(z80, z80->a, z80->e, z80->f & C_FLAG);
            break; // sbc a, e
        case 0x9c:
            z80->a = subb(z80, z80->a, z80->h, z80->f & C_FLAG);
            break; // sbc a, h
        case 0x9d:
            z80->a = subb(z80, z80->a, z80->l, z80->f & C_FLAG);
            break; // sbc a, l
        case 0x9e:
            z80->a = subb(z80, z80->a, readb(z80, z80->hl), z80->f & C_FLAG);
            break; // sbc a, (hl)
        case 0x9f:
            z80->a = subb(z80, z80->a, z80->a, z80->f & C_FLAG);
            break;                                       // sbc a, a
        case 0xa0: and(z80, z80->b); break;              // and b
        case 0xa1: and(z80, z80->c); break;              // and c
        case 0xa2: and(z80, z80->d); break;              // and d
        case 0xa3: and(z80, z80->e); break;              // and e
        case 0xa4: and(z80, z80->h); break;              // and h
        case 0xa5: and(z80, z80->l); break;              // and l
        case 0xa6: and(z80, readw(z80, z80->hl)); break; // and (hl)
        case 0xa7: and(z80, z80->a); break;              // and a
        case 0xa8: xor(z80, z80->b); break;              // xor b
        case 0xa9: xor(z80, z80->c); break;              // xor c
        case 0xaa: xor(z80, z80->d); break;              // xor d
        case 0xab: xor(z80, z80->e); break;              // xor e
        case 0xac: xor(z80, z80->h); break;              // xor h
        case 0xad: xor(z80, z80->l); break;              // xor l
        case 0xae: xor(z80, readw(z80, z80->hl)); break; // xor (hl)
        case 0xaf: xor(z80, z80->a); break;              // xor a
        case 0xb0: or (z80, z80->b); break;              // or b
        case 0xb1: or (z80, z80->c); break;              // or c
        case 0xb2: or (z80, z80->d); break;              // or d
        case 0xb3: or (z80, z80->e); break;              // or e
        case 0xb4: or (z80, z80->h); break;              // or h
        case 0xb5: or (z80, z80->l); break;              // or l
        case 0xb6: or (z80, readw(z80, z80->hl)); break; // or (hl)
        case 0xb7: or (z80, z80->a); break;              // or a
        case 0xb8: cp(z80, z80->b); break;               // cp b
        case 0xb9: cp(z80, z80->c); break;               // cp c
        case 0xba: cp(z80, z80->d); break;               // cp d
        case 0xbb: cp(z80, z80->e); break;               // cp e
        case 0xbc: cp(z80, z80->h); break;               // cp h
        case 0xbd: cp(z80, z80->l); break;               // cp l
        case 0xbe: cp(z80, readw(z80, z80->hl)); break;  // cp (hl)
        case 0xbf: cp(z80, z80->a); break;               // cp a
        case 0xc0: retc(z80, ~z80->f & Z_FLAG); break;   // ret nz
        case 0xc1: z80->bc = pop(z80); break;            // pop bc
        case 0xc2: jp(z80, ~z80->f & Z_FLAG); break;     // jp nz, nn
        case 0xc3: z80->pc = instrw(z80); break;         // jp nn
        case 0xc4: callc(z80, ~z80->f & Z_FLAG); break;  // call nz, nn
        case 0xc5: push(z80, z80->bc); break;            // push bc
        case 0xc6:
            z80->a = addb(z80, z80->a, instrb(z80), 0);
            break;                                     // add a, n
        case 0xc8: retc(z80, z80->f & Z_FLAG); break;  // ret z
        case 0xc9: z80->pc = pop(z80); break;          // ret
        case 0xca: jp(z80, z80->f & Z_FLAG); break;    // jp z, nn
        case 0xcc: callc(z80, z80->f & Z_FLAG); break; // call z, nn
        case 0xcd: call(z80); break;                   // call nn
        case 0xce:
            z80->a = addb(z80, z80->a, instrb(z80), z80->f & C_FLAG);
            break;                                       // adc a, n
        case 0xd0: retc(z80, ~z80->f & C_FLAG); break;   // ret nc
        case 0xd1: z80->de = pop(z80); break;            // pop de
        case 0xd2: jp(z80, ~z80->f & C_FLAG); break;     // jp nc, nn
        case 0xd3: out(z80, instrb(z80), z80->a); break; // out (n), a
        case 0xd4: callc(z80, ~z80->f & C_FLAG); break;  // call nc, nn
        case 0xd5: push(z80, z80->de); break;            // push de
        case 0xd6:
            z80->a = subb(z80, z80->a, instrb(z80), 0);
            break;                                    // adc a, n
        case 0xd8: retc(z80, z80->f & C_FLAG); break; // ret c
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
        case 0xda: jp(z80, z80->f & C_FLAG); break; // jp c, nn
        case 0xdb:
            z80->a = in(z80, ((uint16_t)z80->a << 8) | instrb(z80));
            break;                                     // in a, (n)
        case 0xdc: callc(z80, z80->f & C_FLAG); break; // call c, nn
        case 0xde:
            z80->a = subb(z80, z80->a, instrb(z80), z80->f & C_FLAG);
            break;                                     // sbc a, n
        case 0xe0: retc(z80, ~z80->f & P_FLAG); break; // ret po
        case 0xe1: z80->hl = pop(z80); break;          // pop hl
        case 0xe2: jp(z80, ~z80->f & P_FLAG); break;   // jp po, nn
        case 0xe3: {
            uint16_t hl = z80->hl;
            z80->hl = readw(z80, z80->sp);
            writew(z80, z80->sp, hl);
            break;
        }                                               // ex (sp), hl
        case 0xe4: callc(z80, ~z80->f & P_FLAG); break; // call po, nn
        case 0xe5: push(z80, z80->hl); break;           // push hl
        case 0xe6: and(z80, instrb(z80)); break;        // and n
        case 0xe8: retc(z80, z80->f & P_FLAG); break;   // ret pe
        case 0xe9: z80->pc = z80->hl; break;            // jp (hl)
        case 0xea: jp(z80, z80->f & P_FLAG); break;     // jp pe, nn
        case 0xeb: {
            uint16_t de = z80->de;
            z80->de = z80->hl;
            z80->hl = de;
            break;
        }                                              // ex de, hl
        case 0xec: callc(z80, z80->f & P_FLAG); break; // call pe, nn
        case 0xed: exec_ed_instr(z80, instrb(z80)); break;
        case 0xee: xor(z80, instrb(z80)); break;        // xor n
        case 0xf0: retc(z80, ~z80->f & S_FLAG); break;  // ret p
        case 0xf1: z80->af = pop(z80); break;           // pop af
        case 0xf2: jp(z80, ~z80->f & S_FLAG); break;    // jp p, nn
        case 0xf3: z80->iff1 = z80->iff2 = 0; break;    // di
        case 0xf4: callc(z80, ~z80->f & S_FLAG); break; // call p, nn
        case 0xf5: push(z80, z80->af); break;           // push af
        case 0xf6: or (z80, instrb(z80)); break;        // or n
        case 0xf8: retc(z80, z80->f & S_FLAG); break;   // ret m
        case 0xf9: z80->sp = z80->hl; break;            // ld sp, hl
        case 0xfa: jp(z80, z80->f & S_FLAG); break;     // jp m, nn
        case 0xfb: {
            z80->iff1 = z80->iff2 = 1;
            if (z80->interrupt_delay == 0)
                z80->interrupt_delay = 2;
            break;
        }                                              // ei
        case 0xfc: callc(z80, z80->f & S_FLAG); break; // call m, nn
        case 0xfe: cp(z80, instrb(z80)); break;        // cp n

        case 0xc7:
        case 0xcf:
        case 0xd7:
        case 0xdf:
        case 0xe7:
        case 0xef:
        case 0xf7:
        case 0xff:
            push(z80, z80->pc);
            z80->pc = 0x08 * ((opcode & 0x38) >> 3);
            break;

        case 0xcb: exec_cb_instr(z80, instrb(z80)); break;

        case 0xdd:
        case 0xfd: exec_index_instr(z80, opcode, instrb(z80)); break;

        default:
            printf("unimplemented opcode 0x%02x at 0x%04x\n", opcode, z80->pc - 1);
            assert(0);
            exit(EXIT_FAILURE);
            break;
    }

    z80->cycles += opcode_cycles[opcode];
}

/*****************************************************************************/

void z80_init(struct Z80* z80)
{
    memset(z80, 0, sizeof(struct Z80));
    z80->sp = 0xffff;
    z80->a = 0xff;
}

int64_t z80_step(struct Z80* z80)
{
    int64_t const cycles = z80->cycles;

    incr(z80);

    if (!z80->halted)
    {
        uint8_t const opcode = instrb(z80);
        if (!z80->trap || !z80->trap(z80, z80->pc - 1, opcode))
            exec_instr(z80, opcode);
    }
    else
    {
        exec_instr(z80, 0x00);
    }

    if (z80->interrupt_delay)
    {
        if (--z80->interrupt_delay == 0)
            z80->iff1 = z80->iff2 = 1;
    }

    return z80->cycles - cycles;
}

int z80_is_halted(struct Z80 const* z80)
{
    return z80->halted;
}

void z80_interrupt(struct Z80* z80, uint8_t data)
{
    if (z80->interrupt_delay == 0)
        handle_interrupts(z80, data);
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
    printf("PC:0x%04X SP:0x%04X IX:0x%04X IY:0x%04X\n",
           z80->pc,
           z80->sp,
           z80->ix,
           z80->iy);
}
