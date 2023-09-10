#include "z80.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void catchdebug() {}

static uint8_t mem_load(struct Z80* z80, uint16_t const addr)
{
    return ((uint8_t*)z80->userdata)[addr];
}

static void mem_store(struct Z80* z80, uint16_t const addr, uint8_t const value)
{
    /*
    if (addr == 0x1ab) {
        printf("WR: 0x%04x = 0x%02x PC: 0x%04x\n", addr, value, z80->pc);
    }*/

    ((uint8_t*)z80->userdata)[addr] = value;
}

static uint8_t port_load(struct Z80* z80, uint8_t const port)
{
    return 0;
}

static void port_store(struct Z80* z80, uint8_t port, uint8_t const val)
{
    uint8_t operation = z80->c;

    if (operation == 0x02) {
        printf("%c", z80->e);
    } else if (operation == 0x09) {
        uint16_t addr = (z80->d << 8) | z80->e;
        uint8_t byte;
        while ((byte = mem_load(z80, addr++)) != '$') {
            printf("%c", byte);
        }
    }
}


int main(int argc, char **argv)
{
    struct Z80 z80;
    FILE* romfile;
    int length;
    uint8_t memory[65536] = {0};

    memset(memory, 0, sizeof(memory));

    if ((romfile = fopen("./roms/zexdoc.cim", "rb")) == NULL) {
    //if ((romfile = fopen("./roms/prelim.com", "rb")) == NULL) {
        printf("could not open rom file\n");
        exit(EXIT_FAILURE);
    }

    fseek(romfile, 0, SEEK_END);
    length = ftell(romfile);
    fseek(romfile, 0, SEEK_SET);
    printf("loading rom of size %i\n", length);

    if (length > sizeof(memory))
        length = sizeof(memory);
    fread(memory + 0x100, 1, length, romfile);
    fclose(romfile);
    z80_init(&z80);
    z80.userdata = memory;
    z80.mem_load = &mem_load;
    z80.mem_store = &mem_store;
    z80.port_load = &port_load;
    z80.port_store = &port_store;

    // Inject halt at 0x0000
    memory[0x0000] = 0x76;

    // Inject "out (n), a0" at 0x0005, and hook into this to output characters
    memory[0x0005] = 0xD3;
    memory[0x0006] = 0x00;
    memory[0x0007] = 0xC9;

    z80.pc = 0x100;
    while (!z80_is_halted(&z80)) {
        //printf("0x%04x: ", z80.pc);
        z80_step(&z80);
        //puts("");
    }

    return EXIT_SUCCESS;
}
