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
static const int16_t sine_table[257] = {
0,123,245,368,490,612,734,855,975,1096,1215,1334,1451,1568,1684,1799,1913,2026,2138,2248,2357,2464,2571,2675,2778,2879,2978,3076,3172,3266,3358,3448,3536,3621,3705,3786,3865,3942,4016,4088,4157,4224,4289,4350,4410,4466,4520,4571,4619,4665,4708,4748,4785,4819,4850,4879,4904,4926,4946,4962,4976,4986,4994,4998,5000,4998,4994,4986,4976,4962,4946,4926,4904,4879,4850,4819,4785,4748,4708,4665,4619,4571,4520,4466,4410,4350,4289,4224,4157,4088,4016,3942,3865,3786,3705,3621,3536,3448,3358,3266,3172,3076,2978,2879,2778,2675,2571,2464,2357,2248,2138,2026,1913,1799,1684,1568,1451,1334,1215,1096,975,855,734,612,490,368,245,123,0,-123,-245,-368,-490,-612,-734,-855,-975,-1096,-1215,-1334,-1451,-1568,-1684,-1799,-1913,-2026,-2138,-2248,-2357,-2464,-2571,-2675,-2778,-2879,-2978,-3076,-3172,-3266,-3358,-3448,-3536,-3621,-3705,-3786,-3865,-3942,-4016,-4088,-4157,-4224,-4289,-4350,-4410,-4466,-4520,-4571,-4619,-4665,-4708,-4748,-4785,-4819,-4850,-4879,-4904,-4926,-4946,-4962,-4976,-4986,-4994,-4998,-5000,-4998,-4994,-4986,-4976,-4962,-4946,-4926,-4904,-4879,-4850,-4819,-4785,-4748,-4708,-4665,-4619,-4571,-4520,-4466,-4410,-4350,-4289,-4224,-4157,-4088,-4016,-3942,-3865,-3786,-3705,-3621,-3536,-3448,-3358,-3266,-3172,-3076,-2978,-2879,-2778,-2675,-2571,-2464,-2357,-2248,-2138,-2026,-1913,-1799,-1684,-1568,-1451,-1334,-1215,-1096,-975,-855,-734,-612,-490,-368,-245,-123,0
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
