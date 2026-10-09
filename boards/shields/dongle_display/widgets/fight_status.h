/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

struct fight_player_state {
    uint8_t action;
    uint8_t frame;
    uint8_t wpm;
    uint8_t center_offset;
};

struct zmk_widget_fight_status {
    lv_obj_t *obj;
    lv_obj_t *image;
    lv_timer_t *timer;
    struct fight_player_state players[2];
};

int zmk_widget_fight_status_init(struct zmk_widget_fight_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_fight_status_obj(struct zmk_widget_fight_status *widget);
