/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

struct zmk_widget_fight_status {
    lv_obj_t *obj;
    lv_obj_t *image;
    lv_timer_t *timer;
    uint8_t frames[2];
    uint8_t idle_divider;
};

int zmk_widget_fight_status_init(struct zmk_widget_fight_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_fight_status_obj(struct zmk_widget_fight_status *widget);
