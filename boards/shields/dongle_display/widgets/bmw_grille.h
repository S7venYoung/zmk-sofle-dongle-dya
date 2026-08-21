/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

struct zmk_widget_bmw_grille {
    lv_obj_t *obj;
    lv_obj_t *left_outline;
    lv_obj_t *right_outline;
    lv_point_precise_t left_points[5];
    lv_point_precise_t right_points[5];
};

int zmk_widget_bmw_grille_init(struct zmk_widget_bmw_grille *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_bmw_grille_obj(struct zmk_widget_bmw_grille *widget);
