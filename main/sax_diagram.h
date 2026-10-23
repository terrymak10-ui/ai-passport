#pragma once
#include "sax_music.h"
/* Coordinates are an original front-view schematic, not a mechanical drawing.
 * Thumb is deliberately offset as a back-key callout. No Yamaha art is copied. */
#define SAX_DIAGRAM_WIDTH 240
#define SAX_DIAGRAM_HEIGHT 185
#define SAX_DIAGRAM_KEY_COUNT (6 + SAX_AUX_COUNT)
typedef struct {
    int x, y, w, h;
    uint8_t main_mask;
    uint16_t aux_mask;
    const char *glyph;
} sax_diagram_key_t;
const sax_diagram_key_t *sax_diagram_key(size_t index);
bool sax_diagram_pressed(const sax_note_t *note, const sax_diagram_key_t *key);
typedef enum {
    SAX_F_THUMB = 1u << 0, SAX_F_INDEX = 1u << 1, SAX_F_MIDDLE = 1u << 2,
    SAX_F_RING = 1u << 3, SAX_F_PINKY = 1u << 4, SAX_F_PALM = 1u << 5,
    SAX_F_INDEX_SIDE = 1u << 6, SAX_F_MIDDLE_SIDE = 1u << 7,
} sax_finger_t;
typedef struct { uint8_t left, right; } sax_hands_t;
sax_hands_t sax_diagram_hands(const sax_note_t *note);
/* All-or-empty copy: never truncate a UTF-8 character. */
bool sax_hand_text(uint8_t fingers, bool left, char *text, size_t capacity);
