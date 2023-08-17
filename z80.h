#include <stdint.h>

struct Z80 {
    uint8_t (*mem_load)(void*, uint16_t);
    void (*mem_store)(void*, uint16_t, uint8_t);
    void* userdata;

    uint16_t pc;
    uint16_t sp;
    uint16_t ix;
    uint16_t iy;
    union { struct { uint8_t f; uint8_t a; }; uint16_t af; };
    union { struct { uint8_t c; uint8_t b; }; uint16_t bc; };
    union { struct { uint8_t e; uint8_t d; }; uint16_t de; };
    union { struct { uint8_t l; uint8_t h; }; uint16_t hl; };
    uint16_t afp;
    uint16_t bcp;
    uint16_t dep;
    uint16_t hlp;
    uint8_t i, r;
    uint8_t iff;
    uint8_t halted;
};

void z80_init(struct Z80* z80);

void z80_step(struct Z80* z80);

int z80_is_halted(struct Z80 const* z80);

/** Debug test code. Will be removed later.
 * */
void z80_trace(struct Z80* z80);
