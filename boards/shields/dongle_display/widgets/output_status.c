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

/* The default 9x14 transport glyphs, with the BMW theme's white-on-black palette. */
static const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t sym_usb_map[] = {
    0,0,0,0, 255,255,255,255,
    0x7f,0x00,0x41,0x00,0x55,0x00,0x41,0x00,0xff,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xff,0x80,
};
static const lv_img_dsc_t sym_usb = {
    .header.cf = LV_COLOR_FORMAT_I1, .header.w = 9, .header.h = 14,
    .data_size = 36, .data = sym_usb_map,
};

static const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t sym_bt_map[] = {
    0,0,0,0, 255,255,255,255,
    0x3e,0x00,0x67,0x00,0xe3,0x80,0xe9,0x80,0x8c,0x80,0xc9,0x80,0xe3,0x80,
    0xe3,0x80,0xc9,0x80,0x8c,0x80,0xe9,0x80,0xe3,0x80,0x67,0x00,0x3e,0x00,
};
static const lv_img_dsc_t sym_bt = {
    .header.cf = LV_COLOR_FORMAT_I1, .header.w = 9, .header.h = 14,
    .data_size = 36, .data = sym_bt_map,
};

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance selected;
    enum zmk_transport preferred;
};

static struct output_status_state get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return (struct output_status_state) {
        .selected = zmk_endpoint_get_selected(),
        .preferred = zmk_endpoint_get_preferred_transport(),
    };
}

static void set_status(struct zmk_widget_output_status *widget,
                       struct output_status_state state) {
    lv_obj_t *usb = lv_obj_get_child(widget->obj, 0);
    lv_obj_t *bt = lv_obj_get_child(widget->obj, 1);
    enum zmk_transport transport = state.selected.transport;

    if (transport == ZMK_TRANSPORT_NONE) {
        transport = state.preferred;
    }
    /* USB is the physical default when the endpoint has not reported yet. */
    if (transport == ZMK_TRANSPORT_NONE) {
        transport = ZMK_TRANSPORT_USB;
    }

    lv_obj_add_flag(usb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(bt, LV_OBJ_FLAG_HIDDEN);

    if (transport == ZMK_TRANSPORT_USB) {
        lv_obj_clear_flag(usb, LV_OBJ_FLAG_HIDDEN);
    } else if (transport == ZMK_TRANSPORT_BLE) {
        lv_obj_clear_flag(bt, LV_OBJ_FLAG_HIDDEN);
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
    lv_obj_set_size(widget->obj, 12, 14);

    lv_obj_t *usb = lv_img_create(widget->obj);
    lv_img_set_src(usb, &sym_usb);
    lv_obj_align(usb, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *bt = lv_img_create(widget->obj);
    lv_img_set_src(bt, &sym_bt);
    lv_obj_align(bt, LV_ALIGN_CENTER, 0, 0);

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
    set_status(widget, get_state(NULL));
}

void zmk_widget_output_status_set_dashboard(struct zmk_widget_output_status *widget, bool enabled) {
    ARG_UNUSED(enabled);
    set_status(widget, get_state(NULL));
}
