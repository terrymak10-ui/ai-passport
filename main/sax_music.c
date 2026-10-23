#include "sax_music.h"
#include <math.h>
#include <stdio.h>
/* Primary key combinations cross-checked against WFG basic charts and Yamaha.
 * Names denote touches the player presses, not mechanically coupled tone holes.
 * Bb4/Bb5 use side Bb; F#4/F#5 use RH middle; F#6 requires a high-F# key.
 * References and interpretation are recorded in docs/applications/eb-sax.md. */
static const sax_note_t notes[SAX_NOTE_COUNT] = {
    {58, 63, SAX_LC | SAX_LBB},             /* Bb3 */
    {59, 63, SAX_LC | SAX_LB},              /* B3 */
    {60, 63, SAX_LC},                      /* C4 */
    {61, 63, SAX_LC | SAX_LCS},             /* Db4 */
    {62, 63, 0},                           /* D4 */
    {63, 63, SAX_LEB},                     /* Eb4 */
    {64, 31, 0},                           /* E4 */
    {65, 15, 0},                           /* F4 */
    {66, 23, 0},                           /* F#4 */
    {67, 7, 0},                            /* G4 */
    {68, 7, SAX_GS},                       /* Ab4 */
    {69, 3, 0},                            /* A4 */
    {70, 3, SAX_SBB},                      /* Bb4 */
    {71, 1, 0},                            /* B4 */
    {72, 2, 0},                            /* C5 */
    {73, 0, 0},                            /* Db5 */
    {74, 63, SAX_OCT},                     /* D5 */
    {75, 63, SAX_OCT | SAX_LEB},            /* Eb5 */
    {76, 31, SAX_OCT},                     /* E5 */
    {77, 15, SAX_OCT},                     /* F5 */
    {78, 23, SAX_OCT},                     /* F#5 */
    {79, 7, SAX_OCT},                      /* G5 */
    {80, 7, SAX_OCT | SAX_GS},              /* Ab5 */
    {81, 3, SAX_OCT},                      /* A5 */
    {82, 3, SAX_OCT | SAX_SBB},             /* Bb5 */
    {83, 1, SAX_OCT},                      /* B5 */
    {84, 2, SAX_OCT},                      /* C6 */
    {85, 0, SAX_OCT},                      /* Db6 */
    {86, 0, SAX_OCT | SAX_PD},              /* D6 */
    {87, 0, SAX_OCT | SAX_PD | SAX_PEB},     /* Eb6 */
    {88, 0, SAX_OCT | SAX_PD | SAX_PEB | SAX_SE}, /* E6 */
    {89, 0, SAX_OCT | SAX_PD | SAX_PEB | SAX_PF | SAX_SE}, /* F6 */
    {90, 0, SAX_OCT | SAX_PD | SAX_PEB | SAX_PF | SAX_SE | SAX_HFS}, /* F#6 */
};
const sax_note_t *sax_note(size_t i) { return i < SAX_NOTE_COUNT ? &notes[i] : NULL; }
double sax_frequency(int midi) { return 440.0 * exp2((midi - 69) / 12.0); }
void sax_note_name(int midi, char *out, size_t size) {
    static const char *names[] = {"C","Db","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
    if (midi < 0 || midi > 127) { snprintf(out, size, "--"); return; }
    snprintf(out, size, "%s%d", names[midi % 12], midi / 12 - 1);
}
void sax_note_alias(int midi, char *out, size_t size) {
    static const char *aliases[] = {"", "C#", "", "D#", "", "", "Gb", "", "G#", "", "A#", ""};
    if (midi < 0 || midi > 127 || !aliases[midi % 12][0]) {
        if (size) out[0] = 0;
        return;
    }
    snprintf(out, size, "%s%d", aliases[midi % 12], midi / 12 - 1);
}
unsigned sax_tone_richness(double hz) {
    if (hz <= 311.13) return 256;
    if (hz >= 440.0) return 0;
    return (unsigned)((440.0 - hz) * 256.0 / (440.0 - 311.13) + 0.5);
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
/* Harmonic weights 1 : .65 : .45 : .30, sampled over one fundamental period.
 * Normalize the table to 20000; interpolation and crossfade are convex, so
 * no hard clipping or per-sample floating-point trigonometry is needed. */
static const int16_t rich_table[257] = {
    0, 1313, 2620, 3915, 5191, 6443, 7666, 8854, 10001, 11102, 12154, 13151,
    14090, 14967, 15779, 16524, 17199, 17802, 18332, 18789, 19172, 19482, 19718, 19882,
    19975, 20000, 19959, 19854, 19688, 19465, 19189, 18864, 18493, 18081, 17633, 17152,
    16644, 16114, 15566, 15005, 14435, 13860, 13286, 12716, 12155, 11605, 11070, 10554,
    10059, 9588, 9143, 8726, 8339, 7982, 7656, 7362, 7100, 6870, 6672, 6503,
    6364, 6254, 6169, 6109, 6071, 6054, 6054, 6070, 6099, 6139, 6186, 6238,
    6292, 6346, 6398, 6445, 6485, 6516, 6536, 6543, 6535, 6513, 6473, 6417,
    6342, 6249, 6137, 6008, 5860, 5695, 5513, 5316, 5105, 4880, 4644, 4397,
    4143, 3882, 3616, 3348, 3079, 2811, 2547, 2287, 2034, 1790, 1555, 1332,
    1122, 925, 744, 578, 428, 296, 180, 81, -2, -68, -118, -153,
    -174, -182, -179, -165, -143, -114, -79, -40, 0, 40, 79, 114,
    143, 165, 179, 182, 174, 153, 118, 68, 2, -81, -180, -296,
    -428, -578, -744, -925, -1122, -1332, -1555, -1790, -2034, -2287, -2547, -2811,
    -3079, -3348, -3616, -3882, -4143, -4397, -4644, -4880, -5105, -5316, -5513, -5695,
    -5860, -6008, -6137, -6249, -6342, -6417, -6473, -6513, -6535, -6543, -6536, -6516,
    -6485, -6445, -6398, -6346, -6292, -6238, -6186, -6139, -6099, -6070, -6054, -6054,
    -6071, -6109, -6169, -6254, -6364, -6503, -6672, -6870, -7100, -7362, -7656, -7982,
    -8339, -8726, -9143, -9588, -10059, -10554, -11070, -11605, -12155, -12716, -13286, -13860,
    -14435, -15005, -15566, -16114, -16644, -17152, -17633, -18081, -18493, -18864, -19189, -19465,
    -19688, -19854, -19959, -20000, -19975, -19882, -19718, -19482, -19172, -18789, -18332, -17802,
    -17199, -16524, -15779, -14967, -14090, -13151, -12154, -11102, -10001, -8854, -7666, -6443,
    -5191, -3915, -2620, -1313, 0,
};
void sax_tone(int16_t *out, size_t count, double hz, uint32_t *phase,
              size_t offset, size_t total) {
    uint32_t step = (uint32_t)(hz * 4294967296.0 / SAX_SAMPLE_RATE);
    const size_t ramp = SAX_SAMPLE_RATE / 100;
    const unsigned rich = sax_tone_richness(hz);
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
        if (rich) {
            int32_t colored = rich_table[index] +
                (rich_table[index+1] - rich_table[index]) * fraction / 65536;
            value = (value * (int32_t)(256 - rich) + colored * (int32_t)rich) / 256;
        }
        out[i] = (int16_t)(value * (int32_t)envelope / (int32_t)ramp);
        *phase += step;
    }
}

sax_player_action_t sax_player_toggle(sax_player_state_t *state) {
    if (*state == SAX_STOPPED) {
        *state = SAX_PLAYING;
        return SAX_PLAYER_START;
    }
    if (*state == SAX_PLAYING) {
        *state = SAX_STOPPING;
        return SAX_PLAYER_STOP;
    }
    return SAX_PLAYER_NONE;
}
void sax_player_complete(sax_player_state_t *state) { *state = SAX_STOPPED; }
void sax_stream_render(sax_stream_t *stream, int16_t *out, size_t count, double hz) {
    /* Saturate the envelope position after attack; no multi-day sample overflow. */
    sax_tone(out, count, hz, &stream->phase, stream->attack_samples, SIZE_MAX);
    size_t remaining = SAX_RELEASE_SAMPLES - stream->attack_samples;
    stream->attack_samples += count < remaining ? count : remaining;
}
void sax_stream_release(sax_stream_t *stream, int16_t *out, double hz) {
    sax_tone(out, SAX_RELEASE_SAMPLES, hz, &stream->phase,
             SAX_RELEASE_SAMPLES, 2 * SAX_RELEASE_SAMPLES);
}
