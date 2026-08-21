/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

#include "battery_status.h"

#define BMW_BAR_WIDTH 46
#define BMW_BAR_HEIGHT 3
#define BMW_BAR_COUNT 2

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static uint8_t levels[BMW_BAR_COUNT];
static bool valid[BMW_BAR_COUNT];

static void update_bar(struct zmk_widget_dongle_battery_status *widget, uint8_t index) {
    lv_obj_t *track = lv_obj_get_child(widget->obj, index);
    lv_obj_t *fill = lv_obj_get_child(track, 0);
    uint8_t level = valid[index] ? levels[index] : 0;

    lv_obj_set_size(fill, MAX(1, DIV_ROUND_UP(BMW_BAR_WIDTH * level, 100)), BMW_BAR_HEIGHT);
    lv_obj_align(fill, index == 0 ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID, 0, 0);

    if (level == 0) {
        lv_obj_add_flag(fill, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(fill, LV_OBJ_FLAG_HIDDEN);
    }
}

void zmk_widget_dongle_battery_status_refresh(
    struct zmk_widget_dongle_battery_status *widget) {
    for (uint8_t i = 0; i < BMW_BAR_COUNT; i++) {
        update_bar(widget, i);
    }
}

static void battery_status_update_cb(struct zmk_peripheral_battery_state_changed state) {
    if (state.source >= BMW_BAR_COUNT) {
        return;
    }

    levels[state.source] = state.state_of_charge;
    valid[state.source] = true;

    struct zmk_widget_dongle_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        update_bar(widget, state.source);
    }
}

static struct zmk_peripheral_battery_state_changed battery_status_get_state(
    const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    return ev != NULL ? *ev : (struct zmk_peripheral_battery_state_changed){0};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_dongle_battery_status,
                            struct zmk_peripheral_battery_state_changed,
                            battery_status_update_cb, battery_status_get_state)
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_peripheral_battery_state_changed);

static lv_obj_t *create_track(lv_obj_t *parent) {
    lv_obj_t *track = lv_obj_create(parent);
    lv_obj_remove_style_all(track);
    lv_obj_set_size(track, BMW_BAR_WIDTH, BMW_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(track, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(track, lv_color_black(), 0);
    lv_obj_set_style_border_width(track, 1, 0);
    lv_obj_set_style_radius(track, 0, 0);

    lv_obj_t *fill = lv_obj_create(track);
    lv_obj_remove_style_all(fill);
    lv_obj_set_style_bg_color(fill, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    lv_obj_set_size(fill, 1, BMW_BAR_HEIGHT);
    return track;
}

int zmk_widget_dongle_battery_status_init(
    struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);
    lv_obj_set_size(widget->obj, 128, BMW_BAR_HEIGHT);

    lv_obj_t *left = create_track(widget->obj);
    lv_obj_align(left, LV_ALIGN_BOTTOM_LEFT, 1, 0);

    lv_obj_t *right = create_track(widget->obj);
    lv_obj_align(right, LV_ALIGN_BOTTOM_RIGHT, -1, 0);

    sys_slist_append(&widgets, &widget->node);
    widget_dongle_battery_status_init();
    zmk_widget_dongle_battery_status_refresh(widget);
    return 0;
}

lv_obj_t *zmk_widget_dongle_battery_status_obj(
    struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}

void zmk_widget_dongle_battery_status_set_split_layout(
    struct zmk_widget_dongle_battery_status *widget, bool enabled) {
    ARG_UNUSED(enabled);
    zmk_widget_dongle_battery_status_refresh(widget);
}

void zmk_widget_dongle_battery_status_set_dashboard_layout(
    struct zmk_widget_dongle_battery_status *widget, bool enabled) {
    ARG_UNUSED(enabled);
    zmk_widget_dongle_battery_status_refresh(widget);
}

void zmk_widget_dongle_battery_status_set_bmw_layout(
    struct zmk_widget_dongle_battery_status *widget, bool enabled) {
    ARG_UNUSED(enabled);
    zmk_widget_dongle_battery_status_refresh(widget);
}
