#include "mcp23s17.h"

struct _EEPROM {
	MCP23s17 *mcp;
	int naddrpins;
	uint8_t *addrpins;
	uint8_t write_en;
	uint8_t output_en;
	uint8_t dataport;
	uint8_t datadir;
	uint8_t addrport;
	uint8_t addrdir;
	int testmode;
	int delayusec;
	unsigned int memsize;
};
typedef struct _EEPROM EEProm;

EEProm *make_EEPROM(MCP23s17 *mcp, unsigned int memsize, int naddrpins, int write_en, int output_en,
					uint8_t addrdir, uint8_t datadir, uint8_t addrport, uint8_t dataport, uint8_t *addrpins);
void eeprom_free(EEProm *eeprom);

void eeprom_setaddress(EEProm *eeprom, uint16_t address);
int eeprom_writebyte(EEProm *eeprom, uint16_t address, uint8_t byte, int method);
int eeprom_readbyte(EEProm *eeprom, uint16_t address, uint8_t *byte);

int eeprom_writebuffer(EEProm *eeprom, uint16_t address, uint8_t *bytes, uint16_t nbytes, int method);
