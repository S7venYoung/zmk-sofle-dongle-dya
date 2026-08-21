/*
 * Copyright (c) 2026 S7venYoung
 * SPDX-License-Identifier: MIT
 */

#include "bmw_grille.h"

static void configure_outline(lv_obj_t *line, lv_point_precise_t *points, size_t count) {
    lv_line_set_points(line, points, count);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
}

int zmk_widget_bmw_grille_init(struct zmk_widget_bmw_grille *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);
    lv_obj_set_size(widget->obj, 128, 64);

    /* Two inward-facing kidney outlines leave a narrow centre seam for the
     * active-layer letter while retaining the tall brace form of the sketch. */
    const lv_point_precise_t left[5] = {
        {62, 10}, {52, 12}, {48, 20}, {49, 36}, {62, 41},
    };
    for (size_t i = 0; i < ARRAY_SIZE(left); i++) {
        widget->left_points[i] = left[i];
        widget->right_points[i].x = 128 - left[i].x;
        widget->right_points[i].y = left[i].y;
    }

    widget->left_outline = lv_line_create(widget->obj);
    configure_outline(widget->left_outline, widget->left_points,
                      ARRAY_SIZE(widget->left_points));

    widget->right_outline = lv_line_create(widget->obj);
    configure_outline(widget->right_outline, widget->right_points,
                      ARRAY_SIZE(widget->right_points));

    return 0;
}

lv_obj_t *zmk_widget_bmw_grille_obj(struct zmk_widget_bmw_grille *widget) {
    return widget->obj;
}
