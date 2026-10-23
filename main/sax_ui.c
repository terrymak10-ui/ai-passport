#include "sax_ui.h"
#include "sax_diagram.h"
#include <stdio.h>
#include <string.h>
LV_FONT_DECLARE(sax_tc_14);
#define BG 0x101925
#define INK 0xE7EDF5
#define OPEN 0x728598
#define PRESS 0xFFD05F
static lv_obj_t *note_label, *other_label, *frequency_label, *status_label, *mode_label;
static lv_obj_t *alias_label, *index_label, *battery_label, *diagram, *left_hint, *right_hint;
/* Draw callback and refresh both run under the same LVGL lock. */
static sax_note_t displayed_note;

static lv_obj_t *text(lv_obj_t *parent, const char *value, int x, int y, int width) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, value);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, width);
    lv_obj_set_style_text_font(label, &sax_tc_14, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(INK), 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    return label;
}
static void line(lv_layer_t *layer, int ox, int oy, int x1, int y1, int x2, int y2,
                 uint32_t color, int width) {
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.p1.x = ox + x1; d.p1.y = oy + y1;
    d.p2.x = ox + x2; d.p2.y = oy + y2;
    d.color = lv_color_hex(color); d.width = width;
    d.round_start = d.round_end = 1;
    lv_draw_line(layer, &d);
}
static void drawn_text(lv_layer_t *layer, int ox, int oy, int x, int y, int width,
                       const char *value, bool active, const lv_font_t *font,
                       bool centered) {
    lv_draw_label_dsc_t d;
    lv_draw_label_dsc_init(&d);
    d.font = font; d.text = value;
    d.color = lv_color_hex(active ? PRESS : OPEN);
    d.align = centered ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT;
    lv_area_t area = {ox+x, oy+y, ox+x+width-1, oy+y+font->line_height-1};
    lv_draw_label(layer, &d, &area);
}
static void diagram_draw(lv_event_t *event) {
    lv_layer_t *layer = lv_event_get_layer(event);
    lv_area_t area;
    lv_obj_get_coords(lv_event_get_target(event), &area);
    const int x = area.x1, y = area.y1;
    const uint32_t brass = 0x6D6250;
    /* Original stylized neck, tapered body, curved bow and flared bell. */
    static const int tube[][2] = {{87,3},{99,1},{112,7},{118,18},{118,155},{121,168},{131,176},{145,174},{155,162},{160,151}};
    for (size_t i = 1; i < sizeof(tube)/sizeof(tube[0]); ++i)
        line(layer,x,y,tube[i-1][0],tube[i-1][1],tube[i][0],tube[i][1],brass,13);
    for (size_t i = 1; i < sizeof(tube)/sizeof(tube[0]); ++i)
        line(layer,x,y,tube[i-1][0],tube[i-1][1],tube[i][0],tube[i][1],0x282B2C,8);
    line(layer,x,y,151,151,169,151,brass,3);
    line(layer,x,y,151,151,155,162,brass,2);
    line(layer,x,y,169,151,165,163,brass,2);
    /* Leaders terminate at the key region; finger lists below remove ambiguity. */
    for (int i = 0; i < 3; ++i) {
        const sax_diagram_key_t *key = sax_diagram_key(i);
        line(layer,x,y,67,key->y,98,key->y,OPEN,1);
    }
    for (int i = 3; i < 6; ++i) {
        const sax_diagram_key_t *key = sax_diagram_key(i);
        line(layer,x,y,124,key->y,161,key->y-5,OPEN,1);
    }
    line(layer,x,y,53,9,65,9,OPEN,1);
    line(layer,x,y,58,109,68,115,OPEN,1);
    line(layer,x,y,153,166,161,173,OPEN,1);
    sax_hands_t hands = sax_diagram_hands(&displayed_note);
    drawn_text(layer,x,y,8,0,56,"左拇指",hands.left&SAX_F_THUMB,&sax_tc_14,false);
    drawn_text(layer,x,y,8,21,56,"左掌側",hands.left&SAX_F_PALM,&sax_tc_14,false);
    const char *lh[] = {"左食指", "左中指", "左無名指"};
    const char *rh[] = {"右食指", "右中指", "右無名指"};
    for (int i = 0; i < 3; ++i) {
        drawn_text(layer,x,y,8,39+i*21,58,lh[i],displayed_note.keys&(1u<<i),&sax_tc_14,false);
        drawn_text(layer,x,y,165,109+i*20,64,rh[i],displayed_note.keys&(1u<<(i+3)),&sax_tc_14,false);
    }
    drawn_text(layer,x,y,8,102,56,"左小指",hands.left&SAX_F_PINKY,&sax_tc_14,false);
    drawn_text(layer,x,y,165,168,64,"右小指",hands.right&SAX_F_PINKY,&sax_tc_14,false);
    drawn_text(layer,x,y,165,36,64,"右食指側",hands.right&SAX_F_INDEX_SIDE,&sax_tc_14,false);
    drawn_text(layer,x,y,165,77,64,"右中指側",hands.right&SAX_F_MIDDLE_SIDE,&sax_tc_14,false);
    if (displayed_note.aux & SAX_HFS)
        drawn_text(layer,x,y,8,155,88,"需高F#鍵",true,&sax_tc_14,false);
    for (size_t i = 0; i < SAX_DIAGRAM_KEY_COUNT; ++i) {
        const sax_diagram_key_t *key = sax_diagram_key(i);
        bool active = sax_diagram_pressed(&displayed_note,key);
        lv_draw_rect_dsc_t d;
        lv_draw_rect_dsc_init(&d);
        d.radius = key->main_mask ? LV_RADIUS_CIRCLE : 4;
        d.bg_color = lv_color_hex(active ? PRESS : BG);
        d.bg_opa = LV_OPA_COVER;
        d.border_color = lv_color_hex(active ? PRESS : OPEN);
        d.border_width = key->main_mask ? 2 : 1;
        lv_area_t box = {x+key->x-key->w/2,y+key->y-key->h/2,
                         x+key->x-key->w/2+key->w-1,y+key->y-key->h/2+key->h-1};
        lv_draw_rect(layer,&d,&box);
        lv_draw_label_dsc_t glyph;
        lv_draw_label_dsc_init(&glyph);
        glyph.font = key->main_mask ? &lv_font_montserrat_14 : &lv_font_montserrat_10;
        glyph.text = key->glyph; glyph.align = LV_TEXT_ALIGN_CENTER;
        glyph.color = lv_color_hex(active ? BG : INK);
        lv_area_t glyph_box = {box.x1, y+key->y-glyph.font->line_height/2,
                              box.x2,y+key->y+(glyph.font->line_height+1)/2-1};
        lv_draw_label(layer,&glyph,&glyph_box);
    }
}
void sax_ui_refresh(const sax_ui_state_t *state) {
    const sax_note_t *note = sax_note(state->selected);
    if (!note) return;
    displayed_note = *note;
    char written[12], concert[12], alias[12], value[96];
    sax_note_name(note->written,written,sizeof(written));
    sax_note_name(note->written-9,concert,sizeof(concert));
    sax_note_alias(state->concert ? note->written-9 : note->written,alias,sizeof(alias));
    lv_label_set_text(note_label,state->concert ? concert : written);
    lv_label_set_text(alias_label,alias);
    lv_label_set_text(mode_label,state->concert ? "實際音高" : "指法音高");
    snprintf(value,sizeof(value),"%u / %u",(unsigned)state->selected+1,SAX_NOTE_COUNT);
    lv_label_set_text(index_label,value);
    snprintf(value,sizeof(value),"%s %s",state->concert ? "指法" : "實音",state->concert ? written : concert);
    lv_label_set_text(other_label,value);
    snprintf(value,sizeof(value),"%.2f Hz",sax_frequency(note->written-9));
    lv_label_set_text(frequency_label,value);
    sax_hands_t hands = sax_diagram_hands(note);
    (void)sax_hand_text(hands.left,true,value,sizeof(value));
    lv_label_set_text(left_hint,value);
    (void)sax_hand_text(hands.right,false,value,sizeof(value));
    lv_label_set_text(right_hint,value);
    if (!state->audio_ok) lv_label_set_text(status_label,"音訊未就緒");
    else {
        const char *action = state->player == SAX_STOPPING ? "停止中" :
                             state->player == SAX_PLAYING ? "OK 停止" : "OK 播放";
        snprintf(value,sizeof(value),"%s | 音量 %u",action,(unsigned)state->volume);
        lv_label_set_text(status_label,value);
    }
    lv_obj_invalidate(diagram);
}
void sax_ui_create(const sax_ui_state_t *state) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen,lv_color_hex(BG),0);
    lv_obj_set_style_bg_opa(screen,LV_OPA_COVER,0);
    lv_obj_set_style_text_font(screen,&sax_tc_14,0);
    lv_obj_set_style_pad_all(screen,0,0);
    lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    note_label = text(screen,"",18,10,110);
    lv_obj_set_style_text_font(note_label,&lv_font_montserrat_40,0);
    mode_label = text(screen,"",132,12,60);
    index_label = text(screen,"",132,32,60);
    battery_label = text(screen,"--%",196,32,32);
    other_label = text(screen,"",132,49,96);
    alias_label = text(screen,"",18,64,73);
    frequency_label = text(screen,"",95,66,133);
    diagram = lv_obj_create(screen);
    lv_obj_remove_style_all(diagram);
    lv_obj_set_pos(diagram,0,82);
    lv_obj_set_size(diagram,SAX_DIAGRAM_WIDTH,SAX_DIAGRAM_HEIGHT);
    lv_obj_remove_flag(diagram,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(diagram,diagram_draw,LV_EVENT_DRAW_MAIN,NULL);
    left_hint = text(screen,"",16,268,211);
    right_hint = text(screen,"",16,285,211);
    status_label = text(screen,"",24,302,203);
    lv_screen_load(screen);
    sax_ui_refresh(state);
}
void sax_ui_battery(int percent) {
    char value[12];
    if (percent < 0) snprintf(value,sizeof(value),"--%%");
    else snprintf(value,sizeof(value),"%d%%",percent);
    lv_label_set_text(battery_label,value);
}
void sax_ui_playback_error(void) {
    lv_label_set_text(status_label,"播放失敗，OK 重試");
}
