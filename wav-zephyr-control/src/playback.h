#pragma once

#include <stdint.h>

int playback_init(void);
int playback_play(const char *filename);
int playback_stop(void);
int playback_pause(void);
int playback_resume(void);
int playback_set_volume(uint8_t volume);
