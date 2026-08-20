/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_wpm_status
{
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *speedometer;
    lv_obj_t *gauge_arc;
    lv_obj_t *gauge_ticks[7];
    lv_obj_t *wpm_label;
    lv_obj_t *needle;
    lv_point_precise_t needle_points[2];
    lv_obj_t *bmw_outline;
    lv_point_precise_t bmw_outline_points[3];
    lv_obj_t *bmw_ticks[6];
    lv_point_precise_t bmw_tick_points[6][2];
    lv_obj_t *bmw_needle;
    lv_point_precise_t bmw_needle_points[2];
    uint8_t display_mode;
    bool peak_mode;
    int last_value;
};

int zmk_widget_wpm_status_init(struct zmk_widget_wpm_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget);
void zmk_widget_wpm_status_refresh(struct zmk_widget_wpm_status *widget);
void zmk_widget_wpm_status_set_peak(struct zmk_widget_wpm_status *widget, bool peak);
void zmk_widget_wpm_status_set_dashboard(struct zmk_widget_wpm_status *widget, bool enabled);
void zmk_widget_wpm_status_set_bmw(struct zmk_widget_wpm_status *widget, bool enabled,
                                   bool right_side);
