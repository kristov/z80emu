#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <locale.h>
#include "z80ex.h"

typedef struct cpu_reg {
    uint16_t PC;
    uint8_t A;
    uint8_t F;
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t H;
    uint8_t L;
    uint8_t I;
    uint8_t R;
    uint16_t SP;
    uint16_t IX;
    uint16_t IY;
} cpu_reg_t;

static void cleanup_view(int sig) {
    endwin();
    exit(0);
}

void init_view() {
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

    noecho();
    cbreak();
    timeout(1000);
    curs_set(FALSE);
}

void draw_reg_win(uint8_t x, uint8_t y) {
    attron(COLOR_PAIR(2));
    mvaddstr(y, x, "┌───────────────────────────────┐"); y++;
    mvaddstr(y, x, "│PC            [program counter]│"); y++;
    mvaddstr(y, x, "│00000 - 0000 - 0000000000000000│"); y++;
    mvaddstr(y, x, "├───────────────┬───────────────┤"); y++;
    mvaddstr(y, x, "│A [accumulator]│F       [flags]│"); y++;
    mvaddstr(y, x, "│000 00 00000000│000 00 00000000│"); y++;
    mvaddstr(y, x, "├───────────────┼───────────────┤"); y++;
    mvaddstr(y, x, "│B     [counter]│C        [port]│"); y++;
    mvaddstr(y, x, "│000 00 00000000│000 00 00000000│"); y++;
    mvaddstr(y, x, "├───────────────┼───────────────┤"); y++;
    mvaddstr(y, x, "│D              │E              │"); y++;
    mvaddstr(y, x, "│000 00 00000000│000 00 00000000│"); y++;
    mvaddstr(y, x, "├───────────────┼───────────────┤"); y++;
    mvaddstr(y, x, "│H              │L              │"); y++;
    mvaddstr(y, x, "│000 00 00000000│000 00 00000000│"); y++;
    mvaddstr(y, x, "├───────────────┼───────────────┤"); y++;
    mvaddstr(y, x, "│I   [interrupt]│R     [refresh]│"); y++;
    mvaddstr(y, x, "│000 00 00000000│000 00 00000000│"); y++;
    mvaddstr(y, x, "├───────────────┴───────────────┤"); y++;
    mvaddstr(y, x, "│SP              [stack pointer]│"); y++;
    mvaddstr(y, x, "│00000 - 0000 - 0000000000000000│"); y++;
    mvaddstr(y, x, "├───────────────────────────────┤"); y++;
    mvaddstr(y, x, "│IX            IXH      IXL     │"); y++;
    mvaddstr(y, x, "│00000  00 00  00000000 00000000│"); y++;
    mvaddstr(y, x, "├───────────────────────────────┤"); y++;
    mvaddstr(y, x, "│IY            IYH      IYL     │"); y++;
    mvaddstr(y, x, "│00000  00 00  00000000 00000000│"); y++;
    mvaddstr(y, x, "└───────────────────────────────┘"); y++;
    attroff(COLOR_PAIR(2));
}

void debug_message(char* message) {
    attron(COLOR_PAIR(3));
    mvaddstr(28, 1, message);
    attroff(COLOR_PAIR(3));
}

void execute_instruction(Z80EX_CONTEXT *cpu) {
    int t_states;
    t_states = z80ex_step(cpu);
}

void refresh_view() {
    draw_reg_win(0, 0);
    refresh();
}

// BEGIN Callbacks
Z80EX_BYTE mem_read(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *z80emu) {
    char message[100];
    sprintf(message, "memory read: address[%016x]", addr);
    debug_message(message);
    return 0;
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

int main(int argc, char *argv[]) {
    cpu_reg_t regs;
    Z80EX_CONTEXT *cpu;
    char c;

    cpu = z80ex_create(mem_read, &regs, mem_write, &regs, port_read, &regs, port_write, &regs, int_read, &regs);

    init_view();
    refresh_view();

    while (1) {
        c = getch();
        if (ERR != c) {
            mvaddch(10, 10, c);
            refresh();
        }
        execute_instruction(cpu);
        refresh_view();
    }

    z80ex_destroy(cpu);
    cleanup_view(1);
}
