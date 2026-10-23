#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define SAX_NOTE_COUNT 15
#define SAX_SAMPLE_RATE 16000
/* Six main keys: LH index/middle/ring then RH index/middle/ring. */
typedef struct { int written; uint8_t keys; bool octave; bool low_c; } sax_note_t;
const sax_note_t *sax_note(size_t index);
double sax_frequency(int sounding_midi);
void sax_note_name(int midi, char *out, size_t size);
/* Stateful oscillator; phase survives PCM block boundaries. */
void sax_tone(int16_t *out, size_t count, double hz, uint32_t *phase,
              size_t offset, size_t total);
