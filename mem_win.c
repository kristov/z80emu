#include <ncurses.h>
#include "mem_win.h"

void mem_win_destroy(mem_win_t* mem_win) {
    delwin(mem_win->win);
}

void mem_win_init(mem_win_t* mem_win, uint8_t width, uint8_t height, uint8_t x, uint8_t y) {
    if (mem_win->win != NULL) {
        delwin(mem_win->win);
    }
    mem_win->win = newwin(height, width, y, x);
    box(mem_win->win, 0, 0);
    wbkgd(mem_win->win, COLOR_PAIR(3));
}

void mem_win_draw(mem_win_t* mem_win, uint8_t* memory, uint16_t pc) {
    char hex[3];

    uint8_t viz_width = mem_win->width - 2;
    uint8_t viz_height = mem_win->height - 2;
    uint8_t nr_bytes_across = viz_width / 3;

    hex[2] = '\0';

    uint8_t ins_on_line = pc / nr_bytes_across;
    uint16_t start_addr = 0;
    if (ins_on_line >= viz_height) {
        start_addr = (nr_bytes_across * (ins_on_line - 2));
    }

    uint8_t b_count = 1;
    uint8_t x = 1;
    uint8_t y = 1;

    for (uint16_t i = start_addr; i < 65536; i++) {
        sprintf(hex, "%02x", memory[i]);
        if (i == pc) {
            wattron(mem_win->win, COLOR_PAIR(7));
            mvwaddstr(mem_win->win, y, x, hex);
            wattroff(mem_win->win, COLOR_PAIR(7));
        }
        else {
            wattron(mem_win->win, COLOR_PAIR(6));
            mvwaddstr(mem_win->win, y, x, hex);
            wattroff(mem_win->win, COLOR_PAIR(6));
        }
        x += 3;
        b_count++;
        if (b_count > nr_bytes_across) {
            b_count = 1;
            x = 1;
            y++;
        }
        if (y > viz_height) {
            break;
        }
    }
}

void mem_win_select_window(mem_win_t* mem_win) {
    wbkgd(mem_win->win, COLOR_PAIR(4));
}

void mem_win_unselect_window(mem_win_t* mem_win) {
    wbkgd(mem_win->win, COLOR_PAIR(3));
}
