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
#define ASM_MEM_VRULE 1
#define AREA_MEM 2
#define RIGHT_HBOX 3
#define HBOX_MSG_HRULE 4
#define AREA_MSG 5
#define AREA_REG 6
#define REG_VBOX_VRULE 7
#define RIGHT_VBOX 8
#define MAIN_HBOX 9
ctk_widget_t WIDGETS[10];
// +-------+-----+-------------------+
// | REG   | ASM | MEM               |
// |       |     |                   |
// |       |     |                   |
// |       +-----+-------------------+
// |       | MSG                     |
// +-------+-------------------------+

char MSG[256];

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

void debug_message(const char *format, ...) {
    memset(&MSG, 0, 256);
    va_list aptr;
    va_start(aptr, format);
    vsprintf(&MSG[0], format, aptr);
    va_end(aptr);
    MSG[255] = '\0';
}

// Z80EX-Callback for a CPU memory read
Z80EX_BYTE mem_read(Z80EX_CONTEXT* cpu, Z80EX_WORD addr, int m1_state, void* user_data) {
    z80emu_t* z80emu;
    z80emu = user_data;
    return z80emu->memory[(uint16_t)addr];
}

// Z80EX-Callback for a CPU memory write
void mem_write(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *z80emu) {
    debug_message("memory write: address[%016x] data[%08x]", addr, value);
}

// Z80EX-Callback for a CPU port read
Z80EX_BYTE port_read(Z80EX_CONTEXT *cpu, Z80EX_WORD port, void *z80emu) {
    debug_message("port read: address[%016x]", port);
    return 0;
}

// Z80EX-Callback for a CPU port write
void port_write(Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value, void *z80emu) {
    debug_message("port write: address[%016x] data[%08x]", port, value);
}

// Z80EX-Callback for an interrupt read
Z80EX_BYTE int_read(Z80EX_CONTEXT *cpu, void *z80emu) {
    debug_message("interrupt vector!");
    return 0;
}

// Z80EX-Callback for DASM memory read
Z80EX_BYTE mem_read_dasm(Z80EX_WORD addr, void *user_data) {
    z80emu_t* z80emu;
    z80emu = user_data;
    return z80emu->memory[(uint16_t)addr];
}

void cleanup_context(z80emu_t* z80emu) {
    if (z80emu->memory != NULL) {
        free(z80emu->memory);
    }
    z80ex_destroy(z80emu->cpu);
}

static void reset_all(z80emu_t* z80emu) {
    z80emu->pc_before = 0;
    z80emu->pc_after = 0;
    z80ex_reset(z80emu->cpu);
}

static void execute_instruction(z80emu_t* z80emu) {
    z80emu->pc_before = z80ex_get_reg(z80emu->cpu, regPC);
    z80ex_step(z80emu->cpu);
    z80emu->pc_after = z80ex_get_reg(z80emu->cpu, regPC);
}

static uint8_t asm_event_handler(ctk_event_t* event, void* user_data) {
    if (event->type != CTK_EVENT_DRAW) {
        return 1;
    }
    z80emu_t* z80emu = (z80emu_t*)user_data;
    char asm_before[255];
    char asm_after[255];
    int t, t2;
    memset(asm_before, 0, 255);
    memset(asm_after, 0, 255);
    z80ex_dasm(asm_before, 255, 0, &t, &t2, mem_read_dasm, z80emu->pc_before, z80emu);
    z80ex_dasm(asm_after, 255, 0, &t, &t2, mem_read_dasm, z80emu->pc_after, z80emu);
    asm_win_draw(event->widget, &z80emu->asm_win, asm_before, asm_after);
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

static uint8_t msg_event_handler(ctk_event_t* event, void* user_data) {
    if (event->type != CTK_EVENT_DRAW) {
        return 1;
    }
    ctk_addstr(event->widget, 0, 0, 1, &MSG[0]);
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

static uint8_t main_event_handler(ctk_event_t* event, void* user_data) {
    if (event->type != CTK_EVENT_KEY) {
        return 1;
    }
    z80emu_t* z80emu = (z80emu_t*)user_data;
    switch (event->key) {
        case 's':
            execute_instruction(z80emu);
            event->ctx->redraw = 1;
            break;
        case 'R':
            reset_all(z80emu);
            event->ctx->redraw = 1;
            break;
        case 'q':
            cleanup_context(z80emu);
            endwin();
            exit(0);
            break;
        default:
            break;
    }
    return 1;
}

void init_windows(z80emu_t* z80emu) {
    ctk_init_area(&WIDGETS[AREA_ASM], 20, 10, 0, 1);
    ctk_init_vrule(&WIDGETS[ASM_MEM_VRULE]);
    ctk_init_area(&WIDGETS[AREA_MEM], 10, 10, 1, 1);
    ctk_init_hbox(&WIDGETS[RIGHT_HBOX], &WIDGETS[AREA_ASM], 3);
    ctk_init_hrule(&WIDGETS[HBOX_MSG_HRULE]);
    ctk_init_area(&WIDGETS[AREA_MSG], 30, 1, 1, 0);
    ctk_init_area(&WIDGETS[AREA_REG], 31, 10, 0, 1);
    ctk_init_vrule(&WIDGETS[REG_VBOX_VRULE]);
    ctk_init_vbox(&WIDGETS[RIGHT_VBOX], &WIDGETS[RIGHT_HBOX], 3);
    ctk_init_hbox(&WIDGETS[MAIN_HBOX], &WIDGETS[AREA_REG], 3);
    ctk_widget_event_handler(&WIDGETS[AREA_ASM], asm_event_handler, z80emu);
    ctk_widget_event_handler(&WIDGETS[AREA_MEM], mem_event_handler, z80emu);
    ctk_widget_event_handler(&WIDGETS[AREA_REG], reg_event_handler, z80emu);
    ctk_widget_event_handler(&WIDGETS[AREA_MSG], msg_event_handler, z80emu);
    ctk_init(&z80emu->ctx, &WIDGETS[MAIN_HBOX], 1);
    ctk_widget_event_handler(&z80emu->ctx.mainwin, main_event_handler, z80emu);
    asm_win_init(&z80emu->asm_win, WIDGETS[AREA_ASM].height);
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

void handle_winch(int sig){
    signal(SIGWINCH, SIG_IGN);
    endwin();
    resize_view(&Z80EMU);
    signal(SIGWINCH, handle_winch);
}

static void handle_int(int sig) {
    cleanup_context(&Z80EMU);
    endwin();
    exit(0);
}

void install_signal_handlers() {
    signal(SIGINT, handle_int);
    signal(SIGWINCH, handle_winch);
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

// Usage
void print_help() {
    printf("Usage: z80emu [-r<binary rom file>]\n");
}

// The main entry point after the instance is set up
void main_program(z80emu_t* z80emu) {
    z80emu->cpu = z80ex_create(mem_read, z80emu, mem_write, z80emu, port_read, z80emu, port_write, z80emu, int_read, z80emu);
    init_view(z80emu);
    ctk_main_loop(&z80emu->ctx);
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
