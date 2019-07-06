#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <locale.h>
#include <z80ex.h>
#include <z80ex_dasm.h>
#include <ctk.h>
#include "reg_win.h"
#include "mem_win.h"
#include "asm_win.h"

// Which sub-window has focus
enum win_mode {
    MODE_REG,
    MODE_ASM,
    MODE_MEM
};
#define MODE_ZERO 0x00
#define MODE_MAX 0x02

#define AREA_ASM 0
#define AREA_MEM 1
#define RIGHT_HBOX 2
#define AREA_MSG 3
#define AREA_REG 4
#define RIGHT_VBOX 5
#define MAIN_HBOX 6
ctk_widget_t WIDGETS[7];
// +-------+-----+-------------------+
// | REG   | ASM | MEM               |
// |       |     |                   |
// |       |     |                   |
// |       +-----+-------------------+
// |       | MSG                     |
// +-------+-------------------------+

// An instance of an emulator UI
typedef struct z80emu {
    FILE* rom_fh;
    uint8_t* memory;
    Z80EX_CONTEXT *cpu;
    uint16_t pc_before;
    uint16_t pc_after;
    enum win_mode win_mode;
    ctk_ctx_t ctx;
    reg_win_t reg_win;
    asm_win_t asm_win;
    WINDOW* msg_win;
    uint8_t msg_width;
    uint8_t msg_height;
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
    wattron(z80emu->msg_win, COLOR_PAIR(CTK_COLOR_WARNING));
    mvwaddstr(z80emu->msg_win, 1, 1, message);
    wattroff(z80emu->msg_win, COLOR_PAIR(CTK_COLOR_WARNING));
}

// Destroy a z80emu instance
void cleanup_context(z80emu_t* z80emu) {
    if (z80emu->memory != NULL) {
        free(z80emu->memory);
    }
    z80ex_destroy(z80emu->cpu);
}

static uint8_t asm_event_handler(ctk_event_t* event, void* user_data) {
    if (event->type != CTK_EVENT_DRAW) {
        return 1;
    }
    //z80emu_t* z80emu = (z80emu_t*)user_data;
    return 1;
}

static uint8_t mem_event_handler(ctk_event_t* event, void* user_data) {
    if (event->type != CTK_EVENT_DRAW) {
        return 1;
    }
    z80emu_t* z80emu = (z80emu_t*)user_data;
    mem_win_draw(event->widget, z80emu->memory, z80emu->pc_before);
    return 1;
}

static uint8_t reg_event_handler(ctk_event_t* event, void* user_data) {
    if (event->type != CTK_EVENT_DRAW) {
        return 1;
    }
    z80emu_t* z80emu = (z80emu_t*)user_data;
    z80emu->reg_win.PC = (uint16_t)z80ex_get_reg(z80emu->cpu, regPC);
    z80emu->reg_win.AF = (uint16_t)z80ex_get_reg(z80emu->cpu, regAF);
    z80emu->reg_win.BC = (uint16_t)z80ex_get_reg(z80emu->cpu, regBC);
    z80emu->reg_win.DE = (uint16_t)z80ex_get_reg(z80emu->cpu, regDE);
    z80emu->reg_win.HL = (uint16_t)z80ex_get_reg(z80emu->cpu, regHL);
    z80emu->reg_win.I = (uint16_t)z80ex_get_reg(z80emu->cpu, regI);
    z80emu->reg_win.R = (uint16_t)z80ex_get_reg(z80emu->cpu, regR);
    z80emu->reg_win.SP = (uint16_t)z80ex_get_reg(z80emu->cpu, regSP);
    z80emu->reg_win.IX = (uint16_t)z80ex_get_reg(z80emu->cpu, regIX);
    z80emu->reg_win.IY = (uint16_t)z80ex_get_reg(z80emu->cpu, regIY);
    reg_win_draw(event->widget, &z80emu->reg_win);
    return 1;
}

void init_windows(z80emu_t* z80emu) {
    ctk_init_area(&WIDGETS[AREA_ASM], 20, 10, 0, 1);
    ctk_init_area(&WIDGETS[AREA_MEM], 10, 10, 1, 1);
    ctk_init_hbox(&WIDGETS[RIGHT_HBOX], &WIDGETS[AREA_ASM], 2);
    ctk_init_area(&WIDGETS[AREA_MSG], 30, 1, 1, 0);
    ctk_init_area(&WIDGETS[AREA_REG], 33, 10, 0, 1);
    ctk_init_vbox(&WIDGETS[RIGHT_VBOX], &WIDGETS[RIGHT_HBOX], 2);
    ctk_init_hbox(&WIDGETS[MAIN_HBOX], &WIDGETS[AREA_REG], 2);
    WIDGETS[AREA_ASM].event_callback = asm_event_handler;
    WIDGETS[AREA_ASM].user_data = z80emu;
    WIDGETS[AREA_MEM].event_callback = mem_event_handler;
    WIDGETS[AREA_MEM].user_data = z80emu;
    WIDGETS[AREA_REG].event_callback = reg_event_handler;
    WIDGETS[AREA_REG].user_data = z80emu;
    ctk_init(&z80emu->ctx, &WIDGETS[MAIN_HBOX], 1);
}

// Curses init
void init_view(z80emu_t* z80emu) {
    ctk_init_curses();
    ctk_init_ctx(&z80emu->ctx, stdscr);
    init_windows(z80emu);
}

// Resize event
void resize_view(z80emu_t* z80emu) {
    init_windows(z80emu);
}

void draw_asm(z80emu_t* z80emu) {
    char asm_before[255];
    char asm_after[255];
    int t, t2;
    memset(asm_before, 0, 255);
    memset(asm_after, 0, 255);
    z80ex_dasm(asm_before, 255, 0, &t, &t2, mem_read_dasm, z80emu->pc_before, z80emu);
    z80ex_dasm(asm_after, 255, 0, &t, &t2, mem_read_dasm, z80emu->pc_after, z80emu);
    asm_win_draw(&z80emu->asm_win, asm_before, asm_after);
}

// Color the border of the selected window
void selected_window_color(z80emu_t* z80emu) {
    switch (z80emu->win_mode) {
        case MODE_REG:
            asm_win_unselect_window(&z80emu->asm_win);
            mem_win_unselect_window();
            break;
        case MODE_ASM:
            asm_win_select_window(&z80emu->asm_win);
            mem_win_unselect_window();
            break;
        case MODE_MEM:
            asm_win_unselect_window(&z80emu->asm_win);
            mem_win_select_window();
            break;
        default:
            break;
    }
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

void reset_all(z80emu_t* z80emu) {
    z80emu->pc_before = 0;
    z80emu->pc_after = 0;
    z80ex_reset(z80emu->cpu);
}

// The main entry point after the instance is set up
void main_program(z80emu_t* z80emu) {
    z80emu->cpu = z80ex_create(mem_read, z80emu, mem_write, z80emu, port_read, z80emu, port_write, z80emu, int_read, z80emu);
    init_view(z80emu);
    ctk_main_loop(&z80emu->ctx);
/*
    uint8_t pause = 1;
    int c;
    while (1) {
        c = getch();
        if (ERR != c) {
            pause = 1;
            switch (c) {
                case 's':
                    pause = 1;
                    execute_instruction(z80emu);
                    refresh_view(z80emu);
                    break;
                case 'R':
                    reset_all(z80emu);
                    refresh_view(z80emu);
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
*/
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
