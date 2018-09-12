#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <locale.h>
#include "z80ex.h"

typedef struct z80emu {
    FILE* rom_fh;
    uint8_t* memory;
    Z80EX_CONTEXT *cpu;
    WINDOW* mnv_nwin;
    WINDOW* rgv_nwin;
    WINDOW* mcv_nwin;
    uint8_t pause;
} z80emu_t;

static void cleanup_view(int sig) {
    endwin();
    exit(0);
}

void cleanup_context(z80emu_t* z80emu) {
    if (z80emu->memory != NULL) {
        free(z80emu->memory);
    }
    delwin(z80emu->rgv_nwin);
    delwin(z80emu->mcv_nwin);
    z80ex_destroy(z80emu->cpu);
}

void init_view(z80emu_t* z80emu) {
    setlocale(LC_ALL, "");
    signal(SIGINT, cleanup_view);
    initscr();
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW,COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLACK);
    }

    z80emu->rgv_nwin = newwin(31, 33, 0, 0);
    z80emu->mcv_nwin = newwin(10, 30, 0, 39);

    box(z80emu->rgv_nwin, 0, 0);
    //mvwin(z80emu->mcv_nwin, 0, 39);
    //wbkgd(z80emu->mnv_nwin, COLOR_PAIR(7));
    wbkgd(z80emu->rgv_nwin, COLOR_PAIR(3));

    noecho();
    cbreak();
    timeout(1000);
    curs_set(FALSE);
}

void draw_reg_win(z80emu_t* z80emu) {
    int x, y;

    x = 1;
    y = 1;
    wattron(z80emu->rgv_nwin, COLOR_PAIR(2));
    mvwaddstr(z80emu->rgv_nwin, y, x, "PC            [program counter]"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "───────────────┬───────────────"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "A [accumulator]│F       [flags]"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "───────────────┼───────────────"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "B     [counter]│C        [port]"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "───────────────┼───────────────"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "D              │E              "); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "───────────────┼───────────────"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "H              │L              "); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "───────────────┴───────────────"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "I                   [interrupt]"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "───────────────────────────────"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "R                     [refresh]"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "───────────────────────────────"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "SP              [stack pointer]"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "───────────────────────────────"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "IX            IXH      IXL     "); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "───────────────────────────────"); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "IY            IYH      IYL     "); y++;
    mvwaddstr(z80emu->rgv_nwin, y, x, "00000 - 0000 - 0000000000000000"); y++;
    wattroff(z80emu->rgv_nwin, COLOR_PAIR(2));
}

void draw_reg_8(z80emu_t* z80emu, uint8_t x, uint8_t y, uint8_t value) {
    char dec[4];
    char hex[3];
    char bin[9];
    uint8_t i;

    dec[3] = '\0';
    hex[2] = '\0';
    bin[8] = '\0';

    sprintf(dec, "%3d", value);
    sprintf(hex, "%02x", value);

    for (i = 0; i < 8; i++) {
        if ((value >> i) & 1) {
            bin[7 - i] = '1';
        }
        else {
            bin[7 - i] = '0';
        }
    }

    mvwaddstr(z80emu->rgv_nwin, y, x, dec); x += 4;
    mvwaddstr(z80emu->rgv_nwin, y, x, hex); x += 3;

    wattron(z80emu->rgv_nwin, COLOR_PAIR(4));
    mvwaddstr(z80emu->rgv_nwin, y, x, bin);
    wattroff(z80emu->rgv_nwin, COLOR_PAIR(4));
}

void draw_reg_16(z80emu_t* z80emu, uint8_t x, uint8_t y, uint16_t value) {
    char dec[6];
    char hex[5];
    char bin[17];
    uint8_t i;

    dec[5] = '\0';
    hex[4] = '\0';
    bin[16] = '\0';

    sprintf(dec, "%5d", value);
    sprintf(hex, "%04x", value);

    for (i = 0; i < 16; i++) {
        if ((value >> i) & 1) {
            bin[15 - i] = '1';
        }
        else {
            bin[15 - i] = '0';
        }
    }

    mvwaddstr(z80emu->rgv_nwin, y, x, dec); x += 8;
    mvwaddstr(z80emu->rgv_nwin, y, x, hex); x += 7;
    mvwaddstr(z80emu->rgv_nwin, y, x, bin);
}

void draw_reg_16_as_two_8(z80emu_t* z80emu, uint8_t x, uint8_t y, uint16_t value) {
    uint8_t lower;
    uint8_t higher;

    lower = (uint8_t)(value & 0xFF);
    higher = (uint8_t)(value >> 8);

    draw_reg_8(z80emu, x, y, higher);
    draw_reg_8(z80emu, x + 16, y, lower);
}

void draw_registers(z80emu_t* z80emu) {
    Z80EX_WORD reg_word;

    reg_word = z80ex_get_reg(z80emu->cpu, regPC);
    draw_reg_16(z80emu, 1, 2, reg_word);

    reg_word = z80ex_get_reg(z80emu->cpu, regAF);
    draw_reg_16_as_two_8(z80emu, 1, 5, reg_word);

    reg_word = z80ex_get_reg(z80emu->cpu, regBC);
    draw_reg_16_as_two_8(z80emu, 1, 8, reg_word);

    reg_word = z80ex_get_reg(z80emu->cpu, regDE);
    draw_reg_16_as_two_8(z80emu, 1, 11, reg_word);

    reg_word = z80ex_get_reg(z80emu->cpu, regHL);
    draw_reg_16_as_two_8(z80emu, 1, 14, reg_word);

    reg_word = z80ex_get_reg(z80emu->cpu, regI);
    draw_reg_16(z80emu, 1, 17, reg_word);

    reg_word = z80ex_get_reg(z80emu->cpu, regR);
    draw_reg_16(z80emu, 1, 20, reg_word);
}

void debug_message(char* message) {
    attron(COLOR_PAIR(3));
    mvaddstr(28, 1, message);
    attroff(COLOR_PAIR(3));
}

void refresh_view(z80emu_t* z80emu) {
    draw_reg_win(z80emu);
    draw_registers(z80emu);
    refresh();
    wrefresh(z80emu->rgv_nwin);
}

void execute_instruction(Z80EX_CONTEXT *cpu) {
    int t_states;
    t_states = z80ex_step(cpu);
}

// BEGIN Callbacks
Z80EX_BYTE mem_read(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *z80emu) {
    char message[101];
    message[100] = '\0';
    sprintf(message, "memory read: address[%016x]", addr);
    debug_message(message);
    return 4;
}

void mem_write(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *z80emu) {
    printf("memory write: address[%016x] data[%08x]\n", addr, value);
}

Z80EX_BYTE port_read(Z80EX_CONTEXT *cpu, Z80EX_WORD port, void *z80emu) {
    printf("port read: address[%016x]\n", port);
    return 0;
}

void port_write(Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value, void *z80emu) {
    printf("port write: address[%016x] data[%08x]\n", port, value);
}

Z80EX_BYTE int_read(Z80EX_CONTEXT *cpu, void *z80emu) {
    printf("interrupt vector!\n");
    return 0;
}
// END Calbacks

void main_program(z80emu_t* z80emu) {
    char c;
    uint8_t pause;

    z80emu->cpu = z80ex_create(mem_read, z80emu, mem_write, z80emu, port_read, z80emu, port_write, z80emu, int_read, z80emu);

    init_view(z80emu);
    refresh_view(z80emu);

    pause = 1;

    while (1) {
        c = getch();
        if (ERR != c) {
            pause = 1;
            switch (c) {
                case ' ':
                    pause = !pause;
                    break;
                case 'q':
                    cleanup_context(z80emu);
                    cleanup_view(1);
                    exit(0);
                    break;
                default:
                    break;
            }
        }
        if (!pause) {
            execute_instruction(z80emu->cpu);
            refresh_view(z80emu);
        }
    }

    cleanup_context(z80emu);
    cleanup_view(1);
}

void load_binary_rom(z80emu_t* z80emu, char* rom_file) {
    unsigned long len;

    z80emu->rom_fh = fopen(rom_file, "rb");
    if (z80emu->rom_fh == NULL) {
        printf("Could not open rom file\n");
        exit(1);
    }

    fseek(z80emu->rom_fh, 0, SEEK_END);
    len = ftell(z80emu->rom_fh);
    rewind(z80emu->rom_fh);

    if (len > 65536) {
        len = 65536;
    }
    fread(z80emu->memory, len, 1, z80emu->rom_fh);

    fclose(z80emu->rom_fh);
}

void print_help() {
    printf("Usage: z80emu [-r<binary rom file>]\n");
}

void init_z80emu(z80emu_t* z80emu) {
    memset(z80emu, 0, sizeof(z80emu_t));
    z80emu->memory = malloc(sizeof(uint8_t) * 65536);
    if (z80emu->memory == NULL) {
        return;
    }
    memset(z80emu->memory, 0, sizeof(uint8_t) * 65536);
}

int main(int argc, char *argv[]) {
    z80emu_t z80emu;
    char* rom_file;
    int8_t c;
	int option_index = 0;

    init_z80emu(&z80emu);

	static struct option long_options[] = {
        {"rom", optional_argument, 0, 'r'},
        {0, 0, 0, 0}
    };

    while (1) {
		c = getopt_long(argc, argv, "r:", long_options, &option_index);
		if (c == -1) {
			break;
		}
        switch (c) {
            case 'r':
                load_binary_rom(&z80emu, optarg);
                break;
            default:
                print_help();
                exit(1);
        }
    }

    main_program(&z80emu);
    return 0;
}
