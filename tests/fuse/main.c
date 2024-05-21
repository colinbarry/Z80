#include "fuse-tests.h"
#include "z80/z80.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 65536
uint8_t memory[MEMORY_SIZE] = {0};

/**
 * Any named tests will be skipped.
 */
static char const *const tests_to_skip[] = {"76", /* FUSE tests say PC doesn't
                                                   * increment on HALT. Not
                                                   * doing so break PAUSE on the
                                                   * 48k Spectrum. */
                                            NULL};
/** True if the named test is in the skip list.
 */
static bool skip_test(char const *s)
{
    char const *const *test = tests_to_skip;
    while (*test)
    {
        if (strcmp(s, *test) == 0)
        {
            return true;
        }
        else
        {
            ++test;
        }
    }

    return false;
}

#define CHECK_REG(REG)                                                         \
    if (z80.REG != assert->regs.REG)                                           \
    {                                                                          \
        if (!traced_name)                                                      \
        {                                                                      \
            traced_name = true;                                                \
            puts(test->label);                                                 \
        }                                                                      \
        ok = false;                                                            \
        printf("  FAIL: " #REG " // expected 0x%04x, actual 0x%04x\n",         \
               assert->regs.REG,                                               \
               z80.REG);                                                       \
    }

#define CHECK_REG_MASK(REG, MASK)                                              \
    if ((z80.REG & MASK) != (assert->regs.REG &MASK))                          \
    {                                                                          \
        if (!traced_name)                                                      \
        {                                                                      \
            traced_name = true;                                                \
            puts(test->label);                                                 \
        }                                                                      \
        ok = false;                                                            \
        printf("  FAIL: " #REG " // expected 0x%04x, actual 0x%04x\n",         \
               assert->regs.REG &MASK,                                         \
               z80.REG &MASK);                                                 \
    }

uint8_t mem_load(struct Z80 *z80, uint16_t addr)
{
    (void)z80;
    return memory[addr];
}

void mem_store(struct Z80 *z80, uint16_t addr, uint8_t val)
{
    (void)z80;
    memory[addr] = val;
}

uint8_t port_load(struct Z80 *z80, uint16_t port)
{
    (void)z80;
    return port >> 8;
}

void port_store(struct Z80 *z80, uint16_t port, uint8_t val)
{
    (void)z80;
    (void)port;
    (void)val;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    int failures = 0;

    struct Z80 z80;
    for (size_t i = 0; i < tests.num_tests; ++i)
    {
        struct Test const *test = &tests.tests[i];
        struct Arrange const *arrange = &test->arrange;
        struct Assert const *assert = &test->assert;

        if (skip_test(test->label))
            continue;

        z80_init(&z80);
        z80.mem_load = mem_load;
        z80.mem_store = mem_store;
        z80.port_load = port_load;
        z80.port_store = port_store;

        for (int i = 0; i < MEMORY_SIZE; i += 4)
        {
            memory[i] = 0xde;
            memory[i + 1] = 0xad;
            memory[i + 2] = 0xbe;
            memory[i + 3] = 0xef;
        }

        z80.af = arrange->regs.af;
        z80.bc = arrange->regs.bc;
        z80.de = arrange->regs.de;
        z80.hl = arrange->regs.hl;
        z80.afp = arrange->regs.afp;
        z80.bcp = arrange->regs.bcp;
        z80.dep = arrange->regs.dep;
        z80.hlp = arrange->regs.hlp;
        z80.ix = arrange->regs.ix;
        z80.iy = arrange->regs.iy;
        z80.sp = arrange->regs.sp;
        z80.pc = arrange->regs.pc;
        z80.i = arrange->regs.i;
        z80.r = arrange->regs.r;
        z80.iff1 = arrange->regs.iff1;
        z80.iff2 = arrange->regs.iff2;

        z80.halted = arrange->halted;

        for (int ci = 0; ci < arrange->num_chunks; ++ci)
        {
            struct Chunk const *chunk = &arrange->chunks[ci];
            memcpy(memory + chunk->addr, &chunk->data, chunk->length);
        }

        int64_t cycles = 0;
        while (cycles < arrange->cycles)
        {
            cycles += z80_step(&z80);
        }

        bool ok = true;
        bool traced_name = false;

        /** While x and y flags are not totally emulated, we'll mask them
         * out of the af comparisons.
         */
        CHECK_REG_MASK(af, 0xffd7);
        CHECK_REG(bc);
        CHECK_REG(de);
        CHECK_REG(hl);
        CHECK_REG(afp);
        CHECK_REG(bcp);
        CHECK_REG(dep);
        CHECK_REG(hlp);
        CHECK_REG(ix);
        CHECK_REG(iy);
        CHECK_REG(sp);
        CHECK_REG(pc);
        CHECK_REG(i);
        CHECK_REG(r);
        CHECK_REG(iff1);
        CHECK_REG(iff2);

        if (!ok)
            ++failures;
    }

    printf("%i TESTS FAILED\n", failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
