#include <ncurses.h>
#include "reg_win.h"
#include "ctk.h"

void reg_win_init(reg_win_t* reg_win, uint8_t width, uint8_t height, uint8_t x, uint8_t y) {
    if (reg_win->win != NULL) {
        delwin(reg_win->win);
    }
    reg_win->win = newwin(height, width, y, x);
    box(reg_win->win, 0, 0);
    wbkgd(reg_win->win, COLOR_PAIR(CTK_COLOR_WINDOW));
}

void reg_win_destroy(reg_win_t* reg_win) {
    delwin(reg_win->win);
}

// Draw a single 8 bit register
void draw_reg_8(reg_win_t* reg_win, uint8_t x, uint8_t y, uint8_t value) {
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

    wattron(reg_win->win, COLOR_PAIR(CTK_COLOR_HIGHLIGHT));
    mvwaddstr(reg_win->win, y, x, dec); x += 4;
    mvwaddstr(reg_win->win, y, x, hex); x += 3;
    mvwaddstr(reg_win->win, y, x, bin);
    wattroff(reg_win->win, COLOR_PAIR(CTK_COLOR_HIGHLIGHT));
}

// Draw a single 16 bit register
void draw_reg_16(reg_win_t* reg_win, uint8_t x, uint8_t y, uint16_t value) {
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

    wattron(reg_win->win, COLOR_PAIR(CTK_COLOR_HIGHLIGHT));
    mvwaddstr(reg_win->win, y, x, dec); x += 8;
    mvwaddstr(reg_win->win, y, x, hex); x += 7;
    mvwaddstr(reg_win->win, y, x, bin);
    wattroff(reg_win->win, COLOR_PAIR(CTK_COLOR_HIGHLIGHT));
}

// Draw a single 16 bit register as two separate 8 bit registers
void draw_reg_16_as_two_8(reg_win_t* reg_win, uint8_t x, uint8_t y, uint16_t value) {
    uint8_t lower;
    uint8_t higher;

    lower = (uint8_t)(value & 0xFF);
    higher = (uint8_t)(value >> 8);

    draw_reg_8(reg_win, x, y, higher);
    draw_reg_8(reg_win, x + 16, y, lower);
}

// Draw the inside borders of the register window
void reg_win_draw(reg_win_t* reg_win) {
    int x, y;

    x = 1;
    y = 1;
    wattron(reg_win->win, COLOR_PAIR(CTK_COLOR_OK));
    mvwaddstr(reg_win->win, y, x, "PC            [program counter]"); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(reg_win->win, y, x, "───────────────┬───────────────"); y++;
    mvwaddstr(reg_win->win, y, x, "A [accumulator]│F       [flags]"); y++;
    mvwaddstr(reg_win->win, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(reg_win->win, y, x, "───────────────┼───────────────"); y++;
    mvwaddstr(reg_win->win, y, x, "B     [counter]│C        [port]"); y++;
    mvwaddstr(reg_win->win, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(reg_win->win, y, x, "───────────────┼───────────────"); y++;
    mvwaddstr(reg_win->win, y, x, "D              │E              "); y++;
    mvwaddstr(reg_win->win, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(reg_win->win, y, x, "───────────────┼───────────────"); y++;
    mvwaddstr(reg_win->win, y, x, "H              │L              "); y++;
    mvwaddstr(reg_win->win, y, x, "000 00 00000000│000 00 00000000"); y++;
    mvwaddstr(reg_win->win, y, x, "───────────────┴───────────────"); y++;
    mvwaddstr(reg_win->win, y, x, "I                   [interrupt]"); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(reg_win->win, y, x, "───────────────────────────────"); y++;
    mvwaddstr(reg_win->win, y, x, "R                     [refresh]"); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(reg_win->win, y, x, "───────────────────────────────"); y++;
    mvwaddstr(reg_win->win, y, x, "SP              [stack pointer]"); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(reg_win->win, y, x, "───────────────────────────────"); y++;
    mvwaddstr(reg_win->win, y, x, "IX            IXH      IXL     "); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(reg_win->win, y, x, "───────────────────────────────"); y++;
    mvwaddstr(reg_win->win, y, x, "IY            IYH      IYL     "); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    wattroff(reg_win->win, COLOR_PAIR(CTK_COLOR_OK));

    draw_reg_16(reg_win, 1, 2, reg_win->PC);
    draw_reg_16_as_two_8(reg_win, 1, 5, reg_win->AF);
    draw_reg_16_as_two_8(reg_win, 1, 8, reg_win->BC);
    draw_reg_16_as_two_8(reg_win, 1, 11, reg_win->DE);
    draw_reg_16_as_two_8(reg_win, 1, 14, reg_win->HL);
    draw_reg_16(reg_win, 1, 17, reg_win->I);
    draw_reg_16(reg_win, 1, 20, reg_win->R);
    draw_reg_16(reg_win, 1, 23, reg_win->SP);
    draw_reg_16(reg_win, 1, 26, reg_win->IX);
    draw_reg_16(reg_win, 1, 29, reg_win->IY);
}

void reg_win_draw_new(reg_win_t* reg_win) {
    int x, y;

    x = 1;
    y = 1;
    wattron(reg_win->win, COLOR_PAIR(CTK_COLOR_OK));
    mvwaddstr(reg_win->win, y, x, "PC            [program counter]"); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(reg_win->win, y, x, "                               "); y++;
    mvwaddstr(reg_win->win, y, x, "A [accumulator] F       [flags]"); y++;
    mvwaddstr(reg_win->win, y, x, "000 00 00000000 000 00 00000000"); y++;
    mvwaddstr(reg_win->win, y, x, "                                "); y++;
    mvwaddstr(reg_win->win, y, x, "B     [counter] C        [port]"); y++;
    mvwaddstr(reg_win->win, y, x, "000 00 00000000 000 00 00000000"); y++;
    mvwaddstr(reg_win->win, y, x, "                               "); y++;
    mvwaddstr(reg_win->win, y, x, "D               E              "); y++;
    mvwaddstr(reg_win->win, y, x, "000 00 00000000 000 00 00000000"); y++;
    mvwaddstr(reg_win->win, y, x, "                               "); y++;
    mvwaddstr(reg_win->win, y, x, "H               L              "); y++;
    mvwaddstr(reg_win->win, y, x, "000 00 00000000 000 00 00000000"); y++;
    mvwaddstr(reg_win->win, y, x, "                               "); y++;
    mvwaddstr(reg_win->win, y, x, "I                   [interrupt]"); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(reg_win->win, y, x, "                               "); y++;
    mvwaddstr(reg_win->win, y, x, "R                     [refresh]"); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(reg_win->win, y, x, "                               "); y++;
    mvwaddstr(reg_win->win, y, x, "SP              [stack pointer]"); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(reg_win->win, y, x, "                               "); y++;
    mvwaddstr(reg_win->win, y, x, "IX            IXH      IXL     "); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    mvwaddstr(reg_win->win, y, x, "                               "); y++;
    mvwaddstr(reg_win->win, y, x, "IY            IYH      IYL     "); y++;
    mvwaddstr(reg_win->win, y, x, "00000 - 0000 - 0000000000000000"); y++;
    wattroff(reg_win->win, COLOR_PAIR(CTK_COLOR_OK));

    draw_reg_16(reg_win, 1, 2, reg_win->PC);
    draw_reg_16_as_two_8(reg_win, 1, 5, reg_win->AF);
    draw_reg_16_as_two_8(reg_win, 1, 8, reg_win->BC);
    draw_reg_16_as_two_8(reg_win, 1, 11, reg_win->DE);
    draw_reg_16_as_two_8(reg_win, 1, 14, reg_win->HL);
    draw_reg_16(reg_win, 1, 17, reg_win->I);
    draw_reg_16(reg_win, 1, 20, reg_win->R);
}

void reg_win_select_window(reg_win_t* reg_win) {
    wbkgd(reg_win->win, COLOR_PAIR(CTK_COLOR_HIGHLIGHT));
}

void reg_win_unselect_window(reg_win_t* reg_win) {
    wbkgd(reg_win->win, COLOR_PAIR(CTK_COLOR_WINDOW));
}
