EXE=eeprog
OBJS=main.o eeprom.o mcp23s17.o srecord.o
CFLAGS=-I/usr/local/include `pkg-config --cflags glib-2.0` -g -DBREADBOARD
LIBS=-L/usr/local/lib/ `pkg-config --libs glib-2.0` -lbcm2835
CC=gcc

all:$(EXE)

$(EXE):$(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)
	
