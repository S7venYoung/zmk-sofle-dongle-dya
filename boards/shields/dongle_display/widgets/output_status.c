/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zmk/display.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/usb_conn_state_changed.h>

#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/events/ble_active_profile_changed.h>
#endif

#include "output_status.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    enum zmk_transport transport;
};

static struct output_status_state get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    struct zmk_endpoint_instance selected = zmk_endpoint_get_selected();
    enum zmk_transport transport = selected.transport;
    if (transport == ZMK_TRANSPORT_NONE) {
        transport = zmk_endpoint_get_preferred_transport();
    }
    return (struct output_status_state){.transport = transport};
}

static void set_status(struct zmk_widget_output_status *widget,
                       struct output_status_state state) {
    lv_label_set_text(lv_obj_get_child(widget->obj, 0),
                      state.transport == ZMK_TRANSPORT_BLE ? "B" : "U");
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_output_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_status(widget, state);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

int zmk_widget_output_status_init(struct zmk_widget_output_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);
    lv_obj_set_size(widget->obj, 12, 10);

    lv_obj_t *label = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

    sys_slist_append(&widgets, &widget->node);
    widget_output_status_init();
    set_status(widget, get_state(NULL));
    return 0;
}

lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget) {
    return widget->obj;
}

void zmk_widget_output_status_set_compact(struct zmk_widget_output_status *widget, bool compact) {
    ARG_UNUSED(compact);
}

void zmk_widget_output_status_set_dashboard(struct zmk_widget_output_status *widget, bool enabled) {
    ARG_UNUSED(enabled);
}
