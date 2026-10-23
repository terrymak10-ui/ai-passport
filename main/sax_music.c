#include "sax_music.h"
#include <math.h>
#include <stdio.h>
static const sax_note_t notes[SAX_NOTE_COUNT] = {
    {60,63,false,true}, {62,63,false,false}, {64,31,false,false},
    {65,15,false,false}, {67,7,false,false}, {69,3,false,false},
    {71,1,false,false}, {72,2,false,false}, {74,63,true,false},
    {76,31,true,false}, {77,15,true,false}, {79,7,true,false},
    {81,3,true,false}, {83,1,true,false}, {84,2,true,false},
};
const sax_note_t *sax_note(size_t i) { return i < SAX_NOTE_COUNT ? &notes[i] : NULL; }
double sax_frequency(int midi) { return 440.0 * exp2((midi - 69) / 12.0); }
void sax_note_name(int midi, char *out, size_t size) {
    static const char *names[] = {"C","Db","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
    if (midi < 0 || midi > 127) { snprintf(out, size, "--"); return; }
    snprintf(out, size, "%s%d", names[midi % 12], midi / 12 - 1);
}
uint8_t sax_volume_step(uint8_t current, bool louder) {
    int next = (int)current + (louder ? 10 : -10);
    if (next < 40) next = 40;
    if (next > 100) next = 100;
    return (uint8_t)next;
}
/* Generated as round(16000 * sin(2*pi*i/256)); interpolated without clipping. */
static const int16_t sine_table[257] = {
    0, 393, 785, 1177, 1568, 1959, 2348, 2735, 3121, 3506, 3888, 4267,
    4645, 5019, 5390, 5758, 6123, 6484, 6841, 7194, 7542, 7886, 8226, 8560,
    8889, 9213, 9531, 9844, 10150, 10451, 10745, 11033, 11314, 11588, 11855, 12115,
    12368, 12614, 12851, 13081, 13304, 13518, 13724, 13921, 14111, 14292, 14464, 14627,
    14782, 14928, 15065, 15192, 15311, 15420, 15521, 15611, 15693, 15764, 15827, 15880,
    15923, 15957, 15981, 15995, 16000, 15995, 15981, 15957, 15923, 15880, 15827, 15764,
    15693, 15611, 15521, 15420, 15311, 15192, 15065, 14928, 14782, 14627, 14464, 14292,
    14111, 13921, 13724, 13518, 13304, 13081, 12851, 12614, 12368, 12115, 11855, 11588,
    11314, 11033, 10745, 10451, 10150, 9844, 9531, 9213, 8889, 8560, 8226, 7886,
    7542, 7194, 6841, 6484, 6123, 5758, 5390, 5019, 4645, 4267, 3888, 3506,
    3121, 2735, 2348, 1959, 1568, 1177, 785, 393, 0, -393, -785, -1177,
    -1568, -1959, -2348, -2735, -3121, -3506, -3888, -4267, -4645, -5019, -5390, -5758,
    -6123, -6484, -6841, -7194, -7542, -7886, -8226, -8560, -8889, -9213, -9531, -9844,
    -10150, -10451, -10745, -11033, -11314, -11588, -11855, -12115, -12368, -12614, -12851, -13081,
    -13304, -13518, -13724, -13921, -14111, -14292, -14464, -14627, -14782, -14928, -15065, -15192,
    -15311, -15420, -15521, -15611, -15693, -15764, -15827, -15880, -15923, -15957, -15981, -15995,
    -16000, -15995, -15981, -15957, -15923, -15880, -15827, -15764, -15693, -15611, -15521, -15420,
    -15311, -15192, -15065, -14928, -14782, -14627, -14464, -14292, -14111, -13921, -13724, -13518,
    -13304, -13081, -12851, -12614, -12368, -12115, -11855, -11588, -11314, -11033, -10745, -10451,
    -10150, -9844, -9531, -9213, -8889, -8560, -8226, -7886, -7542, -7194, -6841, -6484,
    -6123, -5758, -5390, -5019, -4645, -4267, -3888, -3506, -3121, -2735, -2348, -1959,
    -1568, -1177, -785, -393, 0,
};
void sax_tone(int16_t *out, size_t count, double hz, uint32_t *phase,
              size_t offset, size_t total) {
    uint32_t step = (uint32_t)(hz * 4294967296.0 / SAX_SAMPLE_RATE);
    const size_t ramp = SAX_SAMPLE_RATE / 100;
    for (size_t i = 0; i < count; ++i) {
        size_t at = offset + i;
        size_t envelope = ramp;
        if (at < ramp) envelope = at;
        if (at >= total) envelope = 0;
        else if (total - 1 - at < envelope) envelope = total - 1 - at;
        unsigned index = *phase >> 24;
        int32_t fraction = (*phase >> 8) & 65535;
        int32_t value = sine_table[index] +
            (sine_table[index+1] - sine_table[index]) * fraction / 65536;
        out[i] = (int16_t)(value * (int32_t)envelope / (int32_t)ramp);
        *phase += step;
    }
}
