#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Registers
{
    uint16_t af;
    uint16_t bc;
    uint16_t de;
    uint16_t hl;
    uint16_t afp;
    uint16_t bcp;
    uint16_t dep;
    uint16_t hlp;
    uint16_t ix;
    uint16_t iy;
    uint16_t sp;
    uint16_t pc;
    uint8_t i;
    uint8_t r;
    uint8_t iff1;
    uint8_t iff2;
    uint8_t im;
};

#define MAX_CHUNK_SIZE (16)

struct Chunk
{
    uint16_t addr;
    int length;
    uint8_t data[MAX_CHUNK_SIZE];
};

#define MAX_NUM_CHUNKS (4)

struct Arrange
{
    struct Registers regs;
    bool halted;
    int cycles;
    int num_chunks;
    struct Chunk const chunks[MAX_NUM_CHUNKS];
};

struct Assert
{
    struct Registers regs;
};

struct Test
{
    char const *label;
    struct Arrange arrange;
    struct Assert assert;
};

struct Tests
{
    size_t num_tests;
    struct Test const tests[];
};

extern struct Tests tests;
