#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "asm_win.h"
#include "ctk.h"

void asm_win_init(asm_win_t* asm_win, uint8_t width, uint8_t height) {
    if (asm_win->history != NULL) {
        free(asm_win->history);
    }
    asm_win->next_history = 0;
    asm_win->width = width;
    asm_win->height = height;
    asm_win->history = malloc(sizeof(char) * asm_win->width * height);
}

void asm_win_draw(ctk_widget_t* window, asm_win_t* asm_win, char* asm_before, char* asm_after) {
    int len = strlen(asm_before);
    if (len > asm_win->width) {
        len = asm_win->width;
    }
    char* dest = &asm_win->history[asm_win->next_history * asm_win->width];
    memset(dest, 0, asm_win->width);
    memcpy(dest, asm_before, len);
    dest[asm_win->width - 1] = '\0';
    asm_win->next_history++;
    if (asm_win->next_history >= asm_win->height) {
        asm_win->next_history = 0;
    }
    ctk_printf(window, 0, 0, 1, "%d  ", len);
    uint16_t top = asm_win->next_history;
    for (uint16_t y = 0; y < asm_win->height; y++) {
        //for (uint8_t x = 0; x < asm_win->width; x++) {
        //    ctk_addch(window, x, y, 1, '.');
        //
        ctk_printf(window, 0, y, 1, "%s", &asm_win->history[top * asm_win->width]);
        top++;
        if (top >= asm_win->height) {
            top = 0;
        }
    }
}
