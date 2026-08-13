/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <cormoran/zmk/custom_settings.h>
#include <zmk/display_settings.h>

/*
 * Custom Settings serializes a setting by looking up this identifier in the
 * registered DYA RPC subsystem table. Reuse the Custom Settings subsystem;
 * "s7ven_display" was only a setting group name and had no RPC registration.
 */
#define DISPLAY_SETTINGS_SUBSYSTEM "cormoran_custom_settings"

ZMK_CUSTOM_SETTING_DEFINE(
    display_key_stats_enabled, DISPLAY_SETTINGS_SUBSYSTEM, "key_stats_enabled",
    ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL, ZMK_CUSTOM_SETTING_VALUE_BOOL(true),
    ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
    ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
    ZMK_CUSTOM_SETTING_PERMISSION_SECURE,
    ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

ZMK_CUSTOM_SETTING_DEFINE(
    display_key_stats_x, DISPLAY_SETTINGS_SUBSYSTEM, "key_stats_x",
    ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32, ZMK_CUSTOM_SETTING_VALUE_INT32(42),
    ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
    ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
    ZMK_CUSTOM_SETTING_PERMISSION_SECURE,
    ZMK_CUSTOM_SETTING_RANGE_INT32(0, 78));

ZMK_CUSTOM_SETTING_DEFINE(
    display_key_stats_y, DISPLAY_SETTINGS_SUBSYSTEM, "key_stats_y",
    ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32, ZMK_CUSTOM_SETTING_VALUE_INT32(0),
    ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
    ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
    ZMK_CUSTOM_SETTING_PERMISSION_SECURE,
    ZMK_CUSTOM_SETTING_RANGE_INT32(0, 46));

ZMK_CUSTOM_SETTING_DEFINE(
    display_layer_alignment, DISPLAY_SETTINGS_SUBSYSTEM, "layer_alignment",
    ZMK_CUSTOM_SETTING_VALUE_TYPE_INT32, ZMK_CUSTOM_SETTING_VALUE_INT32(1),
    ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
    ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE,
    ZMK_CUSTOM_SETTING_PERMISSION_SECURE,
    ZMK_CUSTOM_SETTING_RANGE_INT32(0, 2));

bool zmk_display_settings_key_stats_enabled(void) {
    bool value = true;
    zmk_custom_setting_get_bool(&display_key_stats_enabled, &value);
    return value;
}

int32_t zmk_display_settings_key_stats_x(void) {
    int32_t value = 42;
    zmk_custom_setting_get_int32(&display_key_stats_x, &value);
    return value;
}

int32_t zmk_display_settings_key_stats_y(void) {
    int32_t value = 0;
    zmk_custom_setting_get_int32(&display_key_stats_y, &value);
    return value;
}

int32_t zmk_display_settings_layer_alignment(void) {
    int32_t value = 1;
    zmk_custom_setting_get_int32(&display_layer_alignment, &value);
    return value;
}
