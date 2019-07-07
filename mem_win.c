#include "mem_win.h"
#include <ctk.h>

void mem_win_draw(ctk_widget_t* window, uint8_t* memory, uint16_t pc) {
    char hex[3];

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
