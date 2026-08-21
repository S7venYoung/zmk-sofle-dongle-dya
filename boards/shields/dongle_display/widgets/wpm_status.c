/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/wpm.h>

#include "wpm_status.h"

LV_IMG_DECLARE(bmw_brace_image);

#define PEAK_WPM_HOLD_MS 3000
#define BMW_WPM_MAX 120

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static int peak_wpm;
static int64_t peak_updated_at;

struct wpm_status_state {
    int current;
    int peak;
};

static int update_peak(int current) {
    int64_t now = k_uptime_get();

    if (current >= peak_wpm) {
        peak_wpm = current;
        peak_updated_at = now;
    } else if (now - peak_updated_at >= PEAK_WPM_HOLD_MS) {
        peak_wpm = current;
        peak_updated_at = now;
    }

    return peak_wpm;
}

static struct wpm_status_state get_state(const zmk_event_t *eh) {
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    int current = ev != NULL ? ev->state : zmk_wpm_get_state();

    return (struct wpm_status_state) {
        .current = current,
        .peak = update_peak(current),
    };
}

static lv_point_precise_t brace_target(int value, bool right) {
    int level = CLAMP(value, 0, BMW_WPM_MAX);
    lv_point_precise_t low = {22, 41};
    lv_point_precise_t middle = {42, 27};
    lv_point_precise_t high = {22, 5};
    lv_point_precise_t target;

    if (level <= BMW_WPM_MAX / 2) {
        target.x = low.x + (middle.x - low.x) * level / (BMW_WPM_MAX / 2);
        target.y = low.y + (middle.y - low.y) * level / (BMW_WPM_MAX / 2);
    } else {
        int upper = level - BMW_WPM_MAX / 2;
        target.x = middle.x + (high.x - middle.x) * upper / (BMW_WPM_MAX / 2);
        target.y = middle.y + (high.y - middle.y) * upper / (BMW_WPM_MAX / 2);
    }

    if (right) {
        target.x = 127 - target.x;
    }
    return target;
}

static void set_wpm(struct zmk_widget_wpm_status *widget, struct wpm_status_state state) {
    if (state.current != widget->last_current) {
        widget->last_current = state.current;
        lv_label_set_text_fmt(widget->current_value, "%d", state.current);
        widget->current_points[1] = brace_target(state.current, false);
        lv_line_set_points(widget->current_needle, widget->current_points, 2);
    }

    if (state.peak != widget->last_peak) {
        widget->last_peak = state.peak;
        lv_label_set_text_fmt(widget->peak_value, "%d", state.peak);
        widget->peak_points[1] = brace_target(state.peak, true);
        lv_line_set_points(widget->peak_needle, widget->peak_points, 2);
    }
}

static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_wpm_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_wpm(widget, state);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state,
                            wpm_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

void zmk_widget_wpm_status_refresh(struct zmk_widget_wpm_status *widget) {
    widget->last_current = -1;
    widget->last_peak = -1;
    set_wpm(widget, get_state(NULL));
}

static lv_obj_t *create_value_label(lv_obj_t *parent, lv_coord_t x) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, x, 20);
    lv_obj_set_width(label, 20);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    return label;
}

int zmk_widget_wpm_status_init(struct zmk_widget_wpm_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);
    lv_obj_set_size(widget->obj, 128, 48);

    widget->background = lv_img_create(widget->obj);
    lv_img_set_src(widget->background, &bmw_brace_image);
    lv_obj_align(widget->background, LV_ALIGN_TOP_LEFT, 0, 0);

    widget->current_value = create_value_label(widget->obj, 5);
    widget->peak_value = create_value_label(widget->obj, 103);

    widget->current_points[0] = (lv_point_precise_t){31, 34};
    widget->current_points[1] = (lv_point_precise_t){22, 41};
    widget->current_needle = lv_line_create(widget->obj);
    lv_line_set_points(widget->current_needle, widget->current_points, 2);
    lv_obj_set_style_line_width(widget->current_needle, 1, 0);

    widget->peak_points[0] = (lv_point_precise_t){96, 34};
    widget->peak_points[1] = (lv_point_precise_t){105, 41};
    widget->peak_needle = lv_line_create(widget->obj);
    lv_line_set_points(widget->peak_needle, widget->peak_points, 2);
    lv_obj_set_style_line_width(widget->peak_needle, 1, 0);

    widget->last_current = -1;
    widget->last_peak = -1;
    sys_slist_append(&widgets, &widget->node);
    widget_wpm_status_init();
    zmk_widget_wpm_status_refresh(widget);
    return 0;
}

lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget) {
    return widget->obj;
}
