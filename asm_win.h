#ifndef ASM_WIN_H
#define ASM_WIN_H

#include <ctk.h>

typedef struct asm_win {
    uint8_t height;
    uint8_t width;
    char* history;
    uint8_t next_history;
} asm_win_t;

void asm_win_init(asm_win_t* asm_win, uint8_t width, uint8_t height);

void asm_win_draw(ctk_widget_t* window, asm_win_t* asm_win, char* asm_before, char* asm_after);

#endif
