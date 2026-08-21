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
#include <zmk/usb.h>

#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/ble.h>
#include <zmk/events/ble_active_profile_changed.h>
#endif

#include "output_status.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance selected;
    enum zmk_transport preferred;
    int profile;
    bool usb_ready;
};

static struct output_status_state get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return (struct output_status_state) {
        .selected = zmk_endpoint_get_selected(),
        .preferred = zmk_endpoint_get_preferred_transport(),
#if IS_ENABLED(CONFIG_ZMK_BLE)
        .profile = zmk_ble_active_profile_index(),
#else
        .profile = 0,
#endif
        .usb_ready = zmk_usb_is_hid_ready(),
    };
}

static void set_status(struct zmk_widget_output_status *widget,
                       struct output_status_state state) {
    lv_obj_t *label = lv_obj_get_child(widget->obj, 0);
    enum zmk_transport transport = state.selected.transport;

    if (transport == ZMK_TRANSPORT_NONE) {
        transport = state.preferred;
    }

    if (transport == ZMK_TRANSPORT_USB) {
        lv_label_set_text(label, state.usb_ready ? "USB" : "USB-");
    } else if (transport == ZMK_TRANSPORT_BLE) {
        lv_label_set_text_fmt(label, state.selected.transport == ZMK_TRANSPORT_BLE ? "BLE%d"
                                                                                  : "BLE%d-",
                              state.profile + 1);
    } else {
        lv_label_set_text(label, "---");
    }
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
    lv_obj_set_size(widget->obj, 24, 8);

    lv_obj_t *label = lv_label_create(widget->obj);
    lv_obj_set_width(label, 24);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    sys_slist_append(&widgets, &widget->node);
    widget_output_status_init();
    return 0;
}

lv_obj_t *zmk_widget_output_status_obj(struct zmk_widget_output_status *widget) {
    return widget->obj;
}

void zmk_widget_output_status_set_compact(struct zmk_widget_output_status *widget, bool compact) {
    ARG_UNUSED(compact);
    set_status(widget, get_state(NULL));
}

void zmk_widget_output_status_set_dashboard(struct zmk_widget_output_status *widget, bool enabled) {
    ARG_UNUSED(widget);
    ARG_UNUSED(enabled);
}
