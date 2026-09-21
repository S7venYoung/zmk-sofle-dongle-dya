#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <lvgl.h>
#include <zephyr/sys/slist.h>

/*
 * The host companion owns the source of truth for Codex usage. This widget is
 * deliberately transport-agnostic so a Studio RPC handler can publish new
 * samples without coupling the display to USB framing.
 */
struct zmk_widget_codex_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *five_hour;
    lv_obj_t *week;
    lv_obj_t *tokens;
    lv_obj_t *reset;
    lv_obj_t *primary_fill;
    lv_obj_t *secondary_fill;
};

int zmk_widget_codex_status_init(struct zmk_widget_codex_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_codex_status_obj(struct zmk_widget_codex_status *widget);
void zmk_widget_codex_status_set_metrics(uint8_t five_hour_used, int16_t week_used,
                                         uint32_t total_tokens, uint32_t reset_in_minutes);
