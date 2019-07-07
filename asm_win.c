#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "asm_win.h"
#include "ctk.h"

void asm_win_init(asm_win_t* asm_win, uint8_t height) {
    if (asm_win->history != NULL) {
        free(asm_win->history);
    }
    asm_win->next_history = 0;
    asm_win->height = height;
    asm_win->history = malloc(sizeof(char) * 20 * height);
}

void asm_win_draw(ctk_widget_t* window, asm_win_t* asm_win, char* asm_before, char* asm_after) {
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
    ctk_printf(window, 0, 0, 1, "%d  ", len);
    uint16_t top = asm_win->next_history;
    for (uint16_t i = 0; i < asm_win->height; i++) {
        ctk_addstr(window, 0, i, 1, "                    ");
        ctk_printf(window, 0, i, 1, "%s", &asm_win->history[top * 20]);
        top++;
        if (top >= asm_win->height) {
            top = 0;
        }
    }
}
