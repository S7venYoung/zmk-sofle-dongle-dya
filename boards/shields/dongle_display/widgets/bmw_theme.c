/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>

#include "bmw_theme.h"

#define BMW_THEME_WIDTH 128
#define BMW_THEME_HEIGHT 64
#define BMW_THEME_BUFFER_SIZE                                                                    \
    LV_CANVAS_BUF_SIZE(BMW_THEME_WIDTH, BMW_THEME_HEIGHT,                                        \
                       LV_COLOR_FORMAT_GET_BPP(LV_COLOR_FORMAT_L8),                               \
                       LV_DRAW_BUF_STRIDE_ALIGN)

static lv_color_t bmw_theme_buffer[BMW_THEME_BUFFER_SIZE];

static void draw_line(lv_layer_t *layer, int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                      uint8_t width) {
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.color = lv_color_black();
    line.width = width;
    line.round_start = 1;
    line.round_end = 1;
    line.p1.x = x1;
    line.p1.y = y1;
    line.p2.x = x2;
    line.p2.y = y2;
    lv_draw_line(layer, &line);
}

static void draw_bmw_braces(lv_obj_t *canvas) {
    lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);

    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);

    /* Left angular brace from the sketch. */
    draw_line(&layer, 30, 4, 20, 14, 2);
    draw_line(&layer, 20, 14, 29, 21, 2);
    draw_line(&layer, 29, 21, 18, 31, 2);
    draw_line(&layer, 18, 31, 29, 39, 2);
    draw_line(&layer, 29, 39, 20, 49, 2);

    /* Mirrored right angular brace. */
    draw_line(&layer, 98, 4, 108, 14, 2);
    draw_line(&layer, 108, 14, 99, 21, 2);
    draw_line(&layer, 99, 21, 110, 31, 2);
    draw_line(&layer, 110, 31, 99, 39, 2);
    draw_line(&layer, 99, 39, 108, 49, 2);

    /* BMW kidney pair, expressed as two facing large braces. */
    draw_line(&layer, 57, 13, 48, 15, 2);
    draw_line(&layer, 48, 15, 47, 43, 2);
    draw_line(&layer, 47, 43, 57, 45, 2);
    draw_line(&layer, 57, 13, 63, 21, 2);
    draw_line(&layer, 63, 21, 63, 37, 2);
    draw_line(&layer, 63, 37, 57, 45, 2);

    draw_line(&layer, 71, 13, 80, 15, 2);
    draw_line(&layer, 80, 15, 81, 43, 2);
    draw_line(&layer, 81, 43, 71, 45, 2);
    draw_line(&layer, 71, 13, 65, 21, 2);
    draw_line(&layer, 65, 21, 65, 37, 2);
    draw_line(&layer, 65, 37, 71, 45, 2);

    /* Lower bars from the draft leave the centre clear for output status. */
    draw_line(&layer, 2, 57, 45, 57, 3);
    draw_line(&layer, 83, 57, 126, 57, 3);

    lv_canvas_finish_layer(canvas, &layer);
}

int zmk_widget_bmw_theme_init(struct zmk_widget_bmw_theme *widget, lv_obj_t *parent) {
    widget->obj = lv_canvas_create(parent);
    lv_canvas_set_buffer(widget->obj, bmw_theme_buffer, BMW_THEME_WIDTH, BMW_THEME_HEIGHT,
                         LV_COLOR_FORMAT_L8);
    draw_bmw_braces(widget->obj);
    return 0;
}

lv_obj_t *zmk_widget_bmw_theme_obj(struct zmk_widget_bmw_theme *widget) {
    return widget->obj;
}
