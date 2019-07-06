#include <ncurses.h>
#include "reg_win.h"
#include "ctk.h"

/*
void draw_reg_8(ctk_widget_t* window, uint8_t x, uint8_t y, uint8_t value) {
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
void draw_reg_16(ctk_widget_t* window, uint8_t x, uint8_t y, uint16_t value) {
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
void draw_reg_16_as_two_8(ctk_widget_t* window, uint8_t x, uint8_t y, uint16_t value) {
    uint8_t lower;
    uint8_t higher;

    lower = (uint8_t)(value & 0xFF);
    higher = (uint8_t)(value >> 8);

    draw_reg_8(window, x, y, higher);
    draw_reg_8(window, x + 16, y, lower);
}
*/

void reg_win_draw(ctk_widget_t* window, reg_win_t* reg_win) {
    int x, y;
    x = 0;
    y = 0;
    ctk_addstr(window, x, y, 1, "PC            [program counter]"); y++;
    ctk_addstr(window, x, y, 1, "00000 - 0000 - 0000000000000000"); y++;
    ctk_addstr(window, x, y, 1, "                               "); y++;
    ctk_addstr(window, x, y, 1, "A [accumulator] F       [flags]"); y++;
    ctk_addstr(window, x, y, 1, "000 00 00000000 000 00 00000000"); y++;
    ctk_addstr(window, x, y, 1, "                               "); y++;
    ctk_addstr(window, x, y, 1, "B     [counter] C        [port]"); y++;
    ctk_addstr(window, x, y, 1, "000 00 00000000 000 00 00000000"); y++;
    ctk_addstr(window, x, y, 1, "                               "); y++;
    ctk_addstr(window, x, y, 1, "D               E              "); y++;
    ctk_addstr(window, x, y, 1, "000 00 00000000 000 00 00000000"); y++;
    ctk_addstr(window, x, y, 1, "                               "); y++;
    ctk_addstr(window, x, y, 1, "H               L              "); y++;
    ctk_addstr(window, x, y, 1, "000 00 00000000 000 00 00000000"); y++;
    ctk_addstr(window, x, y, 1, "                               "); y++;
    ctk_addstr(window, x, y, 1, "I                   [interrupt]"); y++;
    ctk_addstr(window, x, y, 1, "00000 - 0000 - 0000000000000000"); y++;
    ctk_addstr(window, x, y, 1, "                               "); y++;
    ctk_addstr(window, x, y, 1, "R                     [refresh]"); y++;
    ctk_addstr(window, x, y, 1, "00000 - 0000 - 0000000000000000"); y++;
    ctk_addstr(window, x, y, 1, "                               "); y++;
    ctk_addstr(window, x, y, 1, "SP              [stack pointer]"); y++;
    ctk_addstr(window, x, y, 1, "00000 - 0000 - 0000000000000000"); y++;
    ctk_addstr(window, x, y, 1, "                               "); y++;
    ctk_addstr(window, x, y, 1, "IX            IXH      IXL     "); y++;
    ctk_addstr(window, x, y, 1, "00000 - 0000 - 0000000000000000"); y++;
    ctk_addstr(window, x, y, 1, "                               "); y++;
    ctk_addstr(window, x, y, 1, "IY            IYH      IYL     "); y++;
    ctk_addstr(window, x, y, 1, "00000 - 0000 - 0000000000000000"); y++;
    //draw_reg_16(window, 1, 2, reg_win->PC);
    //draw_reg_16_as_two_8(window, 1, 5, reg_win->AF);
    //draw_reg_16_as_two_8(window, 1, 8, reg_win->BC);
    //draw_reg_16_as_two_8(window, 1, 11, reg_win->DE);
    //draw_reg_16_as_two_8(window, 1, 14, reg_win->HL);
    //draw_reg_16(window, 1, 17, reg_win->I);
    //draw_reg_16(window, 1, 20, reg_win->R);
}

void reg_win_select_window(ctk_widget_t* window) {
}

void reg_win_unselect_window(ctk_widget_t* window) {
}
