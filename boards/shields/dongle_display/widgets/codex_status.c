#include "codex_status.h"

#include <stdio.h>

#include <zephyr/sys/util.h>

#include <zmk/codex_metrics.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct codex_metrics {
    uint8_t five_hour_used;
    int16_t week_used;
    uint32_t total_tokens;
    uint32_t reset_in_minutes;
    bool available;
};

static struct codex_metrics metrics;

static void set_bar(lv_obj_t *fill, uint8_t used, uint8_t width) {
    uint8_t remaining = 100 - MIN(used, 100);
    lv_obj_set_width(fill, MAX(1, (width * remaining) / 100));
}

static void refresh(struct zmk_widget_codex_status *widget) {
    if (!metrics.available) {
        lv_label_set_text(widget->five_hour, "5H  --%");
        lv_label_set_text(widget->week, "WK  --%");
        lv_label_set_text(widget->tokens, "TODAY  --");
        lv_label_set_text(widget->reset, "WAITING FOR MAC");
        lv_obj_set_width(widget->primary_fill, 1);
        lv_obj_set_width(widget->secondary_fill, 1);
        return;
    }

    char text[24];
    snprintf(text, sizeof(text), "5H  %02u%%", 100 - MIN(metrics.five_hour_used, 100));
    lv_label_set_text(widget->five_hour, text);
    if (metrics.week_used >= 0) {
        snprintf(text, sizeof(text), "WK  %02u%%", 100 - MIN(metrics.week_used, 100));
        lv_label_set_text(widget->week, text);
    } else {
        lv_label_set_text(widget->week, "WK  --%");
    }
    snprintf(text, sizeof(text), "TODAY %luM", (unsigned long)(metrics.total_tokens / 1000000));
    lv_label_set_text(widget->tokens, text);
    snprintf(text, sizeof(text), "RESET %02lu:%02lu", (unsigned long)(metrics.reset_in_minutes / 60),
             (unsigned long)(metrics.reset_in_minutes % 60));
    lv_label_set_text(widget->reset, text);
    set_bar(widget->primary_fill, metrics.five_hour_used, 53);
    if (metrics.week_used >= 0) {
        set_bar(widget->secondary_fill, metrics.week_used, 53);
    } else {
        lv_obj_set_width(widget->secondary_fill, 1);
    }
}

void zmk_widget_codex_status_set_metrics(uint8_t five_hour_used, int16_t week_used,
                                         uint32_t total_tokens, uint32_t reset_in_minutes) {
    metrics = (struct codex_metrics){
        .five_hour_used = MIN(five_hour_used, 100),
        .week_used = week_used < 0 ? -1 : MIN(week_used, 100),
        .total_tokens = total_tokens,
        .reset_in_minutes = reset_in_minutes,
        .available = true,
    };

    struct zmk_widget_codex_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { refresh(widget); }
}

void zmk_codex_metrics_update(uint8_t five_hour_used, uint32_t total_tokens, uint32_t updated_at) {
    ARG_UNUSED(updated_at);
    /* The first companion protocol only sends five-hour usage and token count.
     * Keep weekly/reset unavailable until those fields are added to the host. */
    zmk_widget_codex_status_set_metrics(five_hour_used, -1, total_tokens, 0);
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, lv_align_t align, int x, int y) {
    lv_obj_t *result = lv_label_create(parent);
    lv_label_set_text(result, text);
    lv_obj_set_style_text_font(result, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_color(result, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(result, 0, LV_PART_MAIN);
    lv_obj_align(result, align, x, y);
    return result;
}

static lv_obj_t *panel(lv_obj_t *parent, int x, int y, int width, int height) {
    lv_obj_t *result = lv_obj_create(parent);
    lv_obj_set_pos(result, x, y);
    lv_obj_set_size(result, width, height);
    lv_obj_set_style_bg_opa(result, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(result, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(result, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(result, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(result, 0, LV_PART_MAIN);
    return result;
}

int zmk_widget_codex_status_init(struct zmk_widget_codex_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 64);
    lv_obj_set_style_bg_color(widget->obj, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    /* Keep the header inside 128 pixels: the full double-slash version
     * overlaps the USB label on the physical SH1106 panel. */
    label(widget->obj, "CODEX/SOFLE", LV_ALIGN_TOP_LEFT, 3, 2);
    label(widget->obj, "USB", LV_ALIGN_TOP_RIGHT, -8, 2);
    lv_obj_t *usb_dot = lv_obj_create(widget->obj);
    lv_obj_set_size(usb_dot, 3, 3);
    lv_obj_set_style_bg_color(usb_dot, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(usb_dot, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(usb_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align(usb_dot, LV_ALIGN_TOP_RIGHT, -3, 4);

    lv_obj_t *primary = panel(widget->obj, 2, 12, 61, 26);
    widget->five_hour = label(primary, "5H  --%", LV_ALIGN_TOP_LEFT, 3, 3);
    lv_obj_t *primary_track = lv_obj_create(primary);
    lv_obj_set_pos(primary_track, 3, 17);
    lv_obj_set_size(primary_track, 53, 4);
    lv_obj_set_style_bg_opa(primary_track, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(primary_track, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(primary_track, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(primary_track, 0, LV_PART_MAIN);
    widget->primary_fill = lv_obj_create(primary_track);
    lv_obj_set_pos(widget->primary_fill, 1, 1);
    lv_obj_set_size(widget->primary_fill, 1, 2);
    lv_obj_set_style_bg_color(widget->primary_fill, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->primary_fill, 0, LV_PART_MAIN);

    lv_obj_t *secondary = panel(widget->obj, 65, 12, 61, 26);
    widget->week = label(secondary, "WK  --%", LV_ALIGN_TOP_LEFT, 3, 3);
    lv_obj_t *secondary_track = lv_obj_create(secondary);
    lv_obj_set_pos(secondary_track, 3, 17);
    lv_obj_set_size(secondary_track, 53, 4);
    lv_obj_set_style_bg_opa(secondary_track, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(secondary_track, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(secondary_track, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(secondary_track, 0, LV_PART_MAIN);
    widget->secondary_fill = lv_obj_create(secondary_track);
    lv_obj_set_pos(widget->secondary_fill, 1, 1);
    lv_obj_set_size(widget->secondary_fill, 1, 2);
    lv_obj_set_style_bg_color(widget->secondary_fill, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->secondary_fill, 0, LV_PART_MAIN);

    lv_obj_t *footer = panel(widget->obj, 2, 42, 124, 19);
    widget->tokens = label(footer, "TODAY  --", LV_ALIGN_TOP_LEFT, 3, 2);
    widget->reset = label(footer, "WAITING FOR MAC", LV_ALIGN_BOTTOM_LEFT, 3, -2);

    sys_slist_append(&widgets, &widget->node);
    refresh(widget);
    return 0;
}

lv_obj_t *zmk_widget_codex_status_obj(struct zmk_widget_codex_status *widget) {
    return widget->obj;
}
