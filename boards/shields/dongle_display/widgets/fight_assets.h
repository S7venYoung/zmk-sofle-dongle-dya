/*
 * Generated from the supplied monochrome fighting animations.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

enum fight_action {
    FIGHT_ACTION_IDLE,
    FIGHT_ACTION_SLOW,
    FIGHT_ACTION_MID,
    FIGHT_ACTION_FAST,
    FIGHT_ACTION_COUNT,
};

enum fight_side {
    FIGHT_SIDE_LEFT,
    FIGHT_SIDE_RIGHT,
};

struct fight_span {
    uint8_t y;
    uint8_t x0;
    uint8_t x1;
};

struct fight_frame {
    const struct fight_span *spans;
    uint16_t count;
};

const struct fight_frame *fight_asset_frame(enum fight_side side, enum fight_action action,
                                            uint8_t frame);
uint8_t fight_asset_frame_count(enum fight_side side, enum fight_action action);
