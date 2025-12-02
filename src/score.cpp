// score.cpp -- LCD bitbang and score display

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <cstdio>
#include <cstring>

// Use GPIO pins provided by the user
static const int LCD_SCK = 34;
static const int LCD_TX  = 35;
static const int LCD_CSn = 33;

// Bitbang SPI write (MSB-first)
static void lcd_write_raw(const char *buf, size_t len) {
    gpio_put(LCD_CSn, 0);
    for (size_t i = 0; i < len; ++i) {
        uint8_t b = (uint8_t)buf[i];
        // send LSB-first (bit0 .. bit7) which matches UART/serial backpack ordering
        for (int bit = 0; bit < 8; ++bit) {
            gpio_put(LCD_TX, (b >> bit) & 1);
            busy_wait_us_32(3);
            gpio_put(LCD_SCK, 1);
            busy_wait_us_32(3);
            gpio_put(LCD_SCK, 0);
        }
    }
    gpio_put(LCD_CSn, 1);
}

// Many serial LCD backpacks accept 0xFE as a command prefix over serial/SPI
static void lcd_send_cmd(uint8_t cmd) {
    char seq[2] = { (char)0xFE, (char)cmd };
    lcd_write_raw(seq, 2);
    busy_wait_us_32(2000);
}

void lcd_init_display() {
    gpio_init(LCD_CSn);
    gpio_set_dir(LCD_CSn, GPIO_OUT);
    gpio_put(LCD_CSn, 1);

    gpio_init(LCD_SCK);
    gpio_set_dir(LCD_SCK, GPIO_OUT);
    gpio_put(LCD_SCK, 0);

    gpio_init(LCD_TX);
    gpio_set_dir(LCD_TX, GPIO_OUT);
    gpio_put(LCD_TX, 0);
}

void lcd_print_score(int score, int level) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Lvl%02d Score:%5d", level, score);
    // Try clearing display and setting cursor (common serial backpack protocol)
    lcd_send_cmd(0x01); // clear display
    busy_wait_us_32(2000);

    // Set DDRAM addr to start of first line (0x80) and write up to 16 chars
    lcd_send_cmd(0x80);
    // Ensure exactly 16 chars for first line
    char line1[17];
    size_t slen = strlen(buf);
    if (slen >= 16) {
        memcpy(line1, buf, 16);
    } else {
        memcpy(line1, buf, slen);
        memset(line1 + slen, ' ', 16 - slen);
    }
    line1[16] = '\0';
    lcd_write_raw(line1, 16);

    // (Optional) If you want the second line to show something, set 0xC0 and write there.
}
