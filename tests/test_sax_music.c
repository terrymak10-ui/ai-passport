#include "sax_music.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    char name[16];
    assert(fabs(sax_frequency(69) - 440) < 1e-9);
    assert(fabs(sax_frequency(63) - 311.1269837) < 1e-6);
    sax_note_name(63, name, sizeof(name)); assert(strcmp(name, "Eb4") == 0);
    assert(sax_note(SAX_NOTE_COUNT) == NULL);
    for (size_t i = 0; i < SAX_NOTE_COUNT; ++i) {
        const sax_note_t *n = sax_note(i);
        assert(n->written == SAX_WRITTEN_MIN + (int)i);
        assert((n->aux & ~SAX_AUX_ALL) == 0);
        if (i) assert(fabs(sax_frequency(n->written - 9) /
                           sax_frequency(sax_note(i-1)->written - 9) - exp2(1.0/12)) < 1e-12);
        assert((n->keys & ~63) == 0);
        if (i) assert(n->written > sax_note(i-1)->written);
    }
    assert(sax_note(0)->written == 58 && sax_note(32)->written == 90);
    assert(sax_note(0)->keys == 63 && sax_note(0)->aux == (SAX_LC | SAX_LBB));
    assert(sax_note(1)->aux == (SAX_LC | SAX_LB));
    assert(sax_note(2)->aux == SAX_LC);
    assert(sax_note(3)->aux == (SAX_LC | SAX_LCS));
    assert(sax_note(8)->keys == 23 && !sax_note(8)->aux); /* RH middle F# */
    assert(sax_note(12)->keys == 3 && sax_note(12)->aux == SAX_SBB);
    assert(sax_note(14)->keys == 2 && !sax_note(14)->aux); /* C5 */
    assert(sax_note(15)->keys == 0 && !sax_note(15)->aux); /* Db5 */
    for (size_t i = 4; i <= 15; ++i) {
        assert(sax_note(i+12)->keys == sax_note(i)->keys);
        assert(sax_note(i+12)->aux == (sax_note(i)->aux | SAX_OCT));
    }
    assert(sax_note(28)->aux == (SAX_OCT | SAX_PD));
    assert(sax_note(29)->aux == (SAX_OCT | SAX_PD | SAX_PEB));
    assert(sax_note(30)->aux == (SAX_OCT | SAX_PD | SAX_PEB | SAX_SE));
    assert(sax_note(31)->aux == (SAX_OCT | SAX_PD | SAX_PEB | SAX_PF | SAX_SE));
    assert(sax_note(32)->aux == (sax_note(31)->aux | SAX_HFS));
    assert(fabs(sax_frequency(sax_note(0)->written-9) - 138.5913155) < 1e-6);
    assert(fabs(sax_frequency(sax_note(32)->written-9) - 880) < 1e-9);
    sax_note_alias(61, name, sizeof(name)); assert(strcmp(name, "C#4") == 0);
    sax_note_alias(66, name, sizeof(name)); assert(strcmp(name, "Gb4") == 0);
    sax_note_alias(60, name, sizeof(name)); assert(name[0] == 0);
    assert(sax_tone_richness(138.59) == 256 && sax_tone_richness(311.13) == 256);
    assert(sax_tone_richness(440) == 0 && sax_tone_richness(880) == 0);
    assert(sax_tone_richness(350) > sax_tone_richness(400));
    int16_t a[16000], b[16000]; uint32_t pa = 0, pb = 0;
    sax_tone(a, 16000, 440, &pa, 0, 16000);
    for (size_t i = 0; i < 16000; i += 100) sax_tone(b+i, 100, 440, &pb, i, 16000);
    assert(memcmp(a, b, sizeof(a)) == 0);
    assert(a[0] == 0 && a[15999] == 0);
    assert(sax_volume_step(80, true) == 90);
    assert(sax_volume_step(90, true) == 100);
    assert(sax_volume_step(100, true) == 100);
    assert(sax_volume_step(40, false) == 40);
    assert(sax_volume_step(80, false) == 70);
    for (int v = 50; v <= 90; v += 10)
        assert(sax_volume_step(sax_volume_step(v, true), false) == v);
    int crossings = 0, peak = 0;
    double energy = 0;
    for (int i = 1; i < 16000; ++i) {
        assert(abs(a[i]) <= SAX_TONE_PEAK);
        if (abs(a[i]) > peak) peak = abs(a[i]);
        energy += (double)a[i] * a[i];
        if (a[i-1] <= 0 && a[i] > 0) ++crossings;
    }
    assert(crossings >= 439 && crossings <= 441);
    assert(peak >= 15900 && peak <= 16000);
    double rms = sqrt(energy / 16000);
    assert(rms > 11000 && rms < 11500); /* sine energy including fades */
    sax_player_state_t player = SAX_STOPPED;
    assert(sax_player_toggle(&player) == SAX_PLAYER_START && player == SAX_PLAYING);
    assert(sax_player_toggle(&player) == SAX_PLAYER_STOP && player == SAX_STOPPING);
    assert(sax_player_toggle(&player) == SAX_PLAYER_NONE && player == SAX_STOPPING);
    sax_player_complete(&player);
    assert(player == SAX_STOPPED);
    assert(sax_player_toggle(&player) == SAX_PLAYER_START);
    sax_player_complete(&player); /* playback error also permits retry */
    assert(player == SAX_STOPPED);
    /* Stream crosses the old one-second limit without fading or phase reset. */
    sax_stream_t whole = {0}, chunks = {0};
    int16_t continuous[16384], blocked[16384], release[SAX_RELEASE_SAMPLES];
    sax_stream_render(&whole, continuous, 16384, 311.1269837);
    for (size_t at = 0; at < 16384; at += 256)
        sax_stream_render(&chunks, blocked + at, 256, 311.1269837);
    assert(memcmp(continuous, blocked, sizeof(continuous)) == 0);
    assert(continuous[0] == 0 && whole.attack_samples == SAX_RELEASE_SAMPLES);
    double tail_energy = 0;
    for (int i = 16000; i < 16384; ++i) {
        assert(abs(continuous[i]) <= SAX_TONE_PEAK);
        tail_energy += (double)continuous[i] * continuous[i];
    }
    assert(sqrt(tail_energy / 384) > 8000);
    sax_stream_release(&chunks, release, 311.1269837);
    assert(release[SAX_RELEASE_SAMPLES-1] == 0);
    for (int i = 0; i < SAX_RELEASE_SAMPLES; ++i)
        assert(abs(release[i]) <= SAX_TONE_PEAK);
    /* Every playable pitch: no out-of-range PCM, no repeated attack or phase
     * seam, and a release ending at zero. Test actual rendered samples. */
    for (size_t note = 0; note < SAX_NOTE_COUNT; ++note) {
        double hz = sax_frequency(sax_note(note)->written - 9);
        sax_stream_t stream = {0};
        sax_stream_render(&stream, a, 16000, hz);
        double note_energy = 0;
        for (size_t i = 0; i < 16000; ++i) {
            assert(abs(a[i]) <= SAX_TONE_PEAK);
            note_energy += (double)a[i] * a[i];
        }
        /* Lowest profile fundamental is ~11038; RMS stays above 7500
         * after the short attack, even in the profile crossfade. */
        assert(sqrt(note_energy / 16000) > 7500);
        sax_stream_release(&stream, release, hz);
        assert(release[SAX_RELEASE_SAMPLES-1] == 0);
    }
    /* Spectral regression: keep the fundamental, add controlled harmonics
     * below C5 sounding pitch, retain pure A4 at the high end. */
    double profiles[] = {250, 440};
    for (size_t profile = 0; profile < 2; ++profile) {
        double hz = profiles[profile], amplitudes[4];
        sax_stream_t stream = {0};
        int16_t warm[256];
        sax_stream_render(&stream, warm, 256, hz);
        sax_stream_render(&stream, a, 16000, hz);
        for (int harmonic = 1; harmonic <= 4; ++harmonic) {
            double real = 0, imag = 0;
            for (int i = 0; i < 16000; ++i) {
                double angle = 6.283185307179586 * hz * harmonic * i / SAX_SAMPLE_RATE;
                real += a[i] * cos(angle); imag += a[i] * sin(angle);
            }
            amplitudes[harmonic-1] = 2 * hypot(real, imag) / 16000;
        }
        assert(amplitudes[0] > 9000);
        if (profile == 0) {
            const double ratios[] = {1, .65, .45, .30};
            for (int h = 1; h < 4; ++h)
                assert(fabs(amplitudes[h] / amplitudes[0] - ratios[h]) < .01);
        } else {
            for (int h = 1; h < 4; ++h) assert(amplitudes[h] < amplitudes[0] * .001);
        }
    }
    puts("Sax music tests: PASS");
}
