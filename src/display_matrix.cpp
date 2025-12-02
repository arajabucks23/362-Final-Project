// display_matrix.cpp - main game loop and input handling

#include "game_classes.h"
#include "pico/stdlib.h"
#include <cstdio>
#include <cstring>

// Keypad GPIO mapping - UPDATE these to match your wiring
// Columns: outputs; Rows: inputs
static const uint8_t KEYPAD_COLS[4] = {2, 3, 4, 5};
static const uint8_t KEYPAD_ROWS[4] = {6, 7, 8, 9};

// Keymap matching the attached schematic:
// Row0: 1 2 3 A
// Row1: 4 5 6 B
// Row2: 7 8 9 C
// Row3: * 0 # D
static const char KEYPAD_MAP[4][4] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

// Centered text color (choose a single color for all text)
static const uint8_t TEXT_R = 0;
static const uint8_t TEXT_G = 200;
static const uint8_t TEXT_B = 200;

// Initialize keypad pins (call once)
static void keypad_init() {
    for (int c = 0; c < 4; ++c) {
        gpio_init(KEYPAD_COLS[c]);
        gpio_set_dir(KEYPAD_COLS[c], GPIO_OUT);
        gpio_put(KEYPAD_COLS[c], 1);
    }
    for (int r = 0; r < 4; ++r) {
        gpio_init(KEYPAD_ROWS[r]);
        gpio_set_dir(KEYPAD_ROWS[r], GPIO_IN);
        gpio_pull_up(KEYPAD_ROWS[r]);
    }
}

// Scan keypad and return 0 if none pressed, or the char
static char keypad_scan() {
    for (int c = 0; c < 4; ++c) {
        // set all cols high
        for (int j = 0; j < 4; ++j) gpio_put(KEYPAD_COLS[j], 1);
        // pull this column low
        gpio_put(KEYPAD_COLS[c], 0);
        busy_wait_us_32(2000);
        for (int r = 0; r < 4; ++r) {
            int v = gpio_get(KEYPAD_ROWS[r]);
            // rows use pull-ups, so pressed will read 0 when column is low
            if (v == 0) {
                // simple debounce: wait a bit and confirm
                busy_wait_us_32(20000);
                if (gpio_get(KEYPAD_ROWS[r]) == 0) return KEYPAD_MAP[r][c];
            }
        }
    }
    return 0;
}

// Simple 5x7 font for uppercase letters and space (subset). Each glyph is 5 cols, LSB top->bottom bits.
// Indices: 0=space,1=A,2=D,3=E,4=H,5=M,6=R,7=S,8=Y,9=I,10=U
static const uint8_t FONT5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // space
    {0x7C,0x12,0x11,0x12,0x7C}, // A
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x7F,0x06,0x18,0x06,0x7F}, // M
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x3F,0x40,0x40,0x40,0x3F}  // U
};

// Helper to map a supported uppercase char to FONT5x7 index
static int font_index_for_char(char ch) {
    if (ch == ' ') return 0;
    if (ch == 'A') return 1;
    if (ch == 'D') return 2;
    if (ch == 'E') return 3;
    if (ch == 'H') return 4;
    if (ch == 'M') return 5;
    if (ch == 'R') return 6;
    if (ch == 'S') return 7;
    if (ch == 'Y') return 8;
    if (ch == 'I') return 9;
    if (ch == 'U') return 10;
    return 0; // fallback to space for unsupported
}

// Render small centered text (uppercase only, limited charset). Blocks for duration_ms milliseconds.
// This version prefers single-line with no inter-character spacing; if that still
// doesn't fit it attempts compact 4-column glyphs; only then it falls back to two-lines.
static void show_centered_text(Hub75Matrix &matrix, const char *text, uint32_t duration_ms) {
    int len = (int)strlen(text);
    int glyph_w = 5;
    int spacing = 0; // no spacing to maximize fit
    int total_w = len * glyph_w + (len - 1) * spacing;

    // if full-width fits -> use it
    bool use_compact = false;
    if (total_w <= 32) {
        // ok
    } else {
        // try compact 4-column glyphs (no spacing)
        int compact_gw = 4;
        int compact_total = len * compact_gw;
        if (compact_total <= 32) {
            glyph_w = compact_gw;
            spacing = 0;
            total_w = compact_total;
            use_compact = true;
        }
    }

    // If still too wide, split into two roughly-equal lines
    bool two_line = (total_w > 32);
    char tmp1[32];
    char tmp2[32];
    const char *line1 = text;
    const char *line2 = nullptr;
    int len1 = len;
    int len2 = 0;
    if (two_line) {
        int split = (len + 1) / 2;
        len1 = split;
        len2 = len - split;
        memcpy(tmp1, text, len1); tmp1[len1] = '\0';
        memcpy(tmp2, text + split, len2); tmp2[len2] = '\0';
        line1 = tmp1;
        line2 = tmp2;
        // recalc widths per-line
        int total_w1 = len1 * glyph_w + (len1 - 1) * spacing;
        int total_w2 = len2 * glyph_w + (len2 - 1) * spacing;
        // if either line still too wide, force compact
        if (total_w1 > 32 || total_w2 > 32) {
            glyph_w = 4;
            spacing = 0;
        }
    }

    uint32_t end = to_ms_since_boot(get_absolute_time()) + duration_ms;
    while ((int32_t)(end - to_ms_since_boot(get_absolute_time())) > 0) {
        matrix.clear();
        if (!two_line) {
            int x0 = (32 - total_w) / 2;
            int y0 = (32 - 7) / 2;
            int x = x0;
            for (int i = 0; i < len; ++i) {
                char ch = text[i];
                int idx = font_index_for_char(ch);
                const uint8_t *g = FONT5x7[idx];
                for (int col = 0; col < glyph_w; ++col) {
                    uint8_t colbits = g[col];
                    for (int row = 0; row < 7; ++row) {
                        if (colbits & (1 << row)) matrix.set_pixel(x + col, y0 + row, TEXT_R, TEXT_G, TEXT_B);
                    }
                }
                x += glyph_w + spacing;
            }
        } else {
            int total_w1 = len1 * glyph_w + (len1 - 1) * spacing;
            int total_w2 = len2 * glyph_w + (len2 - 1) * spacing;
            int x01 = (32 - total_w1) / 2;
            int x02 = (32 - total_w2) / 2;
            int line_height = 7;
            int vertical_spacing = 1;
            int total_h = line_height * 2 + vertical_spacing;
            int y_top = (32 - total_h) / 2;

            int x = x01;
            for (int i = 0; i < len1; ++i) {
                char ch = line1[i];
                int idx = font_index_for_char(ch);
                const uint8_t *g = FONT5x7[idx];
                for (int col = 0; col < glyph_w; ++col) {
                    uint8_t colbits = g[col];
                    for (int row = 0; row < 7; ++row) {
                        if (colbits & (1 << row)) matrix.set_pixel(x + col, y_top + row, TEXT_R, TEXT_G, TEXT_B);
                    }
                }
                x += glyph_w + spacing;
            }

            x = x02;
            for (int i = 0; i < len2; ++i) {
                char ch = line2[i];
                int idx = font_index_for_char(ch);
                const uint8_t *g = FONT5x7[idx];
                for (int col = 0; col < glyph_w; ++col) {
                    uint8_t colbits = g[col];
                    for (int row = 0; row < 7; ++row) {
                        if (colbits & (1 << row)) matrix.set_pixel(x + col, y_top + line_height + vertical_spacing + row, TEXT_R, TEXT_G, TEXT_B);
                    }
                }
                x += glyph_w + spacing;
            }
        }
        matrix.refresh_once();
    }
}

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

    // initialize keypad pins
    keypad_init();

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

        // keypad handling (polling)
        char k = keypad_scan();
        if (k) {
            if (k == 'A') {
                // easy
                show_centered_text(matrix, "EASY", 1000);
                game.set_difficulty(BrickBreaker::EASY);
            } else if (k == 'B') {
                show_centered_text(matrix, "MEDIUM", 1000);
                game.set_difficulty(BrickBreaker::MEDIUM);
            } else if (k == 'C') {
                show_centered_text(matrix, "HARD", 1000);
                game.set_difficulty(BrickBreaker::HARD);
            }
        }

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
