#ifndef MEM_WIN_H
#define MEM_WIN_H

#include <stdint.h>

typedef struct mem_win {
    WINDOW* win;
    uint8_t width;
    uint8_t height;
    uint8_t nr_bytes_across;
    uint8_t nr_lines;
} mem_win_t;

void mem_win_destroy(mem_win_t* mem_win);

void mem_win_init(mem_win_t* mem_win, uint8_t width, uint8_t height, uint8_t x, uint8_t y);

void mem_win_draw(mem_win_t* mem_win, uint8_t* memory, uint16_t pc);

void mem_win_select_window(mem_win_t* mem_win);

void mem_win_unselect_window(mem_win_t* mem_win);

#endif
