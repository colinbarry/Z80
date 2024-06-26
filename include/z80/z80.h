#include <stdint.h>

struct Z80
{
    uint8_t (*mem_load)(struct Z80 *, uint16_t);
    void (*mem_store)(struct Z80 *, uint16_t, uint8_t);
    uint8_t (*port_load)(struct Z80 *, uint16_t);
    void (*port_store)(struct Z80 *z80, uint16_t, uint8_t);

    // Trap will be called on each instruction. Returning a truthy value
    // informs the emulator that the trap has handled the operation.
    uint8_t (*trap)(struct Z80 *z80, uint16_t, uint8_t);
    void *userdata;

    uint16_t pc;
    uint16_t sp;
    uint16_t ix;
    uint16_t iy;
    union
    {
        struct
        {
            uint8_t f;
            uint8_t a;
        };
        uint16_t af;
    };
    union
    {
        struct
        {
            uint8_t c;
            uint8_t b;
        };
        uint16_t bc;
    };
    union
    {
        struct
        {
            uint8_t e;
            uint8_t d;
        };
        uint16_t de;
    };
    union
    {
        struct
        {
            uint8_t l;
            uint8_t h;
        };
        uint16_t hl;
    };
    union
    {
        struct
        {
            uint8_t fp;
            uint8_t ap;
        };
        uint16_t afp;
    };
    union
    {
        struct
        {
            uint8_t cp;
            uint8_t bp;
        };
        uint16_t bcp;
    };
    union
    {
        struct
        {
            uint8_t ep;
            uint8_t dp;
        };
        uint16_t dep;
    };
    union
    {
        struct
        {
            uint8_t lp;
            uint8_t hp;
        };
        uint16_t hlp;
    };
    uint8_t i, r;
    uint8_t interrupt_mode;
    uint8_t iff1, iff2;
    uint8_t interrupt_delay;
    uint8_t halted;
    uint64_t cycles;
};

void z80_init(struct Z80 *z80);

int64_t z80_step(struct Z80 *z80);

void z80_interrupt(struct Z80 *z80, uint8_t data);

int z80_is_halted(struct Z80 const *z80);

void z80_trace(struct Z80 *z80);
