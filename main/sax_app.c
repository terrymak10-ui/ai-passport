/* Offline first increment. BSP is unchanged; all UI is application-owned. */
#include "sax_music.h"
#include "bsp_audio.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_battery.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <stdio.h>

typedef struct { bsp_btn_t key; bsp_btn_ev_t event; } input_t;
typedef struct { int midi; unsigned generation; uint8_t volume; } tone_request_t;
typedef struct { unsigned generation; bool ok; } tone_result_t;
static QueueHandle_t inputs, tones, results;
static lv_obj_t *note_label, *other_label, *frequency_label, *status_label, *mode_label;
static lv_obj_t *keys[6], *octave_key, *low_key, *battery_label;
static bool audio_ok, concert;
static size_t selected;
static unsigned generation;
static sax_player_state_t player = SAX_STOPPED;
static TaskHandle_t audio_task;
static uint8_t volume = SAX_VOLUME_DEFAULT;
static int64_t last_activity;
static bool dimmed;
static const char *TAG = "sax";

static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y, int width) {
    lv_obj_t *o = lv_label_create(parent);
    lv_label_set_text(o, text);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_width(o, width);
    lv_obj_set_style_text_color(o, lv_color_hex(0xE7EDF5), 0);
    return o;
}
static lv_obj_t *key_dot(lv_obj_t *parent, int x, int y, int size) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, size, size);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(o, 2, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(0x62D8C0), 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}
static void fill(lv_obj_t *o, bool closed) {
    lv_obj_set_style_bg_color(o, lv_color_hex(closed ? 0x62D8C0 : 0x172333), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
}
/* Called only under the BSP LVGL lock. Fingering always follows written pitch. */
static void refresh(void) {
    const sax_note_t *n = sax_note(selected);
    char written[12], sounding[12], text[64];
    sax_note_name(n->written, written, sizeof(written));
    sax_note_name(n->written - 9, sounding, sizeof(sounding));
    lv_label_set_text(note_label, concert ? sounding : written);
    lv_label_set_text(mode_label, concert ? "CONCERT / Eb ALTO" : "WRITTEN / Eb ALTO");
    snprintf(text, sizeof(text), "%s %s  |  %u / %u", concert ? "Written" : "Concert",
             concert ? written : sounding, (unsigned)selected + 1, SAX_NOTE_COUNT);
    lv_label_set_text(other_label, text);
    snprintf(text, sizeof(text), "%.2f Hz   A4 = 440", sax_frequency(n->written - 9));
    lv_label_set_text(frequency_label, text);
    for (int i = 0; i < 6; ++i) fill(keys[i], (n->keys >> i) & 1);
    fill(octave_key, n->octave);
    fill(low_key, n->low_c);
    if (!audio_ok) lv_label_set_text(status_label, "Audio unavailable");
    else lv_label_set_text_fmt(status_label, "%s | VOL %u",
                               player == SAX_STOPPING ? "Stopping..." : player == SAX_PLAYING ? "OK: stop" : "OK: play", (unsigned)volume);
}
static void build_ui(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101925), 0);
    lv_obj_set_style_text_font(screen, &lv_font_montserrat_14, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    label(screen, "SAX / PASSPORT", 18, 15, 150);
    battery_label = label(screen, "--%", 177, 15, 45);
    mode_label = label(screen, "", 18, 43, 210);
    note_label = label(screen, "", 18, 66, 204);
    lv_obj_set_style_text_font(note_label, &lv_font_montserrat_40, 0);
    other_label = label(screen, "", 18, 113, 210);
    frequency_label = label(screen, "", 18, 134, 210);
    label(screen, "LH", 27, 160, 35);
    label(screen, "RH", 27, 206, 35);
    for (int i = 0; i < 6; ++i) {
        keys[i] = key_dot(screen, 72 + (i % 3) * 39, i < 3 ? 158 : 204, 28);
        char digit[2] = {(char)('1' + i % 3), 0};
        label(screen, digit, 82 + (i % 3) * 39, i < 3 ? 188 : 234, 16);
    }
    octave_key = key_dot(screen, 196, 162, 16);
    low_key = key_dot(screen, 196, 208, 16);
    label(screen, "Oct", 190, 184, 35);
    label(screen, "C", 201, 230, 20);
    status_label = label(screen, "", 18, 254, 210);
    label(screen, "UP/DOWN: note, OK: sound\nHold: UP/DN vol, OK mode", 18, 279, 210);
    lv_screen_load(screen);
    refresh();
}
static void on_key(bsp_btn_t key, bsp_btn_ev_t event, void *user) {
    (void)user;
    const input_t input = {key, event};
    (void)xQueueSend(inputs, &input, 0);
}
static void audio_worker(void *arg) {
    (void)arg;
    tone_request_t request;
    int16_t pcm[256];
    for (;;) {
        if (xQueueReceive(tones, &request, portMAX_DELAY) != pdTRUE) continue;
        bool ok = bsp_audio_set_format(SAX_SAMPLE_RATE, 16, 1) == ESP_OK;
        bsp_audio_set_volume(request.volume);
        ESP_LOGI(TAG, "Reference start midi=%d hz=%.2f volume=%u peak=%d format_ok=%d",
                 request.midi, sax_frequency(request.midi), (unsigned)request.volume,
                 SAX_TONE_PEAK, ok);
        sax_stream_t stream = {0};
        bool started = false;
        const double hz = sax_frequency(request.midi);
        /* Only this worker owns PCM. Notification is an out-of-band stop request,
         * including when OK is pressed before the start queue is consumed. */
        while (ok) {
            uint32_t stop_generation = 0;
            if (xTaskNotifyWait(0, UINT32_MAX, &stop_generation, 0) == pdTRUE &&
                stop_generation == request.generation) break;
            sax_stream_render(&stream, pcm, 256, hz);
            ok = bsp_audio_write(pcm, sizeof(pcm)) == ESP_OK;
            started = true;
        }
        if (ok && started) {
            sax_stream_release(&stream, pcm, hz);
            ok = bsp_audio_write(pcm, SAX_RELEASE_SAMPLES * sizeof(*pcm)) == ESP_OK;
        }
        /* Drain old PCM before publishing completion. No UI access here. */
        if (ok) {
            for (int i = 0; i < 256; ++i) pcm[i] = 0;
            for (int i = 0; ok && i < 6; ++i) ok = bsp_audio_write(pcm, sizeof(pcm)) == ESP_OK;
        }
        ESP_LOGI(TAG, "Reference completed: pcm_ok=%d (speaker output unverified)", ok);
        /* Generation-tagged stops cannot cancel a later playback if an error
         * races with an OK press immediately before this acknowledgement. */
        const tone_result_t result = {request.generation, ok};
        xQueueOverwrite(results, &result);
    }
}
void app_main(void) {
    inputs = xQueueCreate(16, sizeof(input_t));
    tones = xQueueCreate(1, sizeof(tone_request_t));
    results = xQueueCreate(1, sizeof(tone_result_t));
    if (!inputs || !tones || !results) { ESP_LOGE(TAG, "queue allocation failed"); return; }
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) return;
    audio_ok = bsp_audio_init() == ESP_OK;
    if (audio_ok && xTaskCreate(audio_worker, "sax_audio", 4096, NULL, 4, &audio_task) != pdPASS) audio_ok = false;
    bool battery_ok = bsp_battery_init() == ESP_OK;
    if (!bsp_lvgl_lock(1000)) return;
    build_ui();
    bsp_lvgl_unlock();
    if (bsp_button_init(on_key, NULL) != ESP_OK) { ESP_LOGE(TAG, "buttons unavailable"); return; }
    bsp_display_backlight(65);
    last_activity = esp_timer_get_time();
    int64_t last_battery = -10000000;
    for (;;) {
        input_t input;
        if (xQueueReceive(inputs, &input, pdMS_TO_TICKS(50)) == pdTRUE) {
            last_activity = esp_timer_get_time();
            if (dimmed) { dimmed = false; bsp_display_backlight(65); }
            if (audio_ok && input.key == BSP_BTN_OK &&
                (input.event == BSP_BTN_CLICK || input.event == BSP_BTN_DOUBLE)) {
                sax_player_action_t action = sax_player_toggle(&player);
                if (action == SAX_PLAYER_START) {
                    tone_request_t r = {sax_note(selected)->written - 9, ++generation, volume};
                    if (xQueueSend(tones, &r, 0) != pdTRUE) sax_player_complete(&player);
                } else if (action == SAX_PLAYER_STOP) xTaskNotify(audio_task, generation, eSetValueWithOverwrite);
                if (bsp_lvgl_lock(100)) { refresh(); bsp_lvgl_unlock(); }
            } else if (player == SAX_STOPPED) {
                if (input.event == BSP_BTN_CLICK || input.event == BSP_BTN_DOUBLE) {
                    if (input.key == BSP_BTN_UP && selected > 0) --selected;
                    else if (input.key == BSP_BTN_DOWN && selected + 1 < SAX_NOTE_COUNT) ++selected;
                } else if (input.event == BSP_BTN_LONG) {
                    if (input.key == BSP_BTN_OK) concert = !concert;
                    else if (input.key == BSP_BTN_UP) volume = sax_volume_step(volume, true);
                    else if (input.key == BSP_BTN_DOWN) volume = sax_volume_step(volume, false);
                }
                if (bsp_lvgl_lock(100)) { refresh(); bsp_lvgl_unlock(); }
            }
        }
        tone_result_t result;
        if (xQueueReceive(results, &result, 0) == pdTRUE && result.generation == generation) {
            sax_player_complete(&player);
            if (bsp_lvgl_lock(100)) {
                refresh();
                if (!result.ok) lv_label_set_text(status_label, "Playback failed. OK: retry");
                bsp_lvgl_unlock();
            }
        }
        int64_t now = esp_timer_get_time();
        if (now - last_battery > 10000000) {
            last_battery = now;
            int soc = battery_ok ? bsp_battery_soc() : -1;
            if (bsp_lvgl_lock(100)) {
                char text[12];
                if (soc < 0) snprintf(text, sizeof(text), "--%%");
                else snprintf(text, sizeof(text), "%d%%", soc);
                lv_label_set_text(battery_label, text);
                bsp_lvgl_unlock();
            }
        }
        if (!dimmed && player == SAX_STOPPED && now - last_activity > 60000000) {
            bsp_display_backlight(5);
            dimmed = true;
        }
    }
}
