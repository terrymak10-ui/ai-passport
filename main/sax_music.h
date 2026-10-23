#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define SAX_NOTE_COUNT 33
#define SAX_WRITTEN_MIN 58
#define SAX_WRITTEN_MAX 90
#define SAX_SAMPLE_RATE 16000
#define SAX_SINE_PEAK 16000
#define SAX_TONE_PEAK 20000
#define SAX_VOLUME_DEFAULT 90
/* Volume is a codec control value, not a linear percentage of sound pressure. */
uint8_t sax_volume_step(uint8_t current, bool louder);
/* Six main keys: LH index/middle/ring then RH index/middle/ring. */
typedef enum {
    SAX_OCT = 1u << 0, SAX_LC = 1u << 1, SAX_LEB = 1u << 2,
    SAX_LCS = 1u << 3, SAX_LB = 1u << 4, SAX_LBB = 1u << 5,
    SAX_GS = 1u << 6, SAX_SBB = 1u << 7, SAX_PD = 1u << 8,
    SAX_PEB = 1u << 9, SAX_PF = 1u << 10, SAX_SE = 1u << 11,
    SAX_HFS = 1u << 12,
} sax_aux_key_t;
#define SAX_AUX_COUNT 13
#define SAX_AUX_ALL ((1u << SAX_AUX_COUNT) - 1u)
typedef struct { int written; uint8_t keys; uint16_t aux; } sax_note_t;
const sax_note_t *sax_note(size_t index);
double sax_frequency(int sounding_midi);
void sax_note_name(int midi, char *out, size_t size);
/* Empty for naturals; otherwise the enharmonic spelling, including octave. */
void sax_note_alias(int midi, char *out, size_t size);
/* 0 = pure sine, 256 = rich four-harmonic tone; transition is frequency-based. */
unsigned sax_tone_richness(double hz);
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
