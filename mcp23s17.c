#include "mcp23s17.h"
#include <stdio.h>

void mcp23s17_writebyte(MCP23s17 *mcp, uint8_t reg, uint8_t byte)
{
	mcp->buffer[0] = 0x40 | ((mcp->address<<1) & 0x0f);
//	mcp->buffer[0] &= 0xfe;
	mcp->buffer[1] = reg;
	mcp->buffer[2] = byte;

//	printf("mcp23s17_writebyte: buffer = %02x %02x %02x\n", mcp->buffer[0], mcp->buffer[1], mcp->buffer[2]);
	bcm2835_spi_transfern(mcp->buffer, 3);
//	printf("mcp23s17_writebyte: buffer = %02x %02x %02x\n", mcp->buffer[0], mcp->buffer[1], mcp->buffer[2]);
	
}

void mcp23s17_writebyte2(MCP23s17 *mcp, uint8_t reg, uint8_t byte1, uint8_t byte2)
{
	mcp->buffer[0] &= 0xfe;
	mcp->buffer[1] = reg;
	mcp->buffer[2] = byte1;
	mcp->buffer[3] = byte2;

//	printf("mcp23s17_writebyte2: buffer = %02x %02x %02x %02x\n", mcp->buffer[0], mcp->buffer[1], mcp->buffer[2], mcp->buffer[3]);
	bcm2835_spi_transfern(mcp->buffer, 4);
//	bcm2835_spi_transfern(mcp->buffer, 3);
	
}

void mcp23s17_readbyte(MCP23s17 *mcp, uint8_t reg, uint8_t *byte)
{
	mcp->buffer[0] = 0x41 | ((mcp->address<<1) & 0x0f);
//	mcp->buffer[0] |= 0x01;
	mcp->buffer[1] = reg;
	mcp->buffer[2] = 0;

//	printf("mcp23s17_readbyte: buffer = %02x %02x %02x\n", mcp->buffer[0], mcp->buffer[1], mcp->buffer[2]);
	bcm2835_spi_transfern(mcp->buffer, 3);
//	printf("mcp23s17_readbyte: buffer = %02x %02x %02x\n", mcp->buffer[0], mcp->buffer[1], mcp->buffer[2]);
	
	*byte = mcp->buffer[2];
	
}

MCP23s17 *mcp23s17_new(int address)
{
	MCP23s17 *mcp = calloc(sizeof(MCP23s17), 1);
	if (!mcp) return NULL;
	
	mcp->address = address & 0x07;
//	mcp->buffer[0] = 0x40 | ((mcp->address<<1) & 0x0f);
//	printf("mcp23s17_new: buffer[0] = %02x\n", mcp->buffer[0]);
	
	mcp23s17_writebyte(mcp, IOCON, 0b00001000);

	return mcp;
}

void mcp23s17_free(MCP23s17 *mcp)
{
	free(mcp);
}

