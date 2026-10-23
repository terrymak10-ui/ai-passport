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
        assert(n->written >= 60 && n->written <= 84);
        assert((n->keys & ~63) == 0);
        if (i) assert(n->written > sax_note(i-1)->written);
    }
    assert(sax_note(0)->low_c && sax_note(0)->keys == 63);
    assert(sax_note(7)->keys == 2 && !sax_note(7)->octave);
    assert(sax_note(8)->octave && sax_note(8)->keys == 63);
    int16_t a[16000], b[16000]; uint32_t pa = 0, pb = 0;
    sax_tone(a, 16000, 440, &pa, 0, 16000);
    for (size_t i = 0; i < 16000; i += 100) sax_tone(b+i, 100, 440, &pb, i, 16000);
    assert(memcmp(a, b, sizeof(a)) == 0);
    assert(a[0] == 0 && a[15999] == 0);
    int crossings = 0;
    for (int i = 1; i < 16000; ++i) {
        assert(abs(a[i]) <= 5000);
        if (a[i-1] <= 0 && a[i] > 0) ++crossings;
    }
    assert(crossings >= 439 && crossings <= 441);
    puts("Sax music tests: PASS");
}
