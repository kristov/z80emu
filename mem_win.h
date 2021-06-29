#ifndef MEM_WIN_H
#define MEM_WIN_H

#include <stdint.h>
#include <ctk.h>

typedef struct mem_win {
    uint8_t* memory;
    uint16_t pc;
    uint16_t page_offset;
} mem_win_t;

void mem_win_init(mem_win_t* mem_win, uint8_t* memory);

void mem_win_draw(ctk_widget_t* window, mem_win_t* mem_win);

void mem_win_pageup(mem_win_t* mem_win);

void mem_win_pagedown(mem_win_t* mem_win);

void mem_win_select_window();

void mem_win_unselect_window();

#endif
