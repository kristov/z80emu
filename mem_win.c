#include <ncurses.h>
#include "mem_win.h"
#include <ctk.h>

#define MEM_HBOX 0
#define MEM_VOID 1
#define MEM_WINDOW 2

ctk_widget_t hbox;
ctk_widget_t widgets[3];

void mem_win_destroy() {
}

void mem_win_init(ctk_ctx_t* ctx, uint8_t width, uint8_t height, uint8_t x, uint8_t y) {
    ctk_void_init(&widgets[MEM_VOID], x, y);
    ctk_window_init(&widgets[MEM_WINDOW], width, height, NULL, 0);
    ctk_hbox_init(&widgets[MEM_HBOX], &widgets[MEM_VOID], 2);
    ctk_init_widgets(ctx, &widgets[MEM_HBOX], 1);
}

void mem_win_draw(uint8_t* memory, uint16_t pc) {
    char hex[3];
    ctk_widget_t* window = &widgets[MEM_WINDOW];

    uint8_t viz_width = window->width;
    uint8_t viz_height = window->height;
    uint8_t nr_bytes_across = viz_width / 3;

    hex[2] = '\0';

    uint8_t ins_on_line = pc / nr_bytes_across;
    uint16_t start_addr = 0;
    if (ins_on_line >= viz_height) {
        start_addr = (nr_bytes_across * (ins_on_line - 2));
    }

    uint8_t b_count = 1;
    uint8_t x = 0;
    uint8_t y = 0;

    for (uint16_t i = start_addr; i < 65536; i++) {
        sprintf(hex, "%02x", memory[i]);
        if (i == pc) {
            ctk_addstr(window, x, y, CTK_COLOR_SELECTED, hex);
        }
        else {
            ctk_addstr(window, x, y, CTK_COLOR_WINDOW, hex);
        }
        x += 3;
        b_count++;
        if (b_count > nr_bytes_across) {
            b_count = 1;
            x = 0;
            y++;
        }
        if (y > viz_height) {
            break;
        }
    }
}

void mem_win_select_window() {
    //wbkgd(mem_win->win, COLOR_PAIR(CTK_COLOR_HIGHLIGHT));
}

void mem_win_unselect_window() {
    //wbkgd(mem_win->win, COLOR_PAIR(CTK_COLOR_WINDOW));
}
