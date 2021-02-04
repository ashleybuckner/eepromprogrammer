#include <bcm2835.h>

#define IODIRA 0x00
#define IODIRB 0x01
#define IOCON 0x0a
#define GPIOA 0x12
#define GPIOB 0x13
#define OLATA 0x14
#define OLATB 0x15

struct _mcp23s17 {
	int address;
	uint8_t buffer[4];
};
typedef struct _mcp23s17 MCP23s17;

void mcp23s17_writebyte(MCP23s17 *mcp, uint8_t reg, uint8_t byte);
void mcp23s17_readbyte(MCP23s17 *mcp, uint8_t reg, uint8_t *byte);
MCP23s17 *mcp23s17_new(int address);
void mcp23s17_free(MCP23s17 *mcp);
