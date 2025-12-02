// display_matrix.cpp - main game loop and input handling

#include "game_classes.h"
#include "pico/stdlib.h"
#include <cstdio>

// Read non-blocking arrow input (as before)
enum Arrow { ARROW_NONE, ARROW_LEFT, ARROW_RIGHT };

static Arrow read_arrow() {
    int c = getchar_timeout_us(0);
    if (c < 0) return ARROW_NONE;
    if (c == 27) {
        int c2 = getchar_timeout_us(2000);
        if (c2 == '[') {
            int c3 = getchar_timeout_us(2000);
            if (c3 == 'C') return ARROW_RIGHT;
            if (c3 == 'D') return ARROW_LEFT;
        }
        return ARROW_NONE;
    }
    if (c == 'a' || c == 'A' || c == 'j' || c == 'J') return ARROW_LEFT;
    if (c == 'd' || c == 'D' || c == 'l' || c == 'L') return ARROW_RIGHT;
    return ARROW_NONE;
}

int main() {
    stdio_init_all();

    // init LCD before the game (BrickBreaker calls lcd_print_score during init)
    lcd_init_display();

    Hub75Matrix matrix;
    BrickBreaker game(matrix);

    const uint32_t physics_interval_ms = 40; // ~25Hz
    uint32_t last_physics = to_ms_since_boot(get_absolute_time());

    Arrow last_arrow = ARROW_NONE;
    uint32_t last_key_time = 0;
    const uint32_t key_hold_timeout_ms = 160;

    while (true) {
        game.render();
        matrix.refresh_once();

        Arrow a = read_arrow();
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (a != ARROW_NONE) {
            if (a == ARROW_LEFT) game.move_paddle_left();
            else if (a == ARROW_RIGHT) game.move_paddle_right();
            last_arrow = a;
            last_key_time = now;
        }

        if (now - last_physics >= physics_interval_ms) {
            if (last_arrow != ARROW_NONE && (now - last_key_time) < key_hold_timeout_ms) {
                if (last_arrow == ARROW_LEFT) game.move_paddle_left();
                else if (last_arrow == ARROW_RIGHT) game.move_paddle_right();
            }
            game.update_physics();
            last_physics = now;
        }
    }

    return 0;
}
