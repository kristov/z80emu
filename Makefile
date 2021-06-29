CC := gcc
CFLAGS := -Wall -Werror -ggdb

Z80EX=../z80ex
INCLUDE=-I$(Z80EX)/include/ $(shell pkg-bee --cflags ctk)

OBJECTS =
OBJECTS += $(Z80EX)/z80ex.o
OBJECTS += $(Z80EX)/z80ex_dasm.o
OBJECTS += reg_win.o
OBJECTS += mem_win.o
OBJECTS += asm_win.o

all: z80emu

z80emu: z80emu.c $(OBJECTS)
	$(CC) $(CFLAGS) -I$(Z80EX)/include/ -o $@ $(OBJECTS) $< $(shell pkg-bee --cflags --libs ctk)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(shell pkg-bee --cflags ctk) -c -o $@ $<

clean:
	rm -f z80emu reg_win.o mem_win.o asm_win.o
