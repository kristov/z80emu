#ifndef ASM_WIN_H
#define ASM_WIN_H

#include <stdint.h>

typedef struct asm_win {
    WINDOW* win;
    uint8_t width;
    uint8_t height;
} asm_win_t;

void asm_win_destroy(asm_win_t* asm_win);

void asm_win_init(asm_win_t* asm_win, uint8_t width, uint8_t height, uint8_t x, uint8_t y);

void asm_win_draw(asm_win_t* asm_win, char* asm_before, char* asm_after);

void asm_win_select_window(asm_win_t* asm_win);

void asm_win_unselect_window(asm_win_t* asm_win);

#endif
