#include "srecord.h"
#include <string.h>

static unsigned int gethex(FILE *f, unsigned int n, unsigned int *success)
{
    int d = 0;
    unsigned int nread = 0;
    
    int i;
    for(i=0; i<n; i++) {
        char ch = 0;
        uint8_t dgt = 0;
        unsigned long n = fread(&ch, 1, 1, f);
        if (!n) break;
        if (ch>='a' && ch<='z') ch -= 'a' - 'A';
        if (ch>='A' && ch<='F') {
            dgt = ch - 'A' + 10;
        } else if (ch>='0' && ch<='9') {
            dgt = ch - '0';
        } else {
            break;
        }
        d = (d<<4) + dgt;
        nread++;
    }
    
    *success = nread==n;
    
    return d;
}

int srec_read(uint8_t *memory, FILE *f, unsigned int rombase, unsigned int bffrsize, unsigned int *address_out, int *rectype)
{
    int nread = 0;
    char ch = 0;
    unsigned long n = 0;
    unsigned int success = 0;
    
    do {
        n = fread(&ch, 1, 1, f);
    } while (n>0 && ch<' ');
    
    if (n==0) return -2;
    if (ch!='S') return -1;
    
    char recordtype = 0;
    n = fread(&recordtype, 1, 1, f);
    *rectype = 0;
    int count = gethex(f, 2, &success);
    if (!success) return -1;
    uint8_t checksum = count;

    unsigned int address = 0;
    if (recordtype=='0') {
        address = gethex(f, 4, &success);
        count -= 2;
    } else if (recordtype=='1') {
        address = gethex(f, 4, &success);
        count -= 2;
        *rectype = 1;
    } else if (recordtype=='5') {
        return 0;
    } else if (recordtype=='2' || recordtype=='7') {
    } else if (recordtype=='3' || recordtype=='8') {
    } else if (recordtype=='5') {
        return 0;
    } else if (recordtype=='9') {
        count -= 2;
        return 0;
    } else {
        puts("Invalid SRecord type");
        return -1;
    }
    if (!success) return -1;
    
//    printf("recordtype = %c count = %d address = %04x\n", recordtype, count, address);

    checksum += (address>>8) + (address & 0x0f);

    unsigned int value;
    int i;
    switch (recordtype) {
        case '1':
        case '3':
            if (address_out) *address_out = address;
    
        case '0':
            for (i=0; i<count-1; i++) {
                value = gethex(f, 2, &success);
//                printf("Read %02x\n", value);
                if (address>=rombase && i<bffrsize)
                {
                    memory[i] = value;
                    checksum += value;
                    nread++;
                }

            }
            
            value = gethex(f, 2, &success);
//            if (checksum + value!=0xff) {
//                puts("Checksum error.");
//            }
            
            break;
        case '5':
        case '7':
        case '8':
            nread = 1;
            break;
        default:
            return -1;
            break;
    }
    return nread;
}

int ihex_read(uint8_t *memory, FILE *f, unsigned int rombase, unsigned int bffrsize, unsigned int *address_out, int *rectype)
{
    int nread = 0;
    char ch = 0;
    unsigned long n = 0;
    unsigned int success = 0;
    *rectype = 0;

//  Skip control characters
    do {
        n = fread(&ch, 1, 1, f);
    } while (n>0 && ch<' ');

//  Check for valid start of line.
    if (n==0) return -2;
    if (ch!=':') return -1;
    
    int bytecount = gethex(f, 2, &success);
    unsigned int address = gethex(f, 4, &success);
    unsigned int recordtype = gethex(f, 2, &success);
    uint8_t checksum = bytecount + (address>>0) + (address & 0xff) + (recordtype & 0xff);
    
    switch (recordtype) {
        case 0x00:
            if (address_out) *address_out = address;
            *rectype = 1;
            break;
            
        default:
            break;
    }
    
    uint8_t value;
    int i;
    for(i=0; i<bytecount; i++) {
        value = gethex(f, 2, &success);
        if (address>=rombase && i<bffrsize)
        {
            memory[i] = value;
            nread++;
        }
        checksum += value;
    }
    
    checksum += gethex(f, 2, &success);
    
    return nread;
}

int memory_loadFile(uint8_t *memory, FILE *fin, unsigned int rombase, unsigned int bffrsize, 
						unsigned int memsize, uint16_t *address_out)
{
	uint8_t buffer[256];
	unsigned int address = rombase;
	int rectype;
	unsigned int nread = 0;
	int n;
	
	for(;;) {
		n = srec_read(buffer, fin, rombase, 256, &address, &rectype);
		if (n==-1) break;
		if (n==-2 || rectype!='1') continue;
		
		memcpy(memory + address - rombase, buffer, n);
		
		nread += n;
	}
	
    return nread;
}

