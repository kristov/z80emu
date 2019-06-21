#ifndef MEM_WIN_H
#define MEM_WIN_H

#include <stdint.h>
#include <ctk.h>

void mem_win_destroy();

void mem_win_init(ctk_ctx_t* ctx, uint8_t width, uint8_t height, uint8_t x, uint8_t y);

void mem_win_draw(uint8_t* memory, uint16_t pc);

void mem_win_select_window();

void mem_win_unselect_window();

#endif
