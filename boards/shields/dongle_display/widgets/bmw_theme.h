/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

struct zmk_widget_bmw_theme {
    lv_obj_t *obj;
};

int zmk_widget_bmw_theme_init(struct zmk_widget_bmw_theme *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_bmw_theme_obj(struct zmk_widget_bmw_theme *widget);
