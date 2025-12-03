#include "audio.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#define AUDIO_PIN 31
static uint slice_num;
static uint chan_num;
static bool audio_active = false;

// Simple non-blocking tone queue
struct Tone { uint32_t freq; uint32_t ms; };
static Tone seq_buf[64];
static int seq_len = 0;
static int seq_pos = 0;
static uint32_t seq_remaining_ms = 0;
static absolute_time_t last_tick;

static void audio_init() {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(AUDIO_PIN);
    chan_num  = pwm_gpio_to_channel(AUDIO_PIN);
    pwm_set_enabled(slice_num, true);
    audio_active = true;
    last_tick = get_absolute_time();
}

static void audio_set_freq_nonblocking(uint32_t freq) {
    if (!audio_active) audio_init();
    uint32_t clock_hz = clock_get_hz(clk_sys);
    if (freq == 0) {
        // silence: set level 0
        pwm_set_chan_level(slice_num, chan_num, 0);
        return;
    }
    // clamp freq to reasonable range
    if (freq > (clock_hz / 2)) freq = clock_hz / 2;
    uint32_t wrap = clock_hz / freq - 1;
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan_num, wrap / 2);
}

void audio_stop() {
    if (audio_active) {
        pwm_set_enabled(slice_num, false);
        gpio_set_function(AUDIO_PIN, GPIO_FUNC_SIO);
        gpio_put(AUDIO_PIN, 0);
        audio_active = false;
    }
    seq_len = 0; seq_pos = 0; seq_remaining_ms = 0;
}

// Queue a tone sequence (copy into internal buffer)
static void audio_queue_sequence(const Tone *seq, int n) {
    if (n <= 0) return;
    if (n > (int)(sizeof(seq_buf)/sizeof(seq_buf[0]))) n = sizeof(seq_buf)/sizeof(seq_buf[0]);
    for (int i = 0; i < n; ++i) seq_buf[i] = seq[i];
    seq_len = n;
    seq_pos = 0;
    seq_remaining_ms = seq_buf[0].ms;
    if (seq_buf[0].freq == 0) {
        // silence
        audio_set_freq_nonblocking(0);
    } else {
        audio_set_freq_nonblocking(seq_buf[0].freq);
    }
    last_tick = get_absolute_time();
}

void audio_update() {
    if (seq_len == 0) return;
    absolute_time_t now = get_absolute_time();
    int32_t delta_ms = (int32_t) to_ms_since_boot(now) - (int32_t) to_ms_since_boot(last_tick);
    if (delta_ms <= 0) return;
    last_tick = now;
    if (seq_remaining_ms > (uint32_t)delta_ms) {
        seq_remaining_ms -= delta_ms;
        return;
    }
    // move to next tone(s)
    int32_t rem = (int32_t)delta_ms - (int32_t)seq_remaining_ms;
    seq_pos++;
    if (seq_pos >= seq_len) {
        // finished
        audio_stop();
        return;
    }
    // start next tone
    if (seq_buf[seq_pos].freq == 0) audio_set_freq_nonblocking(0); else audio_set_freq_nonblocking(seq_buf[seq_pos].freq);
    // set remaining time for this tone minus leftover delta
    seq_remaining_ms = seq_buf[seq_pos].ms;
    if (rem > 0) {
        if ((uint32_t)rem >= seq_remaining_ms) {
            // skip ahead (will be handled on next update)
            seq_remaining_ms = 0;
        } else {
            seq_remaining_ms -= rem;
        }
    }
}

void sfx_brick_hit() {
    Tone t[] = {{70,70},{20,60}};
    audio_queue_sequence(t, sizeof(t)/sizeof(t[0]));
}

void sfx_wall_bounce() {
    Tone t[] = {{150,50},{80,50}};
    audio_queue_sequence(t, sizeof(t)/sizeof(t[0]));
}

void sfx_game_start() {
    Tone t[] = {{220,200},{262,200},{330,200},{440,200},{392,200},{330,200},{262,300}};
    audio_queue_sequence(t, sizeof(t)/sizeof(t[0]));
}

void sfx_game_over() {
    // descending sweep from 200 down to 40 in steps
    Tone t[48];
    int n = 0;
    for (int f = 200; f >= 40 && n < (int)(sizeof(t)/sizeof(t[0])); f -= 4) {
        t[n++] = { (uint32_t)f, 20 };
    }
    audio_queue_sequence(t, n);
}

void sfx_win() {
    // Combine game_start melody and a short descending sweep
    Tone start[] = {{220,200},{262,200},{330,200},{440,200},{392,200},{330,200},{262,300}};
    // descending sweep
    Tone sweep[32];
    int sn = 0;
    for (int f = 200; f >= 60 && sn < (int)(sizeof(sweep)/sizeof(sweep[0])); f -= 8) {
        sweep[sn++] = { (uint32_t)f, 30 };
    }
    // build combined buffer
    int total = (int)(sizeof(start)/sizeof(start[0])) + sn;
    if (total > (int)(sizeof(seq_buf)/sizeof(seq_buf[0]))) total = sizeof(seq_buf)/sizeof(seq_buf[0]);
    int idx = 0;
    for (int i = 0; i < (int)(sizeof(start)/sizeof(start[0])) && idx < total; ++i) seq_buf[idx++] = start[i];
    for (int i = 0; i < sn && idx < total; ++i) seq_buf[idx++] = sweep[i];
    seq_len = idx;
    seq_pos = 0;
    if (seq_len > 0) {
        seq_remaining_ms = seq_buf[0].ms;
        if (seq_buf[0].freq == 0) audio_set_freq_nonblocking(0); else audio_set_freq_nonblocking(seq_buf[0].freq);
        last_tick = get_absolute_time();
    }
}

