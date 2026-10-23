#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define SAX_NOTE_COUNT 15
#define SAX_SAMPLE_RATE 16000
#define SAX_TONE_PEAK 16000
#define SAX_VOLUME_DEFAULT 80
/* Volume is a codec control value, not a linear percentage of sound pressure. */
uint8_t sax_volume_step(uint8_t current, bool louder);
/* Six main keys: LH index/middle/ring then RH index/middle/ring. */
typedef struct { int written; uint8_t keys; bool octave; bool low_c; } sax_note_t;
const sax_note_t *sax_note(size_t index);
double sax_frequency(int sounding_midi);
void sax_note_name(int midi, char *out, size_t size);
/* Stateful oscillator; phase survives PCM block boundaries. */
void sax_tone(int16_t *out, size_t count, double hz, uint32_t *phase,
              size_t offset, size_t total);

/* UI remains STOPPING until the worker acknowledges that queued PCM is drained. */
typedef enum { SAX_STOPPED, SAX_PLAYING, SAX_STOPPING } sax_player_state_t;
typedef enum { SAX_PLAYER_NONE, SAX_PLAYER_START, SAX_PLAYER_STOP } sax_player_action_t;
sax_player_action_t sax_player_toggle(sax_player_state_t *state);
void sax_player_complete(sax_player_state_t *state);
typedef struct { uint32_t phase; size_t attack_samples; } sax_stream_t;
void sax_stream_render(sax_stream_t *stream, int16_t *out, size_t count, double hz);
/* Call only after at least one full worker block; emits a 10 ms release. */
#define SAX_RELEASE_SAMPLES (SAX_SAMPLE_RATE / 100)
void sax_stream_release(sax_stream_t *stream, int16_t *out, double hz);
