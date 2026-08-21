/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include "custom_status_screen.h"
#include "widgets/battery_status.h"
#include "widgets/key_stats_status.h"
#include "widgets/layer_status.h"
#include "widgets/output_status.h"
#include "widgets/wpm_status.h"

#include <zmk/display.h>

static struct zmk_widget_output_status output_status_widget;
static struct zmk_widget_dongle_battery_status dongle_battery_status_widget;
static struct zmk_widget_wpm_status wpm_status_widget;

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_LAYER)
static struct zmk_widget_layer_status layer_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_KEY_STATS)
static struct zmk_widget_key_stats_status key_stats_status_widget;
#endif

static lv_style_t global_style;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_style_init(&global_style);
    lv_style_set_bg_color(&global_style, lv_color_white());
    lv_style_set_bg_opa(&global_style, LV_OPA_COVER);
    lv_style_set_text_color(&global_style, lv_color_black());
    lv_style_set_text_font(&global_style, &lv_font_unscii_8);
    lv_style_set_text_letter_space(&global_style, 1);
    lv_obj_add_style(screen, &global_style, LV_PART_MAIN);

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_WPM)
    zmk_widget_wpm_status_init(&wpm_status_widget, screen);
    lv_obj_align(zmk_widget_wpm_status_obj(&wpm_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    zmk_widget_output_status_init(&output_status_widget, screen);
    zmk_widget_output_status_set_dashboard(&output_status_widget, true);
    zmk_widget_output_status_set_compact(&output_status_widget, true);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_BOTTOM_LEFT, 35, -5);

#if IS_ENABLED(CONFIG_ZMK_KEY_STATS)
    zmk_widget_key_stats_status_init(&key_stats_status_widget, screen);
    zmk_widget_key_stats_status_set_dashboard(&key_stats_status_widget, true);
    lv_obj_align(zmk_widget_key_stats_status_obj(&key_stats_status_widget),
                 LV_ALIGN_BOTTOM_LEFT, 72, -5);
#endif

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_LAYER)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    zmk_widget_layer_status_set_dashboard(&layer_status_widget, true);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_CENTER, 0, -6);
#endif

#if IS_ENABLED(CONFIG_ZMK_BATTERY)
    zmk_widget_dongle_battery_status_init(&dongle_battery_status_widget, screen);
    zmk_widget_dongle_battery_status_set_bmw_layout(&dongle_battery_status_widget, true);
    lv_obj_align(zmk_widget_dongle_battery_status_obj(&dongle_battery_status_widget),
                 LV_ALIGN_BOTTOM_MID, 0, 0);
#endif

    return screen;
}
