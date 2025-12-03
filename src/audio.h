// audio.h - simple sound effects API
#pragma once

#include <cstdint>

void sfx_brick_hit();
void sfx_wall_bounce();
void sfx_game_start();
void sfx_game_over();
void sfx_win();
// Call frequently from the main loop to advance non-blocking audio playback
void audio_update();
// Stop any playing sound immediately
void audio_stop();
