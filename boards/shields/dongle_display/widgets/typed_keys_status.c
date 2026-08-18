/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/keys.h>

#include "typed_keys_status.h"

#define TYPED_KEYS_MAX 5
#define HID_KEY_A 0x04
#define HID_KEY_Z 0x1D
#define HID_KEY_BACKSPACE 0x2A

struct typed_keys_state {
    char text[20];
};

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static char typed_keys[TYPED_KEYS_MAX + 1];
static size_t typed_length;
static char layer_letter;

static void append_letter(char letter) {
    if (typed_length == TYPED_KEYS_MAX) {
        memmove(typed_keys, typed_keys + 1, TYPED_KEYS_MAX - 1);
        typed_length--;
    }

    typed_keys[typed_length++] = letter;
    typed_keys[typed_length] = '\0';
}

static struct typed_keys_state typed_keys_get_state(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *layer_ev = as_zmk_layer_state_changed(eh);
    if (layer_ev != NULL && layer_ev->state) {
        layer_letter = '\0';
    }

    const struct zmk_keycode_state_changed *key_ev = as_zmk_keycode_state_changed(eh);
    if (key_ev != NULL && key_ev->state && key_ev->usage_page == HID_USAGE_KEY) {
        if (is_mod(key_ev->usage_page, key_ev->keycode)) {
            /* Start a fresh display sequence for the shortcut being entered. */
            typed_length = 0;
            typed_keys[0] = '\0';
            layer_letter = '\0';
        } else if (key_ev->keycode >= HID_KEY_A && key_ev->keycode <= HID_KEY_Z) {
            char letter = 'A' + (key_ev->keycode - HID_KEY_A);
            append_letter(letter);
            if (zmk_keymap_highest_layer_active() != 0) {
                layer_letter = letter;
            }
        } else if (key_ev->keycode == HID_KEY_BACKSPACE && typed_length > 0) {
            typed_keys[--typed_length] = '\0';
        }
    }

    struct typed_keys_state state = {};
    uint8_t layer = zmk_keymap_highest_layer_active();
    if (layer != 0) {
        const char *layer_name = zmk_keymap_layer_name(layer);
        if (layer_name == NULL) {
            if (layer_letter == '\0') {
                snprintf(state.text, sizeof(state.text), "L%u", layer);
            } else {
                snprintf(state.text, sizeof(state.text), "L%u %c", layer, layer_letter);
            }
        } else if (layer_letter == '\0') {
            snprintf(state.text, sizeof(state.text), "%s", layer_name);
        } else {
            snprintf(state.text, sizeof(state.text), "%s %c", layer_name, layer_letter);
        }
    } else if (typed_length == 0) {
        state.text[0] = '\0';
    } else {
        snprintf(state.text, sizeof(state.text), "%s", typed_keys);
    }

    return state;
}

static void set_typed_keys(struct zmk_widget_typed_keys_status *widget,
                           struct typed_keys_state state) {
    lv_label_set_text(widget->obj, state.text);
}

static void typed_keys_update_cb(struct typed_keys_state state) {
    struct zmk_widget_typed_keys_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_typed_keys(widget, state); }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_typed_keys_status, struct typed_keys_state,
                            typed_keys_update_cb, typed_keys_get_state)
ZMK_SUBSCRIPTION(widget_typed_keys_status, zmk_keycode_state_changed);
ZMK_SUBSCRIPTION(widget_typed_keys_status, zmk_layer_state_changed);

int zmk_widget_typed_keys_status_init(struct zmk_widget_typed_keys_status *widget,
                                      lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);
    lv_obj_set_width(widget->obj, 126);
    lv_obj_set_style_text_align(widget->obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(widget->obj, &lv_font_unscii_16, 0);
    lv_label_set_long_mode(widget->obj, LV_LABEL_LONG_CLIP);

    sys_slist_append(&widgets, &widget->node);
    widget_typed_keys_status_init();
    return 0;
}

lv_obj_t *zmk_widget_typed_keys_status_obj(struct zmk_widget_typed_keys_status *widget) {
    return widget->obj;
}
