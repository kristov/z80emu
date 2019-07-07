#include <ncurses.h>
#include "reg_win.h"
#include "ctk.h"

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

    ctk_addstr(window, x, y, 1, dec); x += 4;
    ctk_addstr(window, x, y, 1, hex); x += 3;
    ctk_addstr(window, x, y, 1, bin);
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

    ctk_addstr(window, x, y, 1, dec); x += 8;
    ctk_addstr(window, x, y, 1, hex); x += 7;
    ctk_addstr(window, x, y, 1, bin);
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

void reg_win_draw(ctk_widget_t* window, reg_win_t* reg_win) {
    int x, y;
    x = 0;
    y = 0;
    ctk_addstr(window, x, y, CTK_COLOR_OK, "PC            [program counter]"); y++;
    draw_reg_16(window, 0, y, reg_win->PC); y++;
    ctk_addstr(window, x, y, CTK_COLOR_OK, "A [accumulator] F       [flags]"); y++;
    draw_reg_16_as_two_8(window, 0, y, reg_win->AF); y++;
    ctk_addstr(window, x, y, CTK_COLOR_OK, "B     [counter] C        [port]"); y++;
    draw_reg_16_as_two_8(window, 0, y, reg_win->BC); y++;
    ctk_addstr(window, x, y, CTK_COLOR_OK, "D               E              "); y++;
    draw_reg_16_as_two_8(window, 0, y, reg_win->DE); y++;
    ctk_addstr(window, x, y, CTK_COLOR_OK, "H               L              "); y++;
    draw_reg_16_as_two_8(window, 0, y, reg_win->HL); y++;
    ctk_addstr(window, x, y, CTK_COLOR_OK, "I                   [interrupt]"); y++;
    draw_reg_16(window, 0, y, reg_win->I); y++;
    ctk_addstr(window, x, y, CTK_COLOR_OK, "R                     [refresh]"); y++;
    draw_reg_16(window, 0, y, reg_win->R); y++;
    ctk_addstr(window, x, y, CTK_COLOR_OK, "SP              [stack pointer]"); y++;
    draw_reg_16(window, 0, y, reg_win->SP); y++;
    ctk_addstr(window, x, y, CTK_COLOR_OK, "IXH             IXL            "); y++;
    draw_reg_16_as_two_8(window, 0, y, reg_win->IX); y++;
    ctk_addstr(window, x, y, CTK_COLOR_OK, "IYH             IYL            "); y++;
    draw_reg_16_as_two_8(window, 0, y, reg_win->IY); y++;
}

void reg_win_select_window(ctk_widget_t* window) {
}

void reg_win_unselect_window(ctk_widget_t* window) {
}
