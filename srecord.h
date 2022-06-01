#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

int srec_read(uint8_t *memory, FILE *f, unsigned int rombase, unsigned int bffrsize, unsigned int *address_out, int *rectype);
int ihex_read(uint8_t *memory, FILE *f, unsigned int rombase, unsigned int bffrsize, unsigned int *address_out, int *rectype);
int memory_loadFile(uint8_t *memory, FILE *fin, unsigned int rombase, unsigned int bffrsize, unsigned int memsize,
                        uint16_t *address);
