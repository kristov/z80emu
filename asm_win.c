#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "asm_win.h"

void asm_win_destroy(asm_win_t* asm_win) {
    delwin(asm_win->win);
}

void asm_win_init(asm_win_t* asm_win, uint8_t width, uint8_t height, uint8_t x, uint8_t y) {
    if (asm_win->win != NULL) {
        delwin(asm_win->win);
    }
    if (asm_win->history != NULL) {
        free(asm_win->history);
    }
    asm_win->width = width - 2;
    asm_win->height = height - 2;
    asm_win->next_history = 0;
    asm_win->history = malloc(sizeof(char) * 20 * height);
    asm_win->win = newwin(height, width, y, x);
    box(asm_win->win, 0, 0);
    wbkgd(asm_win->win, COLOR_PAIR(3));
}

void asm_win_draw(asm_win_t* asm_win, char* asm_before, char* asm_after) {
    int len = strlen(asm_before);
    if (len > 20) {
        len = 20;
    }
    char* dest = &asm_win->history[asm_win->next_history * 20];
    memset(dest, 0, 20);
    memcpy(dest, asm_before, len);
    dest[len] = '\0';
    asm_win->next_history++;
    if (asm_win->next_history >= asm_win->height) {
        asm_win->next_history = 0;
    }
    mvwprintw(asm_win->win, 1, 1, "%d  ", len);
    for (uint16_t i = 0; i < asm_win->height; i++) {
        mvwaddstr(asm_win->win, i + 1, 1, "                    ");
        mvwprintw(asm_win->win, i + 1, 1, "%s", &asm_win->history[i * 20]);
    }
}

void asm_win_select_window(asm_win_t* asm_win) {
    wbkgd(asm_win->win, COLOR_PAIR(4));
}

void asm_win_unselect_window(asm_win_t* asm_win) {
    wbkgd(asm_win->win, COLOR_PAIR(3));
}
