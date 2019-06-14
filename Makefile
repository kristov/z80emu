CC := gcc
CFLAGS := -Wall -Werror -ggdb

LDFLAGS=-lncursesw
INCLUDE=-Iexternal/z80ex/include/

OBJECTS =
OBJECTS += external/z80ex/z80ex.o
OBJECTS += external/z80ex/z80ex_dasm.o
OBJECTS += mem_win.o

z80emu: z80emu.c $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) -o $@ $(OBJECTS) $<

external/z80ex/z80ex.o:
	cd external/z80ex/ && make static

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

clone-emulator:
	mkdir -p external/
	cd external/ && git clone https://github.com/lipro/z80ex.git

clean:
	rm -f z80emu
	cd external/z80ex/ && make clean
