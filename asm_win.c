#include <ncurses.h>
#include "asm_win.h"

void asm_win_destroy(asm_win_t* asm_win) {
    delwin(asm_win->win);
}

void asm_win_init(asm_win_t* asm_win, uint8_t width, uint8_t height, uint8_t x, uint8_t y) {
    if (asm_win->win != NULL) {
        delwin(asm_win->win);
    }
    asm_win->win = newwin(height, width, y, x);
    box(asm_win->win, 0, 0);
    wbkgd(asm_win->win, COLOR_PAIR(3));
}

void asm_win_draw(asm_win_t* asm_win, char* asm_before, char* asm_after) {
    mvwaddstr(asm_win->win, 2, 1, "B               ");
    mvwaddstr(asm_win->win, 2, 3, asm_before);
    mvwaddstr(asm_win->win, 1, 1, "A               ");
    mvwaddstr(asm_win->win, 1, 3, asm_after);
}

void asm_win_select_window(asm_win_t* asm_win) {
    wbkgd(asm_win->win, COLOR_PAIR(4));
}

void asm_win_unselect_window(asm_win_t* asm_win) {
    wbkgd(asm_win->win, COLOR_PAIR(3));
}
