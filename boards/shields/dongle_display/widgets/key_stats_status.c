/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <zephyr/kernel.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/key_stats_changed.h>
#include <zmk/key_stats.h>

#include "key_stats_status.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void format_count(char *buf, size_t len, uint32_t value) {
    if (value < 1000U) {
        snprintf(buf, len, "%u", value);
    } else if (value < 1000000U) {
        snprintf(buf, len, "%uk", value / 1000U);
    } else if (value < 1000000000U) {
        snprintf(buf, len, "%um", value / 1000000U);
    } else {
        snprintf(buf, len, "%ub", value / 1000000000U);
    }
}

static void set_total(struct zmk_widget_key_stats_status *widget, uint32_t total) {
    char count[8];
    format_count(count, sizeof(count), total);
    lv_label_set_text(widget->total_value_label, count);
}

static void key_stats_update_cb(struct zmk_key_stats_changed state) {
    struct zmk_widget_key_stats_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_total(widget, state.total);
    }
}

static struct zmk_key_stats_changed get_state(const zmk_event_t *eh) {
    const struct zmk_key_stats_changed *ev = as_zmk_key_stats_changed(eh);
    return ev != NULL ? *ev : (struct zmk_key_stats_changed) {
        .total = zmk_key_stats_total(),
        .today = 0,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_key_stats_status, struct zmk_key_stats_changed,
                            key_stats_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_key_stats_status, zmk_key_stats_changed);

void zmk_widget_key_stats_status_set_dashboard(
    struct zmk_widget_key_stats_status *widget, bool enabled) {
    ARG_UNUSED(enabled);
    set_total(widget, zmk_key_stats_total());
}

int zmk_widget_key_stats_status_init(
    struct zmk_widget_key_stats_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);
    lv_obj_set_size(widget->obj, 38, 8);

    widget->total_label = lv_label_create(widget->obj);
    lv_label_set_text(widget->total_label, "T");
    lv_obj_set_style_text_font(widget->total_label, &lv_font_unscii_8, 0);
    lv_obj_align(widget->total_label, LV_ALIGN_LEFT_MID, 0, 0);

    widget->total_value_label = lv_label_create(widget->obj);
    lv_obj_set_width(widget->total_value_label, 31);
    lv_obj_set_style_text_font(widget->total_value_label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_align(widget->total_value_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(widget->total_value_label, LV_ALIGN_RIGHT_MID, 0, 0);

    widget->today_label = NULL;
    widget->today_value_label = NULL;

    sys_slist_append(&widgets, &widget->node);
    widget_key_stats_status_init();
    set_total(widget, zmk_key_stats_total());
    return 0;
}

lv_obj_t *zmk_widget_key_stats_status_obj(
    struct zmk_widget_key_stats_status *widget) {
    return widget->obj;
}
