#include "srecord.h"

#ifndef BUFFLEN
#define BUFFLEN 512			//	256
#endif

int memory_loadSRecFile(uint8_t *memory, FILE *fin, unsigned int rombase, unsigned int bffrsize, unsigned int memsize,
                        uint16_t *address_out)
{
    char linebuffer[BUFFLEN];
    int errorflag = 0;

	char *p = fgets(linebuffer, BUFFLEN, fin);
	if (p==NULL) {
//		puts("Could not read line");
		return -2;
	}
	
	if (linebuffer[0]!='S') {
		errorflag = -1;
//		puts("SRecord does not start with S");
		return -1;
	}
	
	p++;
	int recordtype = 0;
	int n = sscanf(p, "%1x", &recordtype);
	p += 1;
	int count = 0;
	n = sscanf(p, "%2x", &count);
	p += 2;
	
//	printf("Got record of type %d count = %d\n", recordtype, count);
	
	unsigned int address = 0;
	if (recordtype==0) {
		return 0;
	} else if (recordtype==1) {
		n = sscanf(p, "%4x", &address);
		p += 4;
		count -= 2;
    } else if (recordtype==5) {
		return 0;
	} else if (recordtype==2 || recordtype==7) {
		n = sscanf(p, "%6x", &address);
		p += 6;
	} else if (recordtype==3 || recordtype==8) {
		n = sscanf(p, "%8x", &address);
		p += 8;
	} else if (recordtype==5) {
		return 0;
	} else if (recordtype==9) {
//		n = sscanf(p, "%4x", &startaddress);
		p += 4;
		count -= 2;
		return 0;
	} else {
		puts("Invalid SRecord type");
		return -1;
	}
	
	unsigned int value;
	int nread = 0;
	int i;
	switch (recordtype) {
		case 0:
			break;
		case 1:
		case 3:
			*address_out = address;
	
			for (i=0; i<count-1; i++) {
				n = sscanf(p, "%2x", &value);
				if (address>=rombase && (address - rombase)<memsize && i<bffrsize)
				{
					memory[i] = value;
					nread++;
				}

//				nread++;
				address++;
				p += 2;
			}
			
			break;
		case 5:
		case 7:
		case 8:
			nread = 1;
			break;
		default:
			return -1;
			break;
	}
	

    return nread;
}

