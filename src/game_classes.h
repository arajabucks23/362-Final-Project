// game_classes.h
// Central class declarations for Hub75Matrix and BrickBreaker

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// LCD score functions (implemented in score.cpp)
void lcd_init_display();
void lcd_print_score(int score, int level);

// Hub75Matrix: simple GPIO bit-banged driver for 32x32 HUB75
class Hub75Matrix {
public:
    // Pin definitions (user-provided mapping)
    static constexpr uint PIN_R1 = 21;
    static constexpr uint PIN_G1 = 20;
    static constexpr uint PIN_B1 = 19;
    static constexpr uint PIN_R2 = 18;
    static constexpr uint PIN_G2 = 17;
    static constexpr uint PIN_B2 = 16;
    static constexpr uint PIN_A  = 26;
    static constexpr uint PIN_B  = 27;
    static constexpr uint PIN_C  = 28;
    static constexpr uint PIN_D  = 29;
    static constexpr uint PIN_CLK= 15;
    static constexpr uint PIN_OE = 13;
    static constexpr uint PIN_LAT= 14;

    // masks
    static constexpr uint32_t M_R1 = 1u << PIN_R1;
    static constexpr uint32_t M_G1 = 1u << PIN_G1;
    static constexpr uint32_t M_B1 = 1u << PIN_B1;
    static constexpr uint32_t M_R2 = 1u << PIN_R2;
    static constexpr uint32_t M_G2 = 1u << PIN_G2;
    static constexpr uint32_t M_B2 = 1u << PIN_B2;
    static constexpr uint32_t M_CLK= 1u << PIN_CLK;
    static constexpr uint32_t M_LAT= 1u << PIN_LAT;
    static constexpr uint32_t M_OE = 1u << PIN_OE;
    static constexpr uint32_t DATA_MASK = M_R1|M_G1|M_B1|M_R2|M_G2|M_B2;

    // Refresh tuning
    static constexpr int BITPLANES = 5; // fewer planes -> faster refresh
    static constexpr int DWELL_SCALE = 4; // smaller dwell -> faster refresh

    // framebuffer
    uint8_t fb[32][32][3];

    Hub75Matrix();
    void set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    void clear();
    void refresh_once();

private:
    void set_row_address(int row);
};

// Brick data
struct Brick {
    int x, y;
    int hits;
    bool alive;
    uint8_t r, g, b;
    void hit();
};

// BrickBreaker class
class BrickBreaker {
public:
    enum Difficulty { EASY = 0, MEDIUM = 1, HARD = 2 };
    static constexpr int WIDTH = 32;
    static constexpr int HEIGHT = 32;

    Hub75Matrix &m;

    // Paddle
    int paddle_w;
    int paddle_h;
    int paddle_x;
    int paddle_y;

    // Ball
    float ball_x, ball_y;
    float ball_vx, ball_vy;

    // Bricks
    static constexpr int brick_w = 4;
    static constexpr int brick_h = 2;
    static constexpr int brick_spacing = 1;
    static constexpr int brick_cols = 6;
    static constexpr int brick_rows = 4;
    Brick bricks[brick_rows * brick_cols];

    // Score / level
    int score;
    int level;
    Difficulty difficulty;
    float speed_scale; // multiplier applied to base ball speed

    BrickBreaker(Hub75Matrix &matrix);
    void set_difficulty(Difficulty d);
    void init_bricks_for_level();
    void reset();
    void move_paddle_left();
    void move_paddle_right();
    void update_physics();
    void render();
};
