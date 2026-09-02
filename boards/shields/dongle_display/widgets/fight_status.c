/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/position_state_changed.h>

#include "fight_assets.h"
#include "fight_status.h"

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define IMAGE_PALETTE_BYTES 8
#define IMAGE_ROW_BYTES (DISPLAY_WIDTH / 8)
#define IMAGE_BYTES (IMAGE_PALETTE_BYTES + IMAGE_ROW_BYTES * DISPLAY_HEIGHT)
#define FIGHT_TICK_MS 100
#define WPM_WINDOW_MS 5000U
#define WPM_PRESS_HISTORY 64
#define WPM_SLOW_THRESHOLD 5
#define WPM_MID_THRESHOLD 30
#define WPM_FAST_THRESHOLD 70
#define HUD_BAR_WIDTH 46
#define HUD_BAR_Y 2
#define FIGHTER_MOVE_STEP 3U
#define FIGHTER_MAX_OFFSET 14U

struct side_press_history {
    uint32_t timestamps[WPM_PRESS_HISTORY];
    uint8_t head;
    uint8_t count;
};

static struct side_press_history histories[2];
static struct k_spinlock history_lock;
static uint8_t battery_levels[2];
static bool battery_valid[2];
static LV_ATTRIBUTE_MEM_ALIGN uint8_t framebuffer[IMAGE_BYTES];
static lv_image_dsc_t fight_image = {
    .header.cf = LV_COLOR_FORMAT_I1,
    .header.w = DISPLAY_WIDTH,
    .header.h = DISPLAY_HEIGHT,
    .data_size = IMAGE_BYTES,
    .data = framebuffer,
};

static bool is_left_position(uint32_t position) {
    /* Sofle transform: six left keys then six right keys on each 13-slot row. */
    return position < 64U && (position % 13U) < 6U;
}

static void record_press(uint8_t side, uint32_t now) {
    k_spinlock_key_t key = k_spin_lock(&history_lock);
    struct side_press_history *history = &histories[side];
    history->timestamps[history->head] = now;
    history->head = (history->head + 1U) % WPM_PRESS_HISTORY;
    if (history->count < WPM_PRESS_HISTORY) {
        history->count++;
    }
    k_spin_unlock(&history_lock, key);
}

static int position_listener_cb(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *event = as_zmk_position_state_changed(eh);
    if (event != NULL && event->state && event->position < 64U) {
        record_press(is_left_position(event->position) ? 0U : 1U, k_uptime_get_32());
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(fighting_theme_positions, position_listener_cb);
ZMK_SUBSCRIPTION(fighting_theme_positions, zmk_position_state_changed);

static int battery_listener_cb(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *event =
        as_zmk_peripheral_battery_state_changed(eh);
    if (event != NULL && event->source < ARRAY_SIZE(battery_levels)) {
        battery_levels[event->source] = MIN(event->state_of_charge, 100U);
        battery_valid[event->source] = true;
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(fighting_theme_batteries, battery_listener_cb);
ZMK_SUBSCRIPTION(fighting_theme_batteries, zmk_peripheral_battery_state_changed);

static uint8_t side_wpm(uint8_t side, uint32_t now) {
    uint8_t active = 0;
    k_spinlock_key_t key = k_spin_lock(&history_lock);
    const struct side_press_history *history = &histories[side];
    for (uint8_t i = 0; i < history->count; i++) {
        uint8_t index = (history->head + WPM_PRESS_HISTORY - 1U - i) % WPM_PRESS_HISTORY;
        if (now - history->timestamps[index] <= WPM_WINDOW_MS) {
            active++;
        } else {
            break;
        }
    }
    k_spin_unlock(&history_lock, key);

    /* Standard WPM: characters per minute divided by five, over a 5 s window. */
    return MIN((active * 12U + 2U) / 5U, UINT8_MAX);
}

static enum fight_action action_for_wpm(uint8_t wpm) {
    if (wpm >= WPM_FAST_THRESHOLD) {
        return FIGHT_ACTION_FAST;
    }
    if (wpm >= WPM_MID_THRESHOLD) {
        return FIGHT_ACTION_MID;
    }
    if (wpm >= WPM_SLOW_THRESHOLD) {
        return FIGHT_ACTION_SLOW;
    }
    return FIGHT_ACTION_IDLE;
}

static void set_pixel(uint8_t x, uint8_t y) {
    if (x < DISPLAY_WIDTH && y < DISPLAY_HEIGHT) {
        framebuffer[IMAGE_PALETTE_BYTES + y * IMAGE_ROW_BYTES + x / 8] |= BIT(7 - (x & 7));
    }
}

static void draw_hline(uint8_t x, uint8_t y, uint8_t width) {
    for (uint8_t i = 0; i < width; i++) {
        set_pixel(x + i, y);
    }
}

static void draw_battery_bar(uint8_t side) {
    const uint8_t x = side == 0 ? 1 : DISPLAY_WIDTH - 1 - HUD_BAR_WIDTH;
    const uint8_t level = battery_valid[side] ? battery_levels[side] : 0;
    const uint8_t fill = (HUD_BAR_WIDTH - 2U) * level / 100U;

    draw_hline(x, HUD_BAR_Y, HUD_BAR_WIDTH);
    draw_hline(x, HUD_BAR_Y + 4U, HUD_BAR_WIDTH);
    set_pixel(x, HUD_BAR_Y + 1U);
    set_pixel(x, HUD_BAR_Y + 2U);
    set_pixel(x, HUD_BAR_Y + 3U);
    set_pixel(x + HUD_BAR_WIDTH - 1U, HUD_BAR_Y + 1U);
    set_pixel(x + HUD_BAR_WIDTH - 1U, HUD_BAR_Y + 2U);
    set_pixel(x + HUD_BAR_WIDTH - 1U, HUD_BAR_Y + 3U);

    for (uint8_t row = 1; row < 4; row++) {
        uint8_t start = side == 0 ? x + HUD_BAR_WIDTH - 1U - fill : x + 1U;
        draw_hline(start, HUD_BAR_Y + row, fill);
    }
}

static void draw_transport(void) {
    static const uint8_t glyph_u[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e};
    static const uint8_t glyph_b[7] = {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e};
    struct zmk_endpoint_instance endpoint = zmk_endpoint_get_selected();
    enum zmk_transport transport = endpoint.transport;
    if (transport == ZMK_TRANSPORT_NONE) {
        transport = zmk_endpoint_get_preferred_transport();
    }
    const uint8_t *glyph = transport == ZMK_TRANSPORT_BLE ? glyph_b : glyph_u;

    for (uint8_t y = 0; y < 7; y++) {
        for (uint8_t x = 0; x < 5; x++) {
            if (glyph[y] & BIT(4 - x)) {
                set_pixel(61 + x, y + 1U);
            }
        }
    }
}

static void draw_frame(const struct fight_frame *frame, bool mirror, int8_t x_offset) {
    for (uint16_t index = 0; index < frame->count; index++) {
        const struct fight_span *span = &frame->spans[index];
        if (mirror) {
            for (int x = DISPLAY_WIDTH - 1 - span->x1;
                 x <= DISPLAY_WIDTH - 1 - span->x0; x++) {
                set_pixel(x + x_offset, span->y);
            }
        } else {
            for (int x = span->x0; x <= span->x1; x++) {
                set_pixel(x + x_offset, span->y);
            }
        }
    }
}

static void advance_player(struct fight_player_state *player, enum fight_side side,
                           uint8_t desired_action) {
    uint8_t count = fight_asset_frame_count(side, (enum fight_action)player->action);
    if (player->frame + 1U < count) {
        player->frame++;
        return;
    }

    /* WPM chooses only the next action; never interrupt an action already playing. */
    player->action = desired_action;
    player->frame = 0;
}

static void render(struct zmk_widget_fight_status *widget) {
    uint32_t now = k_uptime_get_32();
    for (uint8_t side = 0; side < 2; side++) {
        widget->players[side].wpm = side_wpm(side, now);
        uint8_t target_offset = MIN(widget->players[side].wpm / 5U, FIGHTER_MAX_OFFSET);
        if (widget->players[side].center_offset < target_offset) {
            widget->players[side].center_offset =
                MIN(widget->players[side].center_offset + FIGHTER_MOVE_STEP, target_offset);
        } else if (widget->players[side].center_offset > target_offset) {
            if (widget->players[side].center_offset - target_offset <= FIGHTER_MOVE_STEP) {
                widget->players[side].center_offset = target_offset;
            } else {
                widget->players[side].center_offset -= FIGHTER_MOVE_STEP;
            }
        }
        advance_player(&widget->players[side], (enum fight_side)side,
                       action_for_wpm(widget->players[side].wpm));
    }

    memset(framebuffer + IMAGE_PALETTE_BYTES, 0, IMAGE_BYTES - IMAGE_PALETTE_BYTES);
    framebuffer[0] = framebuffer[1] = framebuffer[2] = framebuffer[3] = 0xff;
    framebuffer[4] = framebuffer[5] = framebuffer[6] = framebuffer[7] = 0;

    draw_battery_bar(0);
    draw_battery_bar(1);
    draw_transport();

    const struct fight_player_state *left = &widget->players[0];
    const struct fight_player_state *right = &widget->players[1];
    draw_frame(fight_asset_frame(FIGHT_SIDE_LEFT, (enum fight_action)left->action, left->frame),
               true, left->center_offset);
    draw_frame(fight_asset_frame(FIGHT_SIDE_RIGHT, (enum fight_action)right->action, right->frame),
               false, -(int8_t)right->center_offset);
    lv_obj_invalidate(widget->image);
}

static void timer_cb(lv_timer_t *timer) {
    render(lv_timer_get_user_data(timer));
}

int zmk_widget_fight_status_init(struct zmk_widget_fight_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);
    lv_obj_set_size(widget->obj, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    widget->image = lv_image_create(widget->obj);
    lv_image_set_src(widget->image, &fight_image);
    lv_obj_align(widget->image, LV_ALIGN_TOP_LEFT, 0, 0);

    for (uint8_t side = 0; side < 2; side++) {
        widget->players[side] = (struct fight_player_state){
            .action = FIGHT_ACTION_IDLE,
            .frame = side,
            .wpm = 0,
            .center_offset = 0,
        };
    }
    widget->timer = lv_timer_create(timer_cb, FIGHT_TICK_MS, widget);
    render(widget);
    return 0;
}

lv_obj_t *zmk_widget_fight_status_obj(struct zmk_widget_fight_status *widget) {
    return widget->obj;
}
