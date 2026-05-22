#include "lvgl.h"

#include <stdint.h>
#include <stdio.h>
#include "pratice_demo.h"

static lv_obj_t *chart = NULL;
static lv_chart_series_t *wave_ser = NULL;
static lv_timer_t *wave_timer = NULL;
static uint32_t wave_index = 0;

static const int16_t wave_table[] = {
    50, 56, 62, 68, 73, 78, 82, 86,
    89, 92, 94, 96, 97, 98, 98, 97,
    96, 94, 92, 89, 86, 82, 78, 73,
    68, 62, 56, 50, 44, 38, 32, 27,
    22, 18, 14, 11, 8,  6,  4,  3,
    2,  2,  3,  4,  6,  8,  11, 14,
    18, 22, 27, 32, 38, 44
};

#define WAVE_TABLE_SIZE (sizeof(wave_table) / sizeof(wave_table[0]))

static void wave_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    if (chart == NULL || wave_ser == NULL) {
        return;
    }

    int value = wave_table[wave_index % WAVE_TABLE_SIZE];

    lv_chart_set_next_value(chart, wave_ser, value);

    wave_index++;
}

static void create_wave_chart(lv_obj_t *parent)
{
    chart = lv_chart_create(parent);
    lv_obj_set_size(chart, 700, 260);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 20);

    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(chart, 200);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    /*
     * 这个是把折线上的小圆点隐藏掉。
     * 不然每个采样点都会有一个点，像示波器就不好看。
     */
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);

    wave_ser = lv_chart_add_series(chart,
                                   lv_palette_main(LV_PALETTE_GREEN),
                                   LV_CHART_AXIS_PRIMARY_Y);

    /*
     * 先填满一屏初始数据。
     * 不然刚开始 chart 左边可能是空的。
     */
    for (uint32_t i = 0; i < 200; i++) {
        lv_chart_set_next_value(chart, wave_ser, 50);
    }

    /*
     * 每 30ms 加一个点。
     * 30ms 大约是 33FPS 左右，看起来就是实时滚动波形。
     */
    wave_timer = lv_timer_create(wave_timer_cb, 30, NULL);
}

void practice_demo(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "LVGL Wave Chart Demo");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    create_wave_chart(scr);

    lv_obj_t *tip = lv_label_create(scr);
    lv_label_set_text(tip, "Fake waveform by lv_chart + lv_timer");
    lv_obj_align(tip, LV_ALIGN_BOTTOM_MID, 0, -20);
}