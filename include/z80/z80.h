#include <stdint.h>

/** State of the Z80 microprocessor.
 */
struct Z80
{
    /** mem_load must be set to a function which will load an 8-bit value
     * from the given address.
     */
    uint8_t (*mem_load)(struct Z80 *, uint16_t);
    /** mem_store must be set to a function which will store an 8-bit value
     * into the given address.
     */
    void (*mem_store)(struct Z80 *, uint16_t, uint8_t);
    /** port_load must be set to a function which will read an 8-bit value
     * from the given port.
     */
    uint8_t (*port_load)(struct Z80 *, uint16_t);
    /** port_store must be set to a function which will write an 8-bit value
     * to the given port.
     */
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

/** Initializes a z80 struct to the default state.
 * @param z80 Pointer to the Z80 struct which will be initialized.
 */
void z80_init(struct Z80 *z80);

/** Fetches and executes the next opcode.
 * @param z80
 * @return Number of cycles taken to execute the step.
 */
int64_t z80_step(struct Z80 *z80);

/** Handle any pending interrupts.
 * @param z80
 * @param data The 8-bit value used for the interrupt in mode 0 and 2.
 */
void z80_interrupt(struct Z80 *z80, uint8_t data);

/** Checks if the Z80 is in a halted state.
 * @return true if the Z80 is halted.
 */
int z80_is_halted(struct Z80 const *z80);

/** Writes the state of the Z80s registers and flags to the console.
 * @param z80
 */
void z80_trace(struct Z80 *z80);
