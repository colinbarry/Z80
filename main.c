#include "z80.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t mem_load(void* userdata, uint16_t const addr)
{
    return ((uint8_t*)userdata)[addr];
}

static void mem_store(void* userdata, uint16_t const addr, uint8_t const value)
{
    ((uint8_t*)userdata)[addr] = value;
}

int main(int argc, char **argv)
{
    struct Z80 z80;
    FILE* romfile;
    int length;
    uint8_t memory[65536] = {
        0x21, 0x42, 0x42, // ld hl, nn
        0x11, 0x11, 0x11, // ld de, nn
        0x19,             // add hl, de
        0x76              // halt
    };

    memset(memory, 0, sizeof(memory));

    if ((romfile = fopen("./48.rom", "rb")) == NULL) {
        printf("could not open file ./48.rom\n");
        exit(EXIT_FAILURE);
    }

    fseek(romfile, 0, SEEK_END);
    length = ftell(romfile);
    fseek(romfile, 0, SEEK_SET);
    printf("loading rom of size %i\n", length);

    if (length > sizeof(memory))
        length = sizeof(memory);
    fread(memory, 1, length, romfile);
    fclose(romfile);

    z80_init(&z80);
    z80.userdata = memory;
    z80.mem_load = &mem_load;
    z80.mem_store = &mem_store;

    while (!z80_is_halted(&z80)) {
        z80_step(&z80);
    }

    z80_trace(&z80);

    return EXIT_SUCCESS;
}
