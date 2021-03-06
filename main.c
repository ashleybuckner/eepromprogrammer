//
//  main.c
//  eeprog
//
//  Created by ashley on 27/09/2020.
//

//#define BREADBOARD
//#define STRIPBOARD

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <glib.h>
#include "eeprom.h"
#include "srecord.h"

static int clock_divider = 64;
static unsigned int rombase = 0xe000;
static unsigned int memsize = 0x2000;
static gboolean readrom = FALSE;
static gboolean writerom = FALSE;
static gboolean verifyrom = FALSE;
static gboolean eraserom = FALSE;
static gboolean interactive = FALSE;
static gboolean verbose = FALSE;
static int buffersize = 16;
static int mcpaddr = 0;
static int method = 0;
static int erasebyte = 0xff;
static gboolean testmode = FALSE;
static gchar *format = NULL;
static int delayusec = 10000;

static GOptionEntry entries[] = {
    {"address",     'a', 0, G_OPTION_ARG_INT, &mcpaddr, "MCP address", NULL},
    {"spiclock",    'c', 0, G_OPTION_ARG_INT, &clock_divider, "SPI clock divider", NULL},
    {"rombase",     'b', 0, G_OPTION_ARG_INT, &rombase, "ROM base address", NULL},
    {"memsize",     's', 0, G_OPTION_ARG_INT, &memsize, "ROM size (bytes)", NULL},
    {"read",        'r', 0, G_OPTION_ARG_NONE, &readrom, "Read ROM", NULL},
    {"write",       'w', 0, G_OPTION_ARG_NONE, &writerom, "Write ROM", NULL},
    {"verify",      'y', 0, G_OPTION_ARG_NONE, &verifyrom, "Verify ROM", NULL},
    {"erase",       'x', 0, G_OPTION_ARG_NONE, &eraserom, "Erase ROM", NULL},
    {"interactive", 'i', 0, G_OPTION_ARG_NONE, &interactive, "Interactive", NULL},
    {"verbose",     'v', 0, G_OPTION_ARG_NONE, &verbose, "Verbose mode", NULL},
    {"method",      'm', 0, G_OPTION_ARG_INT, &mcpaddr, "Programming method", NULL},
    {"erasebyte",   'e', 0, G_OPTION_ARG_INT, &erasebyte, "Erase value", NULL},
    {"format",      'f', 0, G_OPTION_ARG_STRING, &format, "Format", NULL},
    {"testmode",    't', 0, G_OPTION_ARG_NONE, &testmode, "Test mode", NULL},
    {"delay",       'd', 0, G_OPTION_ARG_INT, &delayusec, "Programming delay (usec)", NULL},
    {NULL}
};

int naddrpins = 5;

uint8_t addrpins[] = {0, 0, 0, 0, 0};							// Breadboard
uint8_t write_en = 0;
uint8_t output_en = 0;

uint8_t dataport = -1;
uint8_t datadir = -1;
uint8_t addrport = -1;
uint8_t addrdir = -1;

//    Pins for A8-A12
//#ifdef BREADBOARD
//uint8_t addrpins[] = {26, 5, 23, 19, 24};							// Breadboard
//uint8_t write_en = 4;
//uint8_t output_en = 17;

//uint8_t dataport = GPIOA;
//uint8_t datadir = IODIRA;
//uint8_t addrport = GPIOB;
//uint8_t addrdir = IODIRB;
//#endif

//#ifdef STRIPBOARD
//uint8_t addrpins[] = {7, 24, 21, 25, 23};						// Srtipboard
//uint8_t write_en = 16;
//uint8_t output_en = 8;

//uint8_t dataport = GPIOB;
//uint8_t datadir = IODIRB;
//uint8_t addrport = GPIOA;
//uint8_t addrdir = IODIRA;
//#endif

int main(int argc, const char * argv[])
{
    GError *error = NULL;
    GOptionContext *context = g_option_context_new("- MCP23S17.");
    g_option_context_add_main_entries(context, entries, NULL);

    if (!g_option_context_parse(context, &argc, (gchar ***)&argv, &error))
    {
         g_print("option parsing failed: %s\n", error->message);
         exit(1);
    }
    
    GKeyFile *keyfile = g_key_file_new();
    gboolean loaded = g_key_file_load_from_file(keyfile, "/home/pi/.eeprogrc", G_KEY_FILE_NONE, NULL);
 
 //	TO DO - read in npins and put a loop in...   
    if (loaded) {
		addrpins[0] = g_key_file_get_integer(keyfile, "GPIO", "A8", NULL);
		addrpins[1] = g_key_file_get_integer(keyfile, "GPIO", "A9", NULL);
		addrpins[2] = g_key_file_get_integer(keyfile, "GPIO", "A10", NULL);
		addrpins[3] = g_key_file_get_integer(keyfile, "GPIO", "A11", NULL);
		addrpins[4] = g_key_file_get_integer(keyfile, "GPIO", "A12", NULL);
		output_en = g_key_file_get_integer(keyfile, "GPIO", "OE", NULL);
		write_en = g_key_file_get_integer(keyfile, "GPIO", "WE", NULL);
		mcpaddr = g_key_file_get_integer(keyfile, "MCP23S17", "SPIaddress", NULL);
		gchar *port = g_key_file_get_string(keyfile, "MCP23S17", "data", NULL);

		if (g_strcmp0(port, "A")==0) {
			dataport = GPIOA;
			datadir = IODIRA;
			addrport = GPIOB;
			addrdir = IODIRB;
		} else if (g_strcmp0(port, "B")==0) {
			dataport = GPIOB;
			datadir = IODIRB;
			addrport = GPIOA;
			addrdir = IODIRA;
		}
		if (port) g_free(port); 
	} else {
		printf("Could not load eeprogrc\n");
	}

    g_key_file_free(keyfile);
    
    int i;
	if (verbose) {
		puts("EEPROM Programmer v1.0");
		for(i=0; i<naddrpins; i++) {
			g_print("Address pin A%d   = %d\n", i+8, addrpins[i]);
		}
		g_print("Write enable pin      = %d\n", write_en);
		g_print("O/P enable pin        = %d\n", output_en);
		g_print("Data GPIO register    = %d\n", dataport);
		g_print("Data DIR register     = %d\n", datadir);
		g_print("Address GPIO register = %d\n", addrport);
		g_print("Address DIR register  = %d\n", addrdir);
		g_print("SPI address           = %d\n", mcpaddr);
		putchar('\n');
	}
	
//    gchar *filename;
    if (argc<=1 && (writerom || verifyrom)) {
        puts("No filename provided.");
        exit(-1);
    }
    
    if (!bcm2835_init()) {
        printf("bcm2835_init failed. Are you running as root??\n");
        return 1;
     }

     if (!bcm2835_spi_begin()) {
         printf("bcm2835_spi_begin failed. Are you running as root??\n");
         return 1;
     }
     
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(clock_divider);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    MCP23s17 *mcp = mcp23s17_new(mcpaddr);
    if(!mcp) {
		puts("Could not create MCP23S17 object.");
		goto finish;
	}

    EEProm *eeprom = make_EEPROM(mcp, memsize, naddrpins, write_en, output_en, addrdir, datadir, addrport, dataport, addrpins);
    if (!eeprom) {
		goto finish;
	}
	
	eeprom->testmode = testmode;
	eeprom->delayusec = delayusec;

	uint8_t byte = 0x00;
    uint16_t address;
    
    if (interactive) {
		char lnbffr[81];
		address = rombase;
		for(;;) {
			eeprom_readbyte(eeprom, address - rombase, &byte);
//			printf("%04x %02x\n", address, byte);
			printf("%04x %02x ", address, byte);
			fflush(stdout);
			fgets(lnbffr, 80, stdin);
			if (lnbffr[0]=='.') {
				break;
			} else if (lnbffr[0]=='\n') {
				address++;
			} else if (lnbffr[0]=='@') {
				char *p = lnbffr + 1;
				address = (uint16_t)strtol(p, NULL, 16);
			} else if (lnbffr[0]=='-') {
				address--;
			} else {
				char *p = lnbffr;
				char *pend;
				byte = (uint8_t)strtol(p, &pend, 16);
				if (p!=pend) {
					printf("Programming %04x with %02x\n", address, byte);
					eeprom_writebyte(eeprom, address - rombase, byte, method);
					address++;
				}
			}
		}
	}
    
    if (eraserom) {
		byte = erasebyte;
		if (verbose) printf("Erasing EEPROM... memsize = 0x%04x byte = %02x\n", memsize, byte);
        
        for(address = 0; address<memsize; address++) {
			eeprom_writebyte(eeprom, address, byte, method);
		}
	}
    
    if (writerom) {
        FILE *fin = NULL;
        fin = fopen(argv[1], "r");
        if (!fin) {
            printf("No such file!\n");
            goto finish;
        }

        uint8_t *memory = malloc(buffersize); // buffersize
        if (verbose) printf("Programming EEPROM...\n\n");
        for(;!feof(fin);) {
//            int nread = memory_loadSRecFile(memory, fin, rombase, memsize, &address);
            int nread = memory_loadSRecFile(memory, fin, rombase, buffersize, memsize, &address);
            
            if (nread==0) {
                continue;
            }
            
            if (nread==-2) {
                break;
            } else if (nread<0) {
                printf("Read error from memory_loadSRecFile %d.\n", nread);
                goto finish;
            } else {
/*                if (address<rombase || (address - rombase)>memsize) {
                    printf("\nAddress out of bounds %04x\n", address);
                    break;
                }
                
                printf("%04x", address);
                int n;
                for(n=0; n<nread; n++) {
					byte = memory[n];
                    printf(" %02x", byte);
 
					eeprom_writebyte(eeprom, address, byte, 0);
                    address++;
                }
                printf("\n"); */
//               printf("%04x %d\n", address, nread);
                eeprom_writebuffer(eeprom, address - rombase, memory, nread, method);
            }
        }
        
        if (fin) fclose(fin);
        free(memory);
    }

//	TO DO - don't print out 'rows' of 16 0xffs...?
/*    if (readrom) {
        if (verbose) printf("Reading EEPROM... memsize = 0x%04x\n", memsize);
        
        for(address = 0; address<memsize; address++) {
            if ((address & 0x0f)==0) {
                printf("\n%04x", rombase + address);
            }
            
			eeprom_readbyte(eeprom, address, &byte);

            printf(" %02x", byte);
        }

        putchar('\n');
    } */
    
    if (readrom) {
        if (verbose) printf("Reading EEPROM... rombase = 0x%04x, memsize = 0x%04x\n\n", rombase, memsize);
        
		uint8_t bytebffr[16];

        for(address = 0; address<memsize; address+=16) {
			int i;
			int suppress = TRUE;
			for(i=0; i<16; i++) {
				eeprom_readbyte(eeprom, address + i, &byte);
				if (byte!=0xff) suppress = FALSE;
//				printf("%04x %2d %02x %d\n", address, i, byte, suppress);
				bytebffr[i] = byte;
			}
		
			if (suppress) {
				continue;
			}
		
//			suppress = FALSE;
			if (!format) {
				printf("%04x", rombase + address);
				for(i=0; i<16; i++) {
					printf(" %02x", bytebffr[i]);
				}
				
				putchar(' ');
				for(i=0; i<16; i++) {
					byte = bytebffr[i];
					printf("%c", isprint(byte)?byte:'.');
				}
				
				putchar('\n');
			} else if (g_strcmp0(format, "S19")==0) {
				uint8_t checksum = 0;
				g_print("S113%04x", rombase + address);
				for(i=0; i<16; i++) {
					byte = bytebffr[i];
					printf("%02x", byte);
					checksum += byte;
				}
				g_print("%02x\n", (uint8_t)(~checksum));
			}
		}
		
		if (g_strcmp0(format, "S19")==0) {
			puts("S9030000FC");
		}
	}

finish:
    g_option_context_free(context);
    if (eeprom) eeprom_free(eeprom);

    return 0;
}
