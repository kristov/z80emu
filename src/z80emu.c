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
#include "z80ex_dasm.h"

// Which sub-window has focus
enum win_mode {
    MODE_REG,
    MODE_ASM,
    MODE_MEM
};
#define MODE_ZERO 0x00
#define MODE_MAX 0x02

// An instance of an emulator UI
typedef struct z80emu {
    FILE* rom_fh;
    uint8_t* memory;
    Z80EX_CONTEXT *cpu;
    uint16_t pc_before;
    uint16_t pc_after;
    enum win_mode win_mode;
    WINDOW* reg_win;
    WINDOW* mem_win;
    WINDOW* msg_win;
    WINDOW* asm_win;
    uint8_t reg_width;
    uint8_t reg_height;
    uint8_t mem_width;
    uint8_t mem_height;
    uint8_t msg_width;
    uint8_t msg_height;
    uint8_t asm_width;
    uint8_t asm_height;
    uint8_t pause;
    char msg[255];
} z80emu_t;

// Only one emulation happening at a time
z80emu_t Z80EMU;

// Z80EX-Callback for a CPU memory read
Z80EX_BYTE mem_read(Z80EX_CONTEXT* cpu, Z80EX_WORD addr, int m1_state, void* user_data) {
    z80emu_t* z80emu;
    z80emu = user_data;
    return z80emu->memory[(uint16_t)addr];
}

// Z80EX-Callback for a CPU memory write
void mem_write(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *z80emu) {
    //printf("memory write: address[%016x] data[%08x]\n", addr, value);
}

// Z80EX-Callback for a CPU port read
Z80EX_BYTE port_read(Z80EX_CONTEXT *cpu, Z80EX_WORD port, void *z80emu) {
    //printf("port read: address[%016x]\n", port);
    return 0;
}

// Z80EX-Callback for a CPU port write
void port_write(Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value, void *z80emu) {
    //printf("port write: address[%016x] data[%08x]\n", port, value);
}

// Z80EX-Callback for an interrupt read
Z80EX_BYTE int_read(Z80EX_CONTEXT *cpu, void *z80emu) {
    //printf("interrupt vector!\n");
    return 0;
}

// Z80EX-Callback for DASM memory read
Z80EX_BYTE mem_read_dasm(Z80EX_WORD addr, void *user_data) {
    z80emu_t* z80emu;
    z80emu = user_data;
    return z80emu->memory[(uint16_t)addr];
}

// Print a debug message in the message window
void debug_message(z80emu_t* z80emu, char* message) {
    wattron(z80emu->msg_win, COLOR_PAIR(3));
    mvwaddstr(z80emu->msg_win, 1, 1, message);
    wattroff(z80emu->msg_win, COLOR_PAIR(3));
}

// Destroy a z80emu instance
void cleanup_context(z80emu_t* z80emu) {
    if (z80emu->memory != NULL) {
        free(z80emu->memory);
    }
    delwin(z80emu->reg_win);
    delwin(z80emu->mem_win);
    delwin(z80emu->msg_win);
    delwin(z80emu->asm_win);
    z80ex_destroy(z80emu->cpu);
}

// Create all UI sub-windows
void init_windows(z80emu_t* z80emu) {
    int x, y;
    uint8_t reg_width;
    uint8_t mem_width, mid_width;

    getmaxyx(stdscr, y, x);
    if (z80emu->reg_win != NULL) {
        delwin(z80emu->reg_win);
    }
    if (z80emu->mem_win != NULL) {
        delwin(z80emu->mem_win);
    }
    if (z80emu->msg_win != NULL) {
        delwin(z80emu->msg_win);
    }

    reg_width = 33;
    mid_width = 33;

    z80emu->reg_width = reg_width;
    z80emu->reg_height = y;

    mem_width = x - reg_width - mid_width;

    z80emu->msg_width = x - reg_width;
    z80emu->msg_height = 3;

    z80emu->asm_width = mid_width;
    z80emu->asm_height = 20;

    z80emu->mem_width = mem_width;
    z80emu->mem_height = y - z80emu->msg_height;

    z80emu->reg_win = newwin(z80emu->reg_height, z80emu->reg_width, 0, 0);
    z80emu->msg_win = newwin(z80emu->msg_height, z80emu->msg_width, z80emu->mem_height, reg_width);
    z80emu->mem_win = newwin(z80emu->mem_height, z80emu->mem_width, 0, reg_width + mid_width);
    z80emu->asm_win = newwin(z80emu->asm_height, z80emu->asm_width, 0, mid_width);

    box(z80emu->reg_win, 0, 0);
    box(z80emu->msg_win, 0, 0);
    box(z80emu->mem_win, 0, 0);
    box(z80emu->asm_win, 0, 0);
    wbkgd(z80emu->reg_win, COLOR_PAIR(3));
    wbkgd(z80emu->msg_win, COLOR_PAIR(3));
    wbkgd(z80emu->mem_win, COLOR_PAIR(3));
    wbkgd(z80emu->asm_win, COLOR_PAIR(3));
}

// Curses init
void init_view(z80emu_t* z80emu) {
    setlocale(LC_ALL, "");
    initscr();
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW,COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_BLACK, COLOR_MAGENTA);
        init_pair(8, COLOR_WHITE, COLOR_BLACK);
    }
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
    timeout(1000);
    curs_set(FALSE);
    init_windows(z80emu);
}

// Resize event
void resize_view(z80emu_t* z80emu) {
    init_windows(z80emu);
}

// Draw the inside borders of the register window
void draw_reg_win(z80emu_t* z80emu) {
    int x, y;

    x = 1;
    y = 1;
    wattron(z80emu->reg_win, COLOR_PAIR(2));
    mvwaddstr(z80emu->reg_win, y, x, "PC            [program counter]"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "───────────────┬───────────────"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "A [accumulator]│F       [flags]"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "───────────────┼───────────────"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "B     [counter]│C        [port]"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "───────────────┼───────────────"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "D              │E              "); y++;
    mvwaddstr(z80emu->reg_win, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "───────────────┼───────────────"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "H              │L              "); y++;
    mvwaddstr(z80emu->reg_win, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "───────────────┴───────────────"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "I                   [interrupt]"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "───────────────────────────────"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "R                     [refresh]"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "───────────────────────────────"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "SP              [stack pointer]"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "───────────────────────────────"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "IX            IXH      IXL     "); y++;
    mvwaddstr(z80emu->reg_win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "───────────────────────────────"); y++;
    mvwaddstr(z80emu->reg_win, y, x, "IY            IYH      IYL     "); y++;
    mvwaddstr(z80emu->reg_win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    wattroff(z80emu->reg_win, COLOR_PAIR(2));
}

// Draw a single 8 bit register
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

    wattron(z80emu->reg_win, COLOR_PAIR(6));
    mvwaddstr(z80emu->reg_win, y, x, dec); x += 4;
    mvwaddstr(z80emu->reg_win, y, x, hex); x += 3;
    mvwaddstr(z80emu->reg_win, y, x, bin);
    wattroff(z80emu->reg_win, COLOR_PAIR(6));
}

// Draw a single 16 bit register
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

    wattron(z80emu->reg_win, COLOR_PAIR(6));
    mvwaddstr(z80emu->reg_win, y, x, dec); x += 8;
    mvwaddstr(z80emu->reg_win, y, x, hex); x += 7;
    mvwaddstr(z80emu->reg_win, y, x, bin);
    wattroff(z80emu->reg_win, COLOR_PAIR(6));
}

// Draw a single 16 bit register as two separate 8 bit registers
void draw_reg_16_as_two_8(z80emu_t* z80emu, uint8_t x, uint8_t y, uint16_t value) {
    uint8_t lower;
    uint8_t higher;

    lower = (uint8_t)(value & 0xFF);
    higher = (uint8_t)(value >> 8);

    draw_reg_8(z80emu, x, y, higher);
    draw_reg_8(z80emu, x + 16, y, lower);
}

// Draw the registers in the register window
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

// Draw the memory view window
void draw_memory(z80emu_t* z80emu) {
    char hex[3];
    uint8_t viz_width, viz_height;
    uint8_t nr_bytes_across;
    uint8_t x, y;
    uint16_t i, start_addr;
    uint8_t b_count;
    uint8_t ins_on_line;
    Z80EX_WORD pc;

    viz_width = z80emu->mem_width - 2;
    viz_height = z80emu->mem_height - 2;
    nr_bytes_across = viz_width / 3;

    hex[2] = '\0';

    pc = z80ex_get_reg(z80emu->cpu, regPC);

    ins_on_line = (uint16_t)pc / nr_bytes_across;
    memset(z80emu->msg, 0, 255);
    sprintf(z80emu->msg, "instruction on line: %d", ins_on_line);
    //debug_message(z80emu, z80emu->msg);

    start_addr = 0;
    if (ins_on_line >= viz_height) {
        start_addr = (nr_bytes_across * (ins_on_line - 2));
    }

    b_count = 1;
    x = 1;
    y = 1;

    for (i = start_addr; i < 65536; i++) {
        sprintf(hex, "%02x", z80emu->memory[i]);
        if (i == (uint16_t)pc) {
            wattron(z80emu->mem_win, COLOR_PAIR(7));
            mvwaddstr(z80emu->mem_win, y, x, hex);
            wattroff(z80emu->mem_win, COLOR_PAIR(7));
        }
        else {
            wattron(z80emu->mem_win, COLOR_PAIR(6));
            mvwaddstr(z80emu->mem_win, y, x, hex);
            wattroff(z80emu->mem_win, COLOR_PAIR(6));
        }
        x += 3;
        b_count++;
        if (b_count > nr_bytes_across) {
            b_count = 1;
            x = 1;
            y++;
        }
        if (y > viz_height) {
            break;
        }
    }
}

// Draw the ASM window
void draw_asm(z80emu_t* z80emu) {
    char asm_before[255];
    char asm_after[255];
    int t, t2;

    z80ex_dasm(asm_before, 255, 0, &t, &t2, mem_read_dasm, z80emu->pc_before, z80emu);
    z80ex_dasm(asm_after, 255, 0, &t, &t2, mem_read_dasm, z80emu->pc_after, z80emu);
    mvwaddstr(z80emu->asm_win, 2, 1, asm_before);
    mvwaddstr(z80emu->asm_win, 1, 1, asm_after);
}

// Color the border of the selected window
void selected_window_color(z80emu_t* z80emu) {
    switch (z80emu->win_mode) {
        case MODE_REG:
            wbkgd(z80emu->reg_win, COLOR_PAIR(4));
            wbkgd(z80emu->asm_win, COLOR_PAIR(3));
            wbkgd(z80emu->mem_win, COLOR_PAIR(3));
            break;
        case MODE_ASM:
            wbkgd(z80emu->reg_win, COLOR_PAIR(3));
            wbkgd(z80emu->asm_win, COLOR_PAIR(4));
            wbkgd(z80emu->mem_win, COLOR_PAIR(3));
            break;
        case MODE_MEM:
            wbkgd(z80emu->reg_win, COLOR_PAIR(3));
            wbkgd(z80emu->asm_win, COLOR_PAIR(3));
            wbkgd(z80emu->mem_win, COLOR_PAIR(4));
            break;
        default:
            break;
    }
}

// Redraw everything
void refresh_view(z80emu_t* z80emu) {
    selected_window_color(z80emu);
    draw_reg_win(z80emu);
    draw_registers(z80emu);
    draw_memory(z80emu);
    draw_asm(z80emu);
    refresh();
    wrefresh(z80emu->reg_win);
    wrefresh(z80emu->msg_win);
    wrefresh(z80emu->mem_win);
    wrefresh(z80emu->asm_win);
}

// Handle movement key presses in the memory view
void key_move_handle_mem(z80emu_t* z80emu, int c) {
    switch (c) {
        case KEY_DOWN:
            break;
        default:
            break;
    }
}

// Handle all movement key presses
void key_move_handle(z80emu_t* z80emu, int c) {
    if (z80emu->win_mode == MODE_MEM) {
        key_move_handle_mem(z80emu, c);
    }
}

// Execute an instruction
void execute_instruction(z80emu_t* z80emu) {
    z80emu->pc_before = z80ex_get_reg(z80emu->cpu, regPC);
    z80ex_step(z80emu->cpu);
    z80emu->pc_after = z80ex_get_reg(z80emu->cpu, regPC);
}

// Window resize events
void handle_winch(int sig){
    signal(SIGWINCH, SIG_IGN);
    endwin();
    resize_view(&Z80EMU);
    refresh_view(&Z80EMU);
    signal(SIGWINCH, handle_winch);
}

// Ctrl-C events
static void handle_int(int sig) {
    cleanup_context(&Z80EMU);
    endwin();
    exit(0);
}

// Install signal handles
void install_signal_handlers() {
    signal(SIGINT, handle_int);
    signal(SIGWINCH, handle_winch);
}

// Load a ROM file from disk into 64k memory
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

// Usage
void print_help() {
    printf("Usage: z80emu [-r<binary rom file>]\n");
}

// initialize an instance of a z80 emulator
void init_z80emu(z80emu_t* z80emu) {
    memset(z80emu, 0, sizeof(z80emu_t));
    z80emu->memory = malloc(sizeof(uint8_t) * 65536);
    if (z80emu->memory == NULL) {
        return;
    }
    memset(z80emu->memory, 0, sizeof(uint8_t) * 65536);
}

// The main entry point after the instance is set up
void main_program(z80emu_t* z80emu) {
    z80emu->cpu = z80ex_create(mem_read, z80emu, mem_write, z80emu, port_read, z80emu, port_write, z80emu, int_read, z80emu);

    init_view(z80emu);
    install_signal_handlers();
    refresh_view(z80emu);

    uint8_t pause = 1;
    int c;
    while (1) {
        c = getch();
        if (ERR != c) {
            pause = 1;
            switch (c) {
                case 's':
                    if (!pause) {
                        execute_instruction(z80emu);
                        refresh_view(z80emu);
                    }
                    break;
                case ' ':
                    pause = !pause;
                    break;
                case 'q':
                    cleanup_context(z80emu);
                    endwin();
                    exit(0);
                    break;
                case 0x09: // TAB
                    z80emu->win_mode += 1;
                    if (z80emu->win_mode > MODE_MAX) {
                        z80emu->win_mode = MODE_ZERO;
                    }
                    refresh_view(z80emu);
                    break;
                case KEY_DOWN:
                    key_move_handle(z80emu, c);
                    refresh_view(z80emu);
                    break;
                case KEY_UP:
                    key_move_handle(z80emu, c);
                    refresh_view(z80emu);
                    break;
                case KEY_RIGHT:
                    key_move_handle(z80emu, c);
                    refresh_view(z80emu);
                    break;
                case KEY_LEFT:
                    key_move_handle(z80emu, c);
                    refresh_view(z80emu);
                    break;
                default:
                    break;
            }
        }
        if (!pause) {
            execute_instruction(z80emu);
            refresh_view(z80emu);
        }
    }
}

// Parse command args and set up the instance
int main(int argc, char *argv[]) {
    int8_t c;
    int option_index = 0;

    init_z80emu(&Z80EMU);

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
                load_binary_rom(&Z80EMU, optarg);
                break;
            default:
                print_help();
                exit(1);
        }
    }

    main_program(&Z80EMU);

    cleanup_context(&Z80EMU);
    endwin();
    return 0;
}
