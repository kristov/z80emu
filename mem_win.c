#include "mem_win.h"
#include <ctk.h>

void mem_win_draw(ctk_widget_t* window, uint8_t* memory, uint16_t pc) {
    char addr[5];
    char hex[3];

    // 0000  01 00 08 21 00 00 11 ff  f7 ed b0 c3 ff f7 c3 ff

    addr[4] = '\0';
    hex[2] = '\0';

    uint8_t x = 0;
    uint8_t y = 0;

    for (uint16_t i = 0; i < 65536; i += 16) {
        sprintf(addr, "%04x", i);
        ctk_addstr(window, x, y, CTK_COLOR_SELECTED, addr);
        x += 6;
        for (uint8_t b = 0; b < 16; b++) {
            uint16_t ptr = i + b;
            sprintf(hex, "%02x", memory[ptr]);
            if (ptr == pc) {
                ctk_addstr(window, x, y, CTK_COLOR_SELECTED, hex);
            }
            else {
                ctk_addstr(window, x, y, CTK_COLOR_WINDOW, hex);
            }
            x += 3;
        }
        x = 0;
        y++;
        if (y > window->height) {
            break;
        }
    }
}
