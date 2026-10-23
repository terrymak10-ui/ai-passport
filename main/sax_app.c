/* Offline first increment. BSP is unchanged; all UI is application-owned. */
#include "sax_music.h"
#include "sax_ui.h"
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
static bool audio_ok, concert;
static size_t selected = 72 - SAX_WRITTEN_MIN; /* C5, above the weakest bass range */
static unsigned generation;
static sax_player_state_t player = SAX_STOPPED;
static TaskHandle_t audio_task;
static uint8_t volume = SAX_VOLUME_DEFAULT;
static int64_t last_activity;
static bool dimmed;
static const char *TAG = "sax";

static sax_ui_state_t ui_state(void) {
    return (sax_ui_state_t){selected, concert, audio_ok, volume, player};
}
static void refresh(void) {
    const sax_ui_state_t state = ui_state();
    sax_ui_refresh(&state);
}
static void build_ui(void) {
    const sax_ui_state_t state = ui_state();
    sax_ui_create(&state);
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
        ESP_LOGI(TAG, "Reference start midi=%d hz=%.2f volume=%u peak_cap=%d rich=%u format_ok=%d",
                 request.midi, sax_frequency(request.midi), (unsigned)request.volume,
                 SAX_TONE_PEAK, sax_tone_richness(sax_frequency(request.midi)), ok);
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
                if (!result.ok) sax_ui_playback_error();
                bsp_lvgl_unlock();
            }
        }
        int64_t now = esp_timer_get_time();
        if (now - last_battery > 10000000) {
            last_battery = now;
            int soc = battery_ok ? bsp_battery_soc() : -1;
            if (bsp_lvgl_lock(100)) {
                sax_ui_battery(soc);
                bsp_lvgl_unlock();
            }
        }
        if (!dimmed && player == SAX_STOPPED && now - last_activity > 60000000) {
            bsp_display_backlight(5);
            dimmed = true;
        }
    }
}
