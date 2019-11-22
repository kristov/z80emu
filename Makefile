CC := gcc
CFLAGS := -Wall -Werror -ggdb

LDFLAGS=-lncursesw
INCLUDE=-Iz80ex/include/ -Ictk/

OBJECTS =
OBJECTS += z80ex/z80ex.o
OBJECTS += z80ex/z80ex_dasm.o
OBJECTS += ctk/ctk.o
OBJECTS += reg_win.o
OBJECTS += mem_win.o
OBJECTS += asm_win.o

z80emu: z80emu.c $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) -o $@ $(OBJECTS) $<

z80ex/z80ex.o:
	mkdir -p z80ex/lib
	cd z80ex/ && make

ctk/ctk.o:
	cd ctk/ && make

%.o: %.c %.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f z80emu reg_win.o mem_win.o asm_win.o
	cd z80ex/ && make clean
	cd ctk/ && make clean
