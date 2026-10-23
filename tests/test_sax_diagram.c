#include "sax_diagram.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void) {
    unsigned main_union=0, aux_union=0;
    for (size_t i=0; i<SAX_DIAGRAM_KEY_COUNT; ++i) {
        const sax_diagram_key_t *key=sax_diagram_key(i);
        assert(key && key->glyph && key->w>0 && key->h>0);
        assert(key->x-key->w/2>=0 && key->x+(key->w+1)/2<=SAX_DIAGRAM_WIDTH);
        assert(key->y-key->h/2>=0 && key->y+(key->h+1)/2<=SAX_DIAGRAM_HEIGHT);
        assert((key->main_mask!=0) != (key->aux_mask!=0));
        assert(!(main_union & key->main_mask) && !(aux_union & key->aux_mask));
        main_union |= key->main_mask; aux_union |= key->aux_mask;
    }
    assert(main_union==63 && aux_union==SAX_AUX_ALL);
    assert(!sax_diagram_key(SAX_DIAGRAM_KEY_COUNT));
    for (size_t i=0;i<SAX_NOTE_COUNT;++i) {
        const sax_note_t *note=sax_note(i);
        unsigned active_main=0, active_aux=0;
        for(size_t k=0;k<SAX_DIAGRAM_KEY_COUNT;++k) {
            const sax_diagram_key_t *key=sax_diagram_key(k);
            if (sax_diagram_pressed(note,key)) {
                active_main|=key->main_mask; active_aux|=key->aux_mask;
            }
        }
        assert(active_main==note->keys && active_aux==note->aux);
        char text[96], short_text[4]={'x'};
        sax_hands_t hands=sax_diagram_hands(note);
        assert(sax_hand_text(hands.left,true,text,sizeof(text)));
        assert(strncmp(text,"左：",strlen("左："))==0);
        assert(!sax_hand_text(hands.left,true,short_text,sizeof(short_text)));
        assert(short_text[0]==0); /* never split a Chinese character */
        assert(sax_hand_text(hands.right,false,text,sizeof(text)));
    }
    sax_hands_t c5=sax_diagram_hands(sax_note(72-SAX_WRITTEN_MIN));
    assert(c5.left==SAX_F_MIDDLE && !c5.right);
    char text[96];
    assert(sax_hand_text(c5.left,true,text,sizeof(text)) && !strcmp(text,"左：中指"));
    assert(sax_hand_text(c5.right,false,text,sizeof(text)) && !strcmp(text,"右：全部放開"));
    sax_hands_t bb=sax_diagram_hands(sax_note(70-SAX_WRITTEN_MIN));
    assert(bb.left==(SAX_F_INDEX|SAX_F_MIDDLE));
    assert(bb.right==SAX_F_INDEX_SIDE); /* side touch does not mean RH main index */
    sax_hands_t c4=sax_diagram_hands(sax_note(60-SAX_WRITTEN_MIN));
    assert(c4.left==(SAX_F_INDEX|SAX_F_MIDDLE|SAX_F_RING));
    assert(c4.right==(SAX_F_INDEX|SAX_F_MIDDLE|SAX_F_RING|SAX_F_PINKY));
    sax_hands_t fs6=sax_diagram_hands(sax_note(90-SAX_WRITTEN_MIN));
    assert(fs6.left==(SAX_F_THUMB|SAX_F_PALM));
    assert(fs6.right==(SAX_F_INDEX_SIDE|SAX_F_MIDDLE_SIDE));
    puts("Sax diagram tests: PASS");
}
