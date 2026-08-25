/*
 * Generated from the supplied monochrome fighting animations.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

enum fight_action {
    FIGHT_ACTION_IDLE,
    FIGHT_ACTION_ATTACK,
    FIGHT_ACTION_HEAVY,
    FIGHT_ACTION_COMBO,
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

const struct fight_frame *fight_asset_frame(enum fight_action action, uint8_t frame);
