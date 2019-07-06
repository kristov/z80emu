#ifndef REG_WIN_H
#define REG_WIN_H

#include <ctk.h>

typedef struct reg_win {
    WINDOW* win;
    uint8_t width;
    uint8_t height;
    uint16_t PC;
    uint16_t AF;
    uint16_t BC;
    uint16_t DE;
    uint16_t HL;
    uint16_t R;
    uint16_t I;
    uint16_t SP;
    uint16_t IX;
    uint16_t IY;
} reg_win_t;

void reg_win_draw(ctk_widget_t* window, reg_win_t* reg_win);

#endif
