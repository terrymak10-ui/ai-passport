/* Optional real-LVGL host rendering test, after Managed Components are present.
 * It uses the firmware's 24 KiB allocator, same UI, generated font and RGB565
 * partial buffer. No SDL, ESP-IDF or hardware stubs are needed. */
#include "sax_ui.h"
#include "sax_diagram.h"
#include "bsp_display_rounding.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
LV_FONT_DECLARE(sax_tc_14);
static uint32_t draw_buffer[240*40/2];
static uint16_t frame[320][240];
static void flush(lv_display_t *display,const lv_area_t *area,uint8_t *pixels) {
    unsigned stride=lv_draw_buf_width_to_stride(lv_area_get_width(area),LV_COLOR_FORMAT_RGB565);
    for(int y=area->y1;y<=area->y2;++y) {
        const uint16_t *row=(const uint16_t *)(pixels+(y-area->y1)*stride);
        for(int x=area->x1;x<=area->x2;++x) {
            assert(x>=0 && x<240 && y>=0 && y<320);
            frame[y][x]=bsp_display_pixel_outside_rounded_rect(x,y,240,320,30)?0:row[x-area->x1];
        }
    }
    lv_display_flush_ready(display);
}
static void save_ppm(const char *prefix,int midi) {
    char path[512]; snprintf(path,sizeof(path),"%s-%d.ppm",prefix,midi);
    FILE *out=fopen(path,"wb"); assert(out);
    fprintf(out,"P6\n240 320\n255\n");
    for(int y=0;y<320;++y) for(int x=0;x<240;++x) {
        uint16_t p=frame[y][x];
        unsigned char rgb[]={((p>>11)&31)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};
        assert(fwrite(rgb,1,3,out)==3);
    }
    fclose(out);
}
static void font_coverage(const char *path) {
    FILE *in=fopen(path,"rb"); assert(in);
    int byte;
    unsigned checked=0;
    while((byte=fgetc(in))!=EOF) {
        uint32_t cp=(unsigned char)byte;
        unsigned follow=0;
        if(cp>=0xF0){cp&=7;follow=3;}
        else if(cp>=0xE0){cp&=15;follow=2;}
        else if(cp>=0xC0){cp&=31;follow=1;}
        for(unsigned i=0;i<follow;++i){int b=fgetc(in);assert(b!=EOF && (b&0xC0)==0x80);cp=(cp<<6)|(b&63);}
        if(cp<32) continue;
        lv_font_glyph_dsc_t glyph;
        assert(lv_font_get_glyph_dsc(&sax_tc_14,&glyph,cp,0));
        assert(!glyph.is_placeholder);
        ++checked;
    }
    fclose(in);
    lv_font_glyph_dsc_t missing = {0};
    assert(!lv_font_get_glyph_dsc(&sax_tc_14,&missing,0x9F98,0) || missing.is_placeholder);
    printf("Font coverage: PASS (%u code points, line height %u)\n",checked,sax_tc_14.line_height);
}
int main(int argc,char **argv) {
    assert(argc==3);
    lv_init(); font_coverage(argv[1]);
    lv_display_t *display=lv_display_create(240,320); assert(display);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,draw_buffer,NULL,sizeof(draw_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    sax_ui_state_t state={72-SAX_WRITTEN_MIN,false,true,90,SAX_STOPPED};
    sax_ui_create(&state); sax_ui_battery(85);
    lv_obj_update_layout(lv_screen_active());
    unsigned diagrams=0, body_labels=0;
    for(uint32_t i=0;i<lv_obj_get_child_count(lv_screen_active());++i) {
        lv_obj_t *child=lv_obj_get_child(lv_screen_active(),i);
        if(lv_obj_check_type(child,&lv_label_class)) {
            const lv_font_t *font=lv_obj_get_style_text_font(child,LV_PART_MAIN);
            assert(font==&sax_tc_14 || font==&lv_font_montserrat_40);
            if(font==&sax_tc_14) ++body_labels;
        } else {
            assert(lv_obj_get_x(child)==0 && lv_obj_get_y(child)==82);
            assert(lv_obj_get_width(child)==240 && lv_obj_get_height(child)==185);
            ++diagrams;
        }
    }
    assert(diagrams==1 && body_labels==9);
    for(size_t i=0;i<SAX_NOTE_COUNT;++i) {
        state.selected=i;
        for(int mode=0;mode<2;++mode) {
            state.concert=mode!=0; sax_ui_refresh(&state);
            sax_hands_t hands=sax_diagram_hands(sax_note(i));
            char text[96]; lv_point_t size;
            assert(sax_hand_text(hands.left,true,text,sizeof(text)));
            lv_text_get_size(&size,text,&sax_tc_14,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE); assert(size.x<=211);
            assert(sax_hand_text(hands.right,false,text,sizeof(text)));
            lv_text_get_size(&size,text,&sax_tc_14,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE); assert(size.x<=211);
            lv_refr_now(display);
            if(!mode && (sax_note(i)->written==60 || sax_note(i)->written==72 || sax_note(i)->written==90))
                save_ppm(argv[2],sax_note(i)->written);
        }
    }
    for(int pass=0;pass<100;++pass) {
        state.selected=pass%SAX_NOTE_COUNT; state.player=pass%3;
        sax_ui_refresh(&state); lv_refr_now(display);
    }
    sax_ui_battery(-1); sax_ui_playback_error(); lv_refr_now(display);
    lv_mem_monitor_t monitor; lv_mem_monitor(&monitor);
    printf("UI rendering: PASS (33 notes x 2 modes, 100 state updates, 24 KiB pool; free=%u biggest=%u)\n",
           (unsigned)monitor.free_size,(unsigned)monitor.free_biggest_size);
    assert(monitor.free_size>2048);
}
