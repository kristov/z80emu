LDFLAGS=-lncursesw
INCLUDE=-Iexternal/z80ex/include/
z80emu: src/z80emu.c external/z80ex/z80ex.o
	gcc $(INCLUDE) $(LDFLAGS) -o $@ external/z80ex/z80ex.o $<

external/z80ex/z80ex.o:
	cd external/z80ex/ && make static

clone-emulator:
	mkdir -p external/
	cd external/ && git clone https://github.com/lipro/z80ex.git

clean:
	rm -f z80emu
	cd external/z80ex/ && make clean
