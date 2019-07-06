#ifndef MEM_WIN_H
#define MEM_WIN_H

#include <stdint.h>
#include <ctk.h>

void mem_win_draw(ctk_widget_t* window, uint8_t* memory, uint16_t pc);

void mem_win_select_window();

void mem_win_unselect_window();

#endif
