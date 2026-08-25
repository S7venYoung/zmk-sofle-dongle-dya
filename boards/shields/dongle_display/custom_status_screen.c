/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include "custom_status_screen.h"
#include "widgets/battery_status.h"
#include "widgets/fight_status.h"
#include "widgets/output_status.h"

#include <zmk/display.h>

static struct zmk_widget_output_status output_status_widget;
static struct zmk_widget_dongle_battery_status dongle_battery_status_widget;
static struct zmk_widget_fight_status fight_status_widget;

static lv_style_t global_style;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_style_init(&global_style);
    lv_style_set_bg_color(&global_style, lv_color_black());
    lv_style_set_bg_opa(&global_style, LV_OPA_COVER);
    lv_style_set_text_color(&global_style, lv_color_white());
    lv_style_set_text_font(&global_style, &lv_font_unscii_8);
    lv_style_set_text_letter_space(&global_style, 1);
    lv_obj_add_style(screen, &global_style, LV_PART_MAIN);

    zmk_widget_fight_status_init(&fight_status_widget, screen);
    lv_obj_align(zmk_widget_fight_status_obj(&fight_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_MID, 0, 0);

#if IS_ENABLED(CONFIG_ZMK_BATTERY)
    zmk_widget_dongle_battery_status_init(&dongle_battery_status_widget, screen);
    lv_obj_align(zmk_widget_dongle_battery_status_obj(&dongle_battery_status_widget),
                 LV_ALIGN_TOP_MID, 0, 2);
#endif

    return screen;
}
