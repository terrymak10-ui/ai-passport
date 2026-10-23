#pragma once
#include "sax_music.h"
#include "lvgl.h"
typedef struct {
    size_t selected;
    bool concert, audio_ok;
    uint8_t volume;
    sax_player_state_t player;
} sax_ui_state_t;
/* Caller holds BSP LVGL lock. The module never calls hardware or owns a task. */
void sax_ui_create(const sax_ui_state_t *state);
void sax_ui_refresh(const sax_ui_state_t *state);
void sax_ui_battery(int percent);
void sax_ui_playback_error(void);
