#include "sax_diagram.h"
#include <string.h>
static const sax_diagram_key_t keys[SAX_DIAGRAM_KEY_COUNT] = {
    {111,46,20,20,1,0,"1"}, {111,67,20,20,2,0,"2"},
    {111,88,20,20,4,0,"3"}, {111,120,20,20,8,0,"1"},
    {111,141,20,20,16,0,"2"}, {111,162,20,20,32,0,"3"},
    {76,9,16,14,0,SAX_OCT,"T"},
    {80,100,20,12,0,SAX_GS,"G#"},
    {71,116,18,12,0,SAX_LCS,"C#"},
    {88,116,14,12,0,SAX_LB,"B"},
    {80,131,20,12,0,SAX_LBB,"Bb"},
    {141,164,18,12,0,SAX_LC,"C"},
    {141,145,20,12,0,SAX_LEB,"Eb"},
    {141,63,20,12,0,SAX_SBB,"Bb"},
    {96,16,14,12,0,SAX_PD,"D"},
    {84,30,18,12,0,SAX_PEB,"Eb"},
    {88,44,14,12,0,SAX_PF,"F"},
    {141,42,16,12,0,SAX_SE,"E"},
    {141,84,20,12,0,SAX_HFS,"F#"},
};
const sax_diagram_key_t *sax_diagram_key(size_t index) {
    return index < SAX_DIAGRAM_KEY_COUNT ? &keys[index] : NULL;
}
bool sax_diagram_pressed(const sax_note_t *note, const sax_diagram_key_t *key) {
    return (note->keys & key->main_mask) || (note->aux & key->aux_mask);
}
sax_hands_t sax_diagram_hands(const sax_note_t *note) {
    sax_hands_t hands = {0};
    const uint8_t fingers[] = {SAX_F_INDEX, SAX_F_MIDDLE, SAX_F_RING};
    for (int i = 0; i < 3; ++i) {
        if (note->keys & (1u << i)) hands.left |= fingers[i];
        if (note->keys & (1u << (i+3))) hands.right |= fingers[i];
    }
    if (note->aux & SAX_OCT) hands.left |= SAX_F_THUMB;
    if (note->aux & (SAX_GS | SAX_LCS | SAX_LB | SAX_LBB)) hands.left |= SAX_F_PINKY;
    if (note->aux & (SAX_PD | SAX_PEB | SAX_PF)) hands.left |= SAX_F_PALM;
    if (note->aux & (SAX_LC | SAX_LEB)) hands.right |= SAX_F_PINKY;
    if (note->aux & (SAX_SE | SAX_SBB)) hands.right |= SAX_F_INDEX_SIDE;
    if (note->aux & SAX_HFS) hands.right |= SAX_F_MIDDLE_SIDE;
    return hands;
}
bool sax_hand_text(uint8_t fingers, bool left, char *text, size_t capacity) {
    static const char *names[] = {"拇指", "食指", "中指", "無名指", "小指", "掌側", "食指側", "中指側"};
    char buffer[128];
    strcpy(buffer, left ? "左：" : "右：");
    bool first = true;
    for (int i = 0; i < 8; ++i) {
        if (!(fingers & (1u << i))) continue;
        if (!first) strcat(buffer, " ");
        strcat(buffer, names[i]);
        first = false;
    }
    if (first) strcat(buffer, "全部放開");
    size_t bytes = strlen(buffer) + 1;
    if (bytes > capacity) {
        if (capacity) text[0] = 0;
        return false;
    }
    memcpy(text, buffer, bytes);
    return true;
}
