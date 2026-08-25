/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

#include "fight_assets.h"
#include "fight_status.h"

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define IMAGE_PALETTE_BYTES 8
#define IMAGE_ROW_BYTES (DISPLAY_WIDTH / 8)
#define IMAGE_BYTES (IMAGE_PALETTE_BYTES + IMAGE_ROW_BYTES * DISPLAY_HEIGHT)
#define FIGHT_TICK_MS 100
#define HEAT_MAX 12
#define HEAT_DECAY_DELAY_MS 550

static atomic_t heat[2];
static atomic_t last_press[2];
static LV_ATTRIBUTE_MEM_ALIGN uint8_t framebuffer[IMAGE_BYTES];
static lv_img_dsc_t fight_image = {
    .header.cf = LV_COLOR_FORMAT_I1,
    .header.w = DISPLAY_WIDTH,
    .header.h = DISPLAY_HEIGHT,
    .data_size = IMAGE_BYTES,
    .data = framebuffer,
};

static bool is_left_position(uint32_t position) {
    /* Sofle transform: each row contains six left keys followed by right keys. */
    return position < 64U && (position % 13U) < 6U;
}

static void add_heat(uint8_t side) {
    atomic_val_t old;
    atomic_val_t next;
    do {
        old = atomic_get(&heat[side]);
        next = MIN(old + 2, HEAT_MAX);
    } while (!atomic_cas(&heat[side], old, next));
    atomic_set(&last_press[side], k_uptime_get_32());
}

static int position_listener_cb(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *event = as_zmk_position_state_changed(eh);
    if (event != NULL && event->state && event->position < 64U) {
        add_heat(is_left_position(event->position) ? 0 : 1);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(fighting_theme_positions, position_listener_cb);
ZMK_SUBSCRIPTION(fighting_theme_positions, zmk_position_state_changed);

static void set_pixel(uint8_t x, uint8_t y) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
        return;
    }
    framebuffer[IMAGE_PALETTE_BYTES + y * IMAGE_ROW_BYTES + x / 8] |=
        BIT(7 - (x & 7));
}

static void draw_frame(const struct fight_frame *frame, bool mirror) {
    for (uint16_t index = 0; index < frame->count; index++) {
        const struct fight_span *span = &frame->spans[index];
        if (mirror) {
            for (int x = DISPLAY_WIDTH - 1 - span->x1;
                 x <= DISPLAY_WIDTH - 1 - span->x0; x++) {
                set_pixel(x, span->y);
            }
        } else {
            for (int x = span->x0; x <= span->x1; x++) {
                set_pixel(x, span->y);
            }
        }
    }
}

static enum fight_action action_for_heat(atomic_val_t value) {
    if (value >= 9) {
        return FIGHT_ACTION_COMBO;
    }
    if (value >= 5) {
        return FIGHT_ACTION_HEAVY;
    }
    if (value > 0) {
        return FIGHT_ACTION_ATTACK;
    }
    return FIGHT_ACTION_IDLE;
}

static void decay_heat(uint8_t side, uint32_t now) {
    atomic_val_t value = atomic_get(&heat[side]);
    uint32_t pressed = (uint32_t)atomic_get(&last_press[side]);
    if (value > 0 && now - pressed >= HEAT_DECAY_DELAY_MS) {
        atomic_dec(&heat[side]);
    }
}

static void render(struct zmk_widget_fight_status *widget) {
    uint32_t now = k_uptime_get_32();
    decay_heat(0, now);
    decay_heat(1, now);

    atomic_val_t left_heat = atomic_get(&heat[0]);
    atomic_val_t right_heat = atomic_get(&heat[1]);
    enum fight_action left_action = action_for_heat(left_heat);
    enum fight_action right_action = action_for_heat(right_heat);

    memset(framebuffer + IMAGE_PALETTE_BYTES, 0, IMAGE_BYTES - IMAGE_PALETTE_BYTES);
    framebuffer[0] = framebuffer[1] = framebuffer[2] = framebuffer[3] = 0;
    framebuffer[4] = framebuffer[5] = framebuffer[6] = framebuffer[7] = 0xff;

    if (left_action != FIGHT_ACTION_IDLE || ++widget->idle_divider >= 3) {
        widget->frames[0] = (widget->frames[0] + 1) & 3U;
    }
    if (right_action != FIGHT_ACTION_IDLE || widget->idle_divider >= 3) {
        widget->frames[1] = (widget->frames[1] + 1) & 3U;
    }
    if (widget->idle_divider >= 3) {
        widget->idle_divider = 0;
    }

    draw_frame(fight_asset_frame(left_action, widget->frames[0]), true);
    draw_frame(fight_asset_frame(right_action, widget->frames[1]), false);
    lv_obj_invalidate(widget->image);
}

static void timer_cb(lv_timer_t *timer) {
    struct zmk_widget_fight_status *widget = timer->user_data;
    render(widget);
}

int zmk_widget_fight_status_init(struct zmk_widget_fight_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);
    lv_obj_set_size(widget->obj, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    widget->image = lv_img_create(widget->obj);
    lv_img_set_src(widget->image, &fight_image);
    lv_obj_align(widget->image, LV_ALIGN_TOP_LEFT, 0, 0);

    widget->frames[0] = 0;
    widget->frames[1] = 2;
    widget->idle_divider = 0;
    widget->timer = lv_timer_create(timer_cb, FIGHT_TICK_MS, widget);
    render(widget);
    return 0;
}

lv_obj_t *zmk_widget_fight_status_obj(struct zmk_widget_fight_status *widget) {
    return widget->obj;
}
