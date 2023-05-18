#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "eeprom.h"

void eeprom_setaddress(EEProm *eeprom, uint16_t address)
{
    mcp23s17_writebyte(eeprom->mcp, eeprom->addrport, address & 0x00ff);
    
    address >>= 8;
    int i;
    for(i=0; i<eeprom->naddrpins; i++) {
//        printf("Setting pin %02d to %1d\n", addrpins[i], address & 0x01);
        bcm2835_gpio_write(eeprom->addrpins[i], address & 0x01);
        address >>= 1;
    }
}

int eeprom_writebyte(EEProm *eeprom, uint16_t address, uint8_t byte, int method)
{
	eeprom_setaddress(eeprom, address);

	if (eeprom->testmode) {
		printf("Writing %02x to %04x\n", byte, address);
	} else {
		if (method==2 && eeprom->busy>0) {
		}
		
		bcm2835_gpio_write(eeprom->output_en, HIGH);                    // Disable EEPROM output
		mcp23s17_writebyte(eeprom->mcp, eeprom->datadir, 0x00);         // Set 23s17 port A (data pins) to be outputs
		mcp23s17_writebyte(eeprom->mcp, eeprom->dataport, byte);        // Write next byte
		bcm2835_gpio_write(eeprom->write_en, LOW);                      // Pulse write enable low
//		usleep(1);                                              		// Longer than a microsecond...???
		bcm2835_delayMicroseconds(eeprom->pulsewidth);
		bcm2835_gpio_write(eeprom->write_en, HIGH);
		
		if (method==1) {
			mcp23s17_writebyte(eeprom->mcp, eeprom->datadir, 0xff);
			uint8_t readbyte;
			do {
				mcp23s17_readbyte(eeprom->mcp, eeprom->dataport, &readbyte);
			} while (byte!=readbyte);
		} else {
//			usleep(eeprom->delayusec);
			bcm2835_delayMicroseconds(eeprom->delayusec);
		}
		
		if (method==2 && eeprom->busy>0) {
		}
	}

	return 0;
}

int eeprom_readbyte(EEProm *eeprom, uint16_t address, uint8_t *byte)
{
	eeprom_setaddress(eeprom, address);
	
	bcm2835_gpio_write(eeprom->output_en, HIGH);                    // Disable EEPROM output
	mcp23s17_writebyte(eeprom->mcp, eeprom->datadir, 0xff);                    // Set 23s17 port A (data pins) to be inputs
	bcm2835_gpio_write(eeprom->output_en, LOW);                        // Enable EEPROM data outputs
	mcp23s17_readbyte(eeprom->mcp, eeprom->dataport, byte);                    // Read 23s17 data pins
	bcm2835_gpio_write(eeprom->output_en, HIGH);                    // Disable EEPROM output

	return 0;
}

int eeprom_writebuffer(EEProm *eeprom, uint16_t address, uint8_t *bytes, uint16_t nbytes, int method)
{
    if (address<0 || address>=eeprom->memsize) {
		printf("\nAddress out of bounds %04x\n", address);
		return -1;
	}
	
	int n;
	for(n=0; n<nbytes; n++) {
		uint8_t byte = bytes[n];
//		printf(" %02x", byte);

		eeprom_writebyte(eeprom, address, byte, method);
		address++;
	}

	return 0;
}

EEProm *make_EEPROM(MCP23s17 *mcp, unsigned int memsize, int naddrpins, int write_en, int output_en,
					uint8_t addrdir, uint8_t datadir, uint8_t addrport, uint8_t dataport, 
					uint8_t buffer_en, uint8_t *addrpins)
{
	int i;
	
	for(i=0; i<naddrpins; i++) {
		if (addrpins[i]<=0) return NULL;
	}
	
	if (write_en<=0 || output_en<=0) return NULL;
	
	EEProm *eeprom = calloc(1, sizeof(EEProm));
	
	eeprom->mcp = mcp;
	eeprom->memsize = memsize;
	
	eeprom->naddrpins = naddrpins;
	eeprom->addrpins = calloc(naddrpins, sizeof(int));
	
	eeprom->addrdir = addrdir;
	eeprom->datadir = datadir;
	eeprom->dataport = dataport;
	eeprom->addrport = addrport;
	
	eeprom->pulsewidth = 1;
	eeprom->busy = -1;
	
	for(i=0; i<naddrpins; i++) {
		eeprom->addrpins[i] = addrpins[i];
	}	
	
	eeprom->write_en = write_en;	
	eeprom->output_en = output_en;
	eeprom->buffer_en = buffer_en;

	eeprom->testmode = 0;

	eeprom->delayusec = 10000;
	
    for(i=0; i<naddrpins; i++) {
		bcm2835_gpio_fsel(eeprom->addrpins[i], BCM2835_GPIO_FSEL_OUTP);
		bcm2835_gpio_write(eeprom->addrpins[i], LOW);
    }
    bcm2835_gpio_fsel(eeprom->write_en, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(eeprom->write_en, HIGH);
    bcm2835_gpio_fsel(eeprom->output_en, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(eeprom->output_en, HIGH);
    bcm2835_gpio_fsel(eeprom->buffer_en, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(eeprom->buffer_en, HIGH);

//    1 => input
//    0 => output
    mcp23s17_writebyte(eeprom->mcp, addrdir, 0x00);                        // Port B (lhs): address pins
    mcp23s17_writebyte(eeprom->mcp, datadir, 0xff);                        // Port A (rhs): data pins

	return eeprom;
}

void eeprom_free(EEProm *eeprom)
{
	free(eeprom->addrpins);
    mcp23s17_free(eeprom->mcp);
    free(eeprom);
}

