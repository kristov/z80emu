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

#define Z80EMU_MODE_KEY 0x00
#define Z80EMU_MODE_CMD 0x01

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

struct label {
    uint16_t addr;
    char* name;
};

typedef struct msg_bar {
    char msg[256];
    uint8_t pos;
} msg_bar_t;

typedef struct z80emu {
    FILE* rom_fh;
    uint8_t* memory;
    Z80EX_CONTEXT *cpu;
    uint16_t pc_before;
    uint16_t pc_after;
    ctk_ctx_t ctx;
    reg_win_t reg_win;
    asm_win_t asm_win;
    mem_win_t mem_win;
    uint8_t paused;
    uint8_t mode;
    msg_bar_t msg_bar;
    struct label* labels;
} z80emu_t;

// Only one emulation happening at a time
z80emu_t Z80EMU;

typedef struct rom_spec {
    char* file;
    uint16_t start;
    uint16_t end;
} rom_spec_t;

static void msg_bar_debug(msg_bar_t* msg_bar, const char *format, ...) {
    memset(&msg_bar->msg[0], 0, 256);
    msg_bar->pos = 0;
    va_list aptr;
    va_start(aptr, format);
    vsprintf(&msg_bar->msg[0], format, aptr);
    va_end(aptr);
    msg_bar->msg[255] = '\0';
}

static void msg_bar_clear(msg_bar_t* msg_bar) {
    memset(&msg_bar->msg[0], 0, 256);
    msg_bar->pos = 0;
}

static char* msg_bar_get_message(msg_bar_t* msg_bar) {
    return &msg_bar->msg[0];
}

static void msg_bar_add_char(msg_bar_t* msg_bar, int key) {
    msg_bar->msg[msg_bar->pos++] = key;
}

// Z80EX-Callback for a CPU memory read
Z80EX_BYTE mem_read(Z80EX_CONTEXT* cpu, Z80EX_WORD addr, int m1_state, void* user_data) {
    z80emu_t* z80emu;
    z80emu = user_data;
    return z80emu->memory[(uint16_t)addr];
}

// Z80EX-Callback for a CPU memory write
void mem_write(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void* user_data) {
    z80emu_t* z80emu = (z80emu_t*)user_data;
    msg_bar_debug(&z80emu->msg_bar, "memory write: address[%016x] data[%08x]", addr, value);
    z80emu->memory[(uint16_t)addr] = value;
}

// Z80EX-Callback for a CPU port read
Z80EX_BYTE port_read(Z80EX_CONTEXT *cpu, Z80EX_WORD port, void* user_data) {
    z80emu_t* z80emu = (z80emu_t*)user_data;
    msg_bar_debug(&z80emu->msg_bar, "port read: address[%016x]", port);
    return 0;
}

// Z80EX-Callback for a CPU port write
void port_write(Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value, void* user_data) {
    z80emu_t* z80emu = (z80emu_t*)user_data;
    msg_bar_debug(&z80emu->msg_bar, "port write: address[%016x] data[%08x]", port, value);
}

// Z80EX-Callback for an interrupt read
Z80EX_BYTE int_read(Z80EX_CONTEXT *cpu, void* user_data) {
    z80emu_t* z80emu = (z80emu_t*)user_data;
    msg_bar_debug(&z80emu->msg_bar, "interrupt vector!");
    return 0;
}

// Z80EX-Callback for DASM memory read
Z80EX_BYTE mem_read_dasm(Z80EX_WORD addr, void* user_data) {
    z80emu_t* z80emu = (z80emu_t*)user_data;
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

static char* look_for_label(z80emu_t* z80emu, uint16_t addr) {
    if (z80emu->labels == NULL) {
        return NULL;
    }
    struct label* label = &z80emu->labels[0];
    while (label->name != NULL) {
        if (label->addr == addr) {
            return label->name;
        }
        label++;
    }
    return NULL;
}

static uint8_t asm_event_handler(ctk_event_t* event, void* user_data) {
    if (event->type != CTK_EVENT_DRAW) {
        return 1;
    }
    z80emu_t* z80emu = (z80emu_t*)user_data;
    if (z80emu->paused) {
        return 1;
    }
    char asm_before[255];
    char asm_after[255];
    char tmp[255];
    int t, t2;
    memset(asm_before, 0, 255);
    memset(asm_after, 0, 255);
    memset(tmp, 0, 255);
    uint16_t pc_before = z80emu->pc_before;
    uint8_t ins = z80emu->memory[pc_before];
    char* name = NULL;
    if ((ins == 0xc2) || (ins == 0xc3) || (ins == 0xca) || (ins == 0xcd)) {
        uint16_t addr = (z80emu->memory[pc_before + 1]) | (z80emu->memory[pc_before + 2] << 8);
        name = look_for_label(z80emu, addr);
    }
    if (ins == 0x10) {
        int8_t addr = z80emu->memory[pc_before + 1];
        uint16_t naddr = addr + pc_before + 2;
        msg_bar_debug(&z80emu->msg_bar, "naddr: %04x", naddr);
        name = look_for_label(z80emu, naddr);
    }
    z80ex_dasm(asm_before, 255, 0, &t, &t2, mem_read_dasm, z80emu->pc_before, z80emu);
    z80ex_dasm(asm_after, 255, 0, &t, &t2, mem_read_dasm, z80emu->pc_after, z80emu);
    if (name != NULL) {
        snprintf(tmp, 255, "%s [%s]", asm_before, name);
        asm_win_draw(event->widget, &z80emu->asm_win, tmp, asm_after);
        return 1;
    }
    asm_win_draw(event->widget, &z80emu->asm_win, asm_before, asm_after);
    return 1;
}

static uint8_t mem_event_handler(ctk_event_t* event, void* user_data) {
    if (event->type != CTK_EVENT_DRAW) {
        return 1;
    }
    z80emu_t* z80emu = (z80emu_t*)user_data;
    z80emu->mem_win.pc = z80emu->pc_before;
    mem_win_draw(event->widget, &z80emu->mem_win);
    return 1;
}

static uint8_t msg_event_handler(ctk_event_t* event, void* user_data) {
    if (event->type != CTK_EVENT_DRAW) {
        return 1;
    }
    z80emu_t* z80emu = (z80emu_t*)user_data;
    ctk_addstr(event->widget, 0, 0, 1, msg_bar_get_message(&z80emu->msg_bar));
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

static uint8_t cmd_mode_handler(z80emu_t* z80emu, int key) {
    if (key == '\n') {
        msg_bar_clear(&z80emu->msg_bar);
        z80emu->mode = Z80EMU_MODE_KEY;
        z80emu->paused = 0;
        return 1;
    }
    msg_bar_add_char(&z80emu->msg_bar, key);
    return 1;
}

static uint8_t main_event_handler(ctk_event_t* event, void* user_data) {
    if (event->type != CTK_EVENT_KEY) {
        return 1;
    }
    z80emu_t* z80emu = (z80emu_t*)user_data;
    if (z80emu->mode == Z80EMU_MODE_CMD) {
        event->ctx->redraw = 1;
        return cmd_mode_handler(z80emu, event->key);
    }
    //msg_bar_clear(&z80emu->msg_bar);
    switch (event->key) {
        case 's':
            execute_instruction(z80emu);
            event->ctx->redraw = 1;
            break;
        case 'i':
            z80ex_int(z80emu->cpu);
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
        case ':':
            z80emu->paused = 1;
            event->ctx->redraw = 1;
            msg_bar_clear(&z80emu->msg_bar);
            z80emu->mode = Z80EMU_MODE_CMD;
            break;
        case 339:
            msg_bar_debug(&z80emu->msg_bar, "page-up");
            mem_win_pageup(&z80emu->mem_win);
            event->ctx->redraw = 1;
            break;
        case 338:
            msg_bar_debug(&z80emu->msg_bar, "page-down");
            mem_win_pagedown(&z80emu->mem_win);
            event->ctx->redraw = 1;
            break;
        default:
            msg_bar_debug(&z80emu->msg_bar, "key: %d", event->key);
            event->ctx->redraw = 1;
            break;
    }
    return 1;
}

void init_windows(z80emu_t* z80emu) {
    ctk_init_area(&WIDGETS[AREA_ASM], 20, 10, 1, 1);
    ctk_init_vrule(&WIDGETS[ASM_MEM_VRULE]);
    ctk_init_area(&WIDGETS[AREA_MEM], 54, 10, 0, 1);
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
    mem_win_init(&z80emu->mem_win, z80emu->memory);
    asm_win_init(&z80emu->asm_win, WIDGETS[AREA_ASM].width, WIDGETS[AREA_ASM].height);
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

void load_list_file(z80emu_t* z80emu, char* param) {
    FILE* fh = fopen(param, "r");
    if (fh == NULL) {
        fprintf(stderr, "Could not open list file: %s\n", param);
        exit(1);
    }
    char* line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    char* label = NULL;
    char* hex = NULL;
    uint32_t labellen = 0;
    uint16_t addr = 0;
    char c = '\0';
    uint16_t nr_labels = 0;
    while ((read = getline(&line, &len, fh)) != -1) {
        label = strtok(line, ":");
        if (label == NULL) {
            continue;
        }
        hex = strtok(NULL, "$");
        if (hex == NULL) {
            continue;
        }
        hex = strtok(NULL, "\n");
        if (hex == NULL) {
            continue;
        }
        while ((c = *hex++)) {
            char v = ((c & 0xf) + (c >> 6)) | ((c >> 3) & 0x8);
            addr = (addr << 4) | (uint16_t) v;
        }
        nr_labels++;
    }
    if (labellen >= 65536) {
        fprintf(stderr, "Too many strings in list file!\n");
        fclose(fh);
        exit(1);
    }

    z80emu->labels = malloc(sizeof(struct label) * (nr_labels + 1));
    struct label* curr_label = NULL;
    curr_label = &z80emu->labels[0];

    rewind(fh);
    while ((read = getline(&line, &len, fh)) != -1) {
        label = strtok(line, ":");
        if (label == NULL) {
            continue;
        }
        hex = strtok(NULL, "$");
        if (hex == NULL) {
            continue;
        }
        hex = strtok(NULL, "\n");
        if (hex == NULL) {
            continue;
        }
        while ((c = *hex++)) {
            char v = ((c & 0xf) + (c >> 6)) | ((c >> 3) & 0x8);
            addr = (addr << 4) | (uint16_t) v;
        }
        curr_label->addr = addr;
        size_t len = strlen(label);
        curr_label->name = malloc(sizeof(char) * (len + 1));
        strncpy(curr_label->name, label, len + 1);
        curr_label++;
    }
    curr_label->addr = 0;
    curr_label->name = NULL;
    fclose(fh);
}

void load_binary_rom(z80emu_t* z80emu, rom_spec_t* rom_spec) {
    unsigned long len;

    z80emu->rom_fh = fopen(rom_spec->file, "rb");
    if (z80emu->rom_fh == NULL) {
        fprintf(stderr, "Could not open rom file: %s\n", rom_spec->file);
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

void parse_rom_spec(char* param, rom_spec_t* rom_spec) {
    rom_spec->start = 0;
    rom_spec->end = 65535;
    char* ptr = NULL;
    ptr = strtok(param, ":");
    rom_spec->file = ptr;
    ptr = strtok(NULL, ":");
    if (ptr == NULL) {
        return;
    }
    rom_spec->start = (uint16_t)strtol(ptr, NULL, 0);
    ptr = strtok(NULL, ":");
    if (ptr == NULL) {
        return;
    }
    rom_spec->end = (uint16_t)strtol(ptr, NULL, 0);
}

// Usage
void print_help() {
    printf("Usage: z80emu [-r <rom>]\n");
    printf("\n");
    printf("  -r <rom>\n");
    printf("    The ROM to load into memory before execution. This can take one of three forms:\n");
    printf("    1) -r rom.bin           : Start address 0, end addr 65535\n");
    printf("    2) -r rom.bin:512       : Start address 512, end addr 65535\n");
    printf("    3) -r rom.bin:512:1024  : Start address 512, end addr 1024\n");
    printf("\n");
    printf("  -l <listing file>\n");
    printf("    A file containing labels and addresses. When a 'call' instruction is encountered\n");
    printf("    in the z80asm-gnu format (-l option)\n");
    printf("\n");
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
    rom_spec_t rom_spec;

    init_z80emu(&Z80EMU);

    static struct option long_options[] = {
        {"help", optional_argument, 0, 'h'},
        {"rom", optional_argument, 0, 'r'},
        {"list", optional_argument, 0, 'l'},
        {0, 0, 0, 0}
    };

    while (1) {
        c = getopt_long(argc, argv, "hr:l:", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'r':
                parse_rom_spec(optarg, &rom_spec);
                load_binary_rom(&Z80EMU, &rom_spec);
                break;
            case 'l':
                load_list_file(&Z80EMU, optarg);
                break;
            case 'h':
                print_help();
                exit(1);
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
