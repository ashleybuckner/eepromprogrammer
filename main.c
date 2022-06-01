//
//  main.c
//  eeprog
//
//  Created by ashley on 27/09/2020.
//

/*
	EEPROM Programmer v1.0
	Address pin A8   = 26
	Address pin A9   = 5
	Address pin A10   = 23
	Address pin A11   = 19
	Address pin A12   = 24
	Write enable pin      = 4
	O/P enable pin        = 17
	Data GPIO register    = 19
	Data DIR register     = 1
	Address GPIO register = 18
	Address DIR register  = 0
	SPI address           = 0
*/

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
static int buffersize = 256;
static int mcpaddr = 0;
static int method = 0;
static int erasebyte = 0xff;
static gboolean testmode = FALSE;
static gchar *format = NULL;
static int delayusec = 10000;
static int pulsewidth = 1; 
static int linelen = 16;
static gchar *devicetype = NULL;

static GOptionEntry entries[] = {
    {"address",     'a', 0, G_OPTION_ARG_INT, &mcpaddr, "MCP hardware address", NULL},
    {"spiclock",    'c', 0, G_OPTION_ARG_INT, &clock_divider, "SPI clock divider", NULL},
    {"rombase",     'b', 0, G_OPTION_ARG_INT, &rombase, "ROM base address", NULL},
    {"memsize",     's', 0, G_OPTION_ARG_INT, &memsize, "ROM size (bytes)", NULL},
    {"read",        'r', 0, G_OPTION_ARG_NONE, &readrom, "Read ROM", NULL},
    {"write",       'w', 0, G_OPTION_ARG_NONE, &writerom, "Write ROM", NULL},
    {"verify",      'y', 0, G_OPTION_ARG_NONE, &verifyrom, "Verify ROM", NULL},
    {"erase",       'x', 0, G_OPTION_ARG_NONE, &eraserom, "Erase ROM", NULL},
    {"interactive", 'i', 0, G_OPTION_ARG_NONE, &interactive, "Interactive", NULL},
    {"verbose",     'v', 0, G_OPTION_ARG_NONE, &verbose, "Verbose mode", NULL},
    {"method",      'm', 0, G_OPTION_ARG_INT, &method, "Programming method", NULL},
    {"erasebyte",   'e', 0, G_OPTION_ARG_INT, &erasebyte, "Erase value", NULL},
    {"format",      'f', 0, G_OPTION_ARG_STRING, &format, "Format", NULL},
    {"testmode",    't', 0, G_OPTION_ARG_NONE, &testmode, "Test mode", NULL},
    {"delay",       'd', 0, G_OPTION_ARG_INT, &delayusec, "Programming delay (usec)", NULL},
    {"pulsewidth",  'p', 0, G_OPTION_ARG_INT, &pulsewidth, "Write enable pulse width (usec)", NULL},
    {"linelen",     'l', 0, G_OPTION_ARG_INT, &linelen, "Output line length (bytes)", NULL},
    {"buffersize",  'z', 0, G_OPTION_ARG_INT, &buffersize, "Input buffer size (bytes)", NULL},
    {"device",      'D', 0, G_OPTION_ARG_STRING, &devicetype, "Device type", NULL},
    {NULL}
};

int naddrpins = 0;

uint8_t write_en = 0;
uint8_t output_en = 0;

uint8_t dataport = -1;
uint8_t datadir = -1;
uint8_t addrport = -1;
uint8_t addrdir = -1;

int main(int argc, const char * argv[])
{
    GError *error = NULL;
    GKeyFile *keyfile = g_key_file_new();
    gboolean loaded = g_key_file_load_from_file(keyfile, "/home/pi/.eeprogrc", G_KEY_FILE_NONE, NULL);
    
    int busy = -1;
    GArray *addrpins = g_array_new(FALSE, TRUE, sizeof(uint8_t));
 
 //	TO DO - read in npins and put a loop in...   
    if (loaded) {
/*		addrpins[0] = g_key_file_get_integer(keyfile, "GPIO", "A8", NULL);
		addrpins[1] = g_key_file_get_integer(keyfile, "GPIO", "A9", NULL);
		addrpins[2] = g_key_file_get_integer(keyfile, "GPIO", "A10", NULL);
		addrpins[3] = g_key_file_get_integer(keyfile, "GPIO", "A11", NULL);
		addrpins[4] = g_key_file_get_integer(keyfile, "GPIO", "A12", NULL); */
		
		uint8_t addrline = 8;
		for(naddrpins=0; naddrpins<8; naddrpins++) {
			gchar *str = g_strdup_printf("A%d", addrline++);
			uint8_t pin = g_key_file_get_integer(keyfile, "GPIO", str, NULL);
			if (pin==0) break;
			g_array_append_val(addrpins, pin);
//			g_print("%s %d %d\n", str, pin, addrpins->len);
			g_free(str);
		}
		
		output_en = g_key_file_get_integer(keyfile, "GPIO", "OE", NULL);
		write_en = g_key_file_get_integer(keyfile, "GPIO", "WE", NULL);
		busy = g_key_file_get_integer(keyfile, "GPIO", "BUSY", NULL);
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
	
	if (naddrpins==0) {
		goto finish;
	}

    g_key_file_free(keyfile);
    
    GOptionContext *context = g_option_context_new("- MCP23S17.");
    g_option_context_add_main_entries(context, entries, NULL);

    if (!g_option_context_parse(context, &argc, (gchar ***)&argv, &error))
    {
         g_print("option parsing failed: %s\n", error->message);
         exit(1);
    }
    
    int i;
	if (verbose) {
		puts("EEPROM Programmer v1.0");
		for(i=0; i<naddrpins; i++) {
			g_print("Address pin A%d   = %d\n", i+8, g_array_index(addrpins, uint8_t, i));
		}
		g_print("Write enable pin      = %d\n", write_en);
		g_print("O/P enable pin        = %d\n", output_en);
		g_print("Data GPIO register    = %d\n", dataport);
		g_print("Data DIR register     = %d\n", datadir);
		g_print("Address GPIO register = %d\n", addrport);
		g_print("Address DIR register  = %d\n", addrdir);
		g_print("SPI address           = %d\n", mcpaddr);
        if (devicetype) puts(devicetype);
		putchar('\n');
	}
	
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

    EEProm *eeprom = make_EEPROM(mcp, memsize, naddrpins, write_en, output_en, addrdir, datadir, addrport, dataport, (uint8_t *)addrpins->data);
    if (!eeprom) {
		goto finish;
	}
	
	eeprom->testmode = testmode;
	eeprom->delayusec = delayusec;
	eeprom->busy = busy;
	eeprom->pulsewidth = pulsewidth;

	uint8_t byte = 0x00;
    uint16_t address;
    
    if (interactive) {
		char lnbffr[81];
		address = rombase;
		for(;;) {
			eeprom_readbyte(eeprom, address - rombase, &byte);
//			printf("%04x %02x ", address, byte);
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
        int rectype;
        if (verbose) printf("Programming EEPROM...\n\n");
        for(;!feof(fin);) {
//            int nread = memory_loadFile(memory, fin, rombase, buffersize, memsize, &address);
			unsigned int addr;
			int nread = srec_read(memory, fin, rombase, buffersize, &addr, &rectype);
//			printf("nread = %d rectype = %d\n", nread, rectype);
			address = addr;
            
            if (nread==0 || !rectype) {
                continue;
            }
            
            if (nread==-2) {
                break;
            } else if (nread<0) {
                printf("Read error from memory_loadSRecFile %d.\n", nread);
                goto finish;
            } else {
                eeprom_writebuffer(eeprom, address - rombase, memory, nread, method);
				printf("%8d %04x", nread, address);
				int i;
				for (i=0; i<nread; i++) printf(" %02x", memory[i]);
				putchar('\n');
            
            }
        }
        
        if (fin) fclose(fin);
        free(memory);
    }

    if (readrom) {
        if (verbose) printf("Reading EEPROM... rombase = 0x%04x, memsize = 0x%04x\n\n", rombase, memsize);
        
//	Write header
        if (g_strcmp0(format, "S19")==0) {
			printf("S0090000656570726f6774\n");
		}
        
		uint8_t bytebffr[16];

        for(address = 0; address<memsize; address+=linelen) {
			int i;
			int suppress = TRUE;
//			int suppress = FALSE;
			for(i=0; i<linelen; i++) {
				eeprom_readbyte(eeprom, address + i, &byte);
				if (byte!=0xff) suppress = FALSE;
				bytebffr[i] = byte;
			}
		
			if (suppress) {
				continue;
			} 
		
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
				uint16_t addr = rombase + address;
//				printf("%04x %02x %02x\n", addr, addr>>8 & 0xff, addr & 0xff);
				uint8_t checksum = /*0x13*/ linelen + 3 + (addr & 0xff) + (addr>>8 & 0xff);
				printf("S1%02x%04x", linelen + 3, addr);
				for(i=0; i<linelen; i++) {
					byte = bytebffr[i];
					printf("%02x", byte);
					checksum += byte;
				}
				g_print("%02x\n", (uint8_t)(~checksum));
			}
		}

//	TO DO - record count		
		if (g_strcmp0(format, "S19")==0) {
//			puts("S5030006F6")
			puts("S9030000FC");
		}
	}

finish:
    g_option_context_free(context);
    if (eeprom) eeprom_free(eeprom);

    return 0;
}
