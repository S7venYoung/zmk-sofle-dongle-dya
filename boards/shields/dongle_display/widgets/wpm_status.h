/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_wpm_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *background;
    lv_obj_t *current_value;
    lv_obj_t *peak_value;
    lv_obj_t *current_needle;
    lv_obj_t *peak_needle;
    lv_point_precise_t current_points[2];
    lv_point_precise_t peak_points[2];
    int last_current;
    int last_peak;
};

int zmk_widget_wpm_status_init(struct zmk_widget_wpm_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget);
void zmk_widget_wpm_status_refresh(struct zmk_widget_wpm_status *widget);
