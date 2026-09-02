/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include "custom_status_screen.h"
#include "widgets/fight_status.h"

#include <zmk/display.h>

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

    return screen;
}
