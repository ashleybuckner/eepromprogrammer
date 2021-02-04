#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <glib.h>

int memory_loadSRecFile(uint8_t *memory, FILE *fin, unsigned int rombase, unsigned int bffrsize, unsigned int memsize,
                        uint16_t *address);
