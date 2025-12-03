#include "game_classes.h"
#include <cstring>

// Hub75Matrix implementations
Hub75Matrix::Hub75Matrix() {
    const uint pins[] = {PIN_R1,PIN_G1,PIN_B1,PIN_R2,PIN_G2,PIN_B2,PIN_A,PIN_B,PIN_C,PIN_D,PIN_CLK,PIN_OE,PIN_LAT};
    for (auto p : pins) {
        gpio_init(p);
        gpio_set_dir(p, GPIO_OUT);
    }

    // initial states: blank panel
    gpio_clr_mask(DATA_MASK);
    gpio_clr_mask(M_CLK | M_LAT);
    gpio_set_mask(M_OE); // OE=1 -> outputs disabled

    clear();
}

void Hub75Matrix::set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= 32 || y < 0 || y >= 32) return;
    fb[y][x][0] = r;
    fb[y][x][1] = g;
    fb[y][x][2] = b;
}

void Hub75Matrix::clear() {
    memset(fb, 0, sizeof(fb));
}

void Hub75Matrix::refresh_once() {
    for (int plane = BITPLANES - 1; plane >= 0; --plane) {
        int us = (1 << plane) * DWELL_SCALE;
        for (int row = 0; row < 16; ++row) {
            gpio_set_mask(M_OE);
            set_row_address(row);
            for (int col = 0; col < 32; ++col) {
                uint8_t r1 = (fb[row][col][0]  >> plane) & 1;
                uint8_t g1 = (fb[row][col][1]  >> plane) & 1;
                uint8_t b1 = (fb[row][col][2]  >> plane) & 1;
                uint8_t r2 = (fb[row+16][col][0] >> plane) & 1;
                uint8_t g2 = (fb[row+16][col][1] >> plane) & 1;
                uint8_t b2 = (fb[row+16][col][2] >> plane) & 1;

                uint32_t set_mask = 0;
                if (r1) set_mask |= M_R1;
                if (g1) set_mask |= M_G1;
                if (b1) set_mask |= M_B1;
                if (r2) set_mask |= M_R2;
                if (g2) set_mask |= M_G2;
                if (b2) set_mask |= M_B2;

                gpio_clr_mask(DATA_MASK);
                if (set_mask) gpio_set_mask(set_mask);

                gpio_set_mask(M_CLK);
                gpio_clr_mask(M_CLK);
            }

            gpio_set_mask(M_LAT);
            busy_wait_us_32(1);
            gpio_clr_mask(M_LAT);

            gpio_clr_mask(M_OE);
            busy_wait_us_32(us);

            gpio_set_mask(M_OE);
        }
    }
}

void Hub75Matrix::set_row_address(int row) {
    uint32_t masks_to_clear = (1u<<PIN_A)|(1u<<PIN_B)|(1u<<PIN_C)|(1u<<PIN_D);
    uint32_t masks_to_set = 0;
    if (row & 0x1) masks_to_set |= (1u<<PIN_A);
    if (row & 0x2) masks_to_set |= (1u<<PIN_B);
    if (row & 0x4) masks_to_set |= (1u<<PIN_C);
    if (row & 0x8) masks_to_set |= (1u<<PIN_D);

    gpio_clr_mask(masks_to_clear);
    if (masks_to_set) gpio_set_mask(masks_to_set);
}

// Brick implementation
void Brick::hit() {
    if (!alive) return;
    alive = false;
}

// BrickBreaker implementations
BrickBreaker::BrickBreaker(Hub75Matrix &matrix) : m(matrix) {
    paddle_w = 5;
    paddle_h = 2;
    paddle_x = 13;
    paddle_y = HEIGHT - paddle_h;

    score = 0;
    level = 1;
    lives = 3;
    game_over = false;
    level_cleared = false;
    difficulty = EASY;
    speed_scale = 1.0f; // default to easy -> will be adjusted by set_difficulty
    init_bricks_for_level();
    reset();
}

void BrickBreaker::set_difficulty(Difficulty d) {
    difficulty = d;
    switch (d) {
        case EASY: speed_scale = 0.5f; break;
        case MEDIUM: speed_scale = 1.0f; break;
        case HARD: speed_scale = 1.8f; break;
    }
    // apply immediately by resetting ball/paddle to new speed
    reset();
}

void BrickBreaker::init_bricks_for_level() {
    int total_bricks_width = brick_cols * brick_w + (brick_cols - 1) * brick_spacing;
    int left_margin = (WIDTH - total_bricks_width) / 2;
    int top_margin = 1;

    for (int row = 0; row < brick_rows; ++row) {
        int row_offset = (row % 2 == 0) ? 0 : (brick_w / 2);
        for (int col = 0; col < brick_cols; ++col) {
            int idx = row * brick_cols + col;
            int bx = left_margin + row_offset + col * (brick_w + brick_spacing);
            int by = top_margin + row * (brick_h + brick_spacing);
            bricks[idx].x = bx;
            bricks[idx].y = by;
            bricks[idx].alive = true;
            bricks[idx].hits = 1;
            bricks[idx].r = 0; bricks[idx].g = 255; bricks[idx].b = 0;
        }
    }
    lcd_print_score(score, level);
}

void BrickBreaker::reset() {
    paddle_x = (WIDTH - paddle_w) / 2;
    ball_x = paddle_x + (paddle_w - 2) / 2;
    ball_y = paddle_y - 3;
    // base velocities scaled by difficulty (increased to make differences clearer)
    float base_vx = 1.0f;
    float base_vy = -1.4f;
    ball_vx = base_vx * speed_scale;
    ball_vy = base_vy * speed_scale;
}

void BrickBreaker::reset_game() {
    score = 0;
    level = 1;
    lives = 3;
    game_over = false;
    level_cleared = false;
    set_difficulty(difficulty); // reapply speed_scale and reset ball/paddle
    init_bricks_for_level();
    reset();
}

void BrickBreaker::mark_level_cleared() {
    level_cleared = true;
}

void BrickBreaker::advance_level() {
    level_cleared = false;
    level++;
    init_bricks_for_level();
    reset();
}

void BrickBreaker::move_paddle_left() {
    if (paddle_x > 0) paddle_x -= 1;
}
void BrickBreaker::move_paddle_right() {
    if (paddle_x + paddle_w < WIDTH) paddle_x += 1;
}

void BrickBreaker::update_physics() {
    float next_x = ball_x + ball_vx;
    float next_y = ball_y + ball_vy;

    if (next_x < 0) { next_x = 0; ball_vx = -ball_vx; }
    if (next_x + 2 > WIDTH) { next_x = WIDTH - 2; ball_vx = -ball_vx; }
    if (next_y < 0) { next_y = 0; ball_vy = -ball_vy; }

    int ball_left = (int)next_x;
    int ball_right = (int)next_x + 1;
    int ball_top = (int)next_y;
    int ball_bottom = (int)next_y + 1;

    if (ball_bottom >= paddle_y && ball_top <= paddle_y + paddle_h - 1) {
        if (!(ball_right < paddle_x || ball_left > paddle_x + paddle_w - 1)) {
            next_y = paddle_y - 2;
            ball_vy = - (ball_vy < 0 ? -ball_vy : ball_vy);
            float hit_pos = ((next_x + 1) - paddle_x) - (paddle_w / 2.0f);
            ball_vx += hit_pos * 0.15f;
            if (ball_vx > 2.0f) ball_vx = 2.0f;
            if (ball_vx < -2.0f) ball_vx = -2.0f;
        }
    }

    for (int i = 0; i < brick_rows * brick_cols; ++i) {
        if (!bricks[i].alive) continue;
        float bx0 = (float)bricks[i].x;
        float by0 = (float)bricks[i].y;
        float bx1 = bx0 + (float)brick_w;
        float by1 = by0 + (float)brick_h;

        float ball_x0 = next_x;
        float ball_y0 = next_y;
        float ball_x1 = next_x + 2.0f;
        float ball_y1 = next_y + 2.0f;

        bool overlap = (ball_x0 < bx1) && (ball_x1 > bx0) && (ball_y0 < by1) && (ball_y1 > by0);
        if (overlap) {
            if (bricks[i].alive) {
                score += level * 50;
                lcd_print_score(score, level);
            }
            bricks[i].hit();
            ball_vy = -ball_vy;

            bool any = false;
            for (int k = 0; k < brick_rows * brick_cols; ++k) { if (bricks[k].alive) { any = true; break; } }
            if (!any) {
                // all bricks cleared: mark level cleared and pause the game
                mark_level_cleared();
                return; // avoid overwriting ball position later in this tick
            }
            // otherwise just stop checking after this collision
            break;
        }
    }

    ball_x = next_x;
    ball_y = next_y;

    if (ball_y + 2 >= HEIGHT) {
           // ball fell off bottom -> lose a life
           lives -= 1;
           if (lives <= 0) {
               // game over: stop updating ball/paddle until reset
               game_over = true;
               return;
           } else {
               // reset ball/paddle but keep current bricks and level
               reset();
               return;
           }
    }
}

void BrickBreaker::render() {
    m.clear();
    for (int i = 0; i < brick_rows * brick_cols; ++i) {
        if (!bricks[i].alive) continue;
        for (int yy = 0; yy < brick_h; ++yy) for (int xx = 0; xx < brick_w; ++xx) {
            m.set_pixel(bricks[i].x + xx, bricks[i].y + yy, bricks[i].r, bricks[i].g, bricks[i].b);
        }
    }

    for (int yy = 0; yy < paddle_h; ++yy) for (int xx = 0; xx < paddle_w; ++xx) {
        m.set_pixel(paddle_x + xx, paddle_y + yy, 0, 0, 255);
    }

    int bx = (int)ball_x;
    int by = (int)ball_y;
    for (int yy = 0; yy < 2; ++yy) for (int xx = 0; xx < 2; ++xx) {
        int px = bx + xx;
        int py = by + yy;
        if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) m.set_pixel(px, py, 255, 255, 255);
    }
}
