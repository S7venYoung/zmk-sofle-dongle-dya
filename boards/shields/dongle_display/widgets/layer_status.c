/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

#include "layer_status.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_status_state {
    uint8_t index;
};

static const char *layer_code(uint8_t index) {
    if (index == 1) {
        return "N";
    }
    if (index == 2) {
        return "R";
    }
    return "D";
}

static struct layer_status_state get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return (struct layer_status_state) {
        .index = zmk_keymap_highest_layer_active(),
    };
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_layer_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        lv_label_set_text(widget->obj, layer_code(state.index));
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state,
                            layer_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

void zmk_widget_layer_status_refresh(struct zmk_widget_layer_status *widget) {
    lv_label_set_text(widget->obj, layer_code(zmk_keymap_highest_layer_active()));
}

void zmk_widget_layer_status_set_dashboard(struct zmk_widget_layer_status *widget, bool enabled) {
    ARG_UNUSED(enabled);
    zmk_widget_layer_status_refresh(widget);
}

int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);
    lv_obj_set_width(widget->obj, 20);
    lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(widget->obj, &lv_font_unscii_16, 0);

    sys_slist_append(&widgets, &widget->node);
    widget_layer_status_init();
    return 0;
}

lv_obj_t *zmk_widget_layer_status_obj(struct zmk_widget_layer_status *widget) {
    return widget->obj;
}
