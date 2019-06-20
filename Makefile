CC := gcc
CFLAGS := -Wall -Werror -ggdb

LDFLAGS=-lncursesw
INCLUDE=-Iexternal/z80ex/include/ -Iexternal/ctk/

OBJECTS =
OBJECTS += external/z80ex/z80ex.o
OBJECTS += external/z80ex/z80ex_dasm.o
OBJECTS += external/ctk/ctk.o
OBJECTS += reg_win.o
OBJECTS += mem_win.o
OBJECTS += asm_win.o

z80emu: z80emu.c $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) -o $@ $(OBJECTS) $<

external/z80ex/z80ex.o:
	cd external/z80ex/ && make static

#external/ctk/ctk.o:
#	cd external/ctk/ && make

%.o: %.c %.h
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clone-emulator:
	mkdir -p external/
	cd external/ && git clone https://github.com/lipro/z80ex.git

clone-ctk:
	mkdir -p external/
	cd external/ && git clone git@github.com:kristov/ctk.git

clean:
	rm -f z80emu $(OBJECTS)
	cd external/z80ex/ && make clean

#test_ctk: test_ctk.c external/ctk/ctk.o
#	$(CC) $(CFLAGS) $(LDFLAGS) -Iexternal/ctk/ -o $@ external/ctk/ctk.o $<
