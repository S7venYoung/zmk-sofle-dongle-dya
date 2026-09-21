#include "codex_status.h"

#include <errno.h>
#include <stdio.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zmk/codex_metrics.h>

#if IS_ENABLED(CONFIG_SETTINGS)
#include <zephyr/settings/settings.h>
#endif

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/* This SH1106 panel's monochrome mapping is inverted by the display stack:
 * LVGL white is physically dark and LVGL black is physically lit. */
#define CODEX_OLED_BACKGROUND lv_color_white()
#define CODEX_OLED_FOREGROUND lv_color_black()

struct codex_metrics {
    uint8_t five_hour_used;
    int16_t week_used;
    uint32_t total_tokens;
    uint32_t reset_in_minutes;
    bool available;
};

static struct codex_metrics metrics;

#if IS_ENABLED(CONFIG_SETTINGS)
#define CODEX_METRICS_SETTINGS_KEY "codex_metrics/metrics"

static bool metrics_settings_ready;
static struct k_work_delayable metrics_save_work;

static int codex_metrics_handle_set(const char *name, size_t len, settings_read_cb read_cb,
                                    void *cb_arg) {
    const char *next;

    if (!settings_name_steq(name, "metrics", &next) || next) {
        return 0;
    }
    if (len != sizeof(metrics)) {
        return -EINVAL;
    }
    int err = read_cb(cb_arg, &metrics, sizeof(metrics));
    if (err <= 0) {
        return err;
    }
    return 0;
}

static int codex_metrics_handle_commit(void) {
    metrics_settings_ready = true;
    return 0;
}

static struct settings_handler codex_metrics_settings_handler = {
    .name = "codex_metrics",
    .h_set = codex_metrics_handle_set,
    .h_commit = codex_metrics_handle_commit,
};

static void codex_metrics_save_work_cb(struct k_work *work) {
    ARG_UNUSED(work);
    if (metrics_settings_ready) {
        settings_save_one(CODEX_METRICS_SETTINGS_KEY, &metrics, sizeof(metrics));
    }
}
#endif

static void set_bar(lv_obj_t *fill, uint8_t percent, uint8_t width) {
    lv_obj_set_width(fill, MAX(1, (width * MIN(percent, 100)) / 100));
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
    if (metrics.reset_in_minutes == 0) {
        snprintf(text, sizeof(text), "RESET --:--");
    } else {
        snprintf(text, sizeof(text), "RESET %02lu:%02lu",
                 (unsigned long)(metrics.reset_in_minutes / 60),
                 (unsigned long)(metrics.reset_in_minutes % 60));
    }
    lv_label_set_text(widget->reset, text);
    set_bar(widget->primary_fill, 100 - metrics.five_hour_used, 53);
    if (metrics.week_used >= 0) {
        set_bar(widget->secondary_fill, 100 - metrics.week_used, 53);
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

#if IS_ENABLED(CONFIG_SETTINGS)
    /* Coalesce repeated host updates so flash is written once per burst. */
    k_work_reschedule(&metrics_save_work, K_SECONDS(5));
#endif
}

void zmk_codex_metrics_update(uint8_t five_hour_used, int16_t week_used, uint32_t total_tokens,
                              uint32_t reset_in_minutes, uint32_t updated_at) {
    ARG_UNUSED(updated_at);
    zmk_widget_codex_status_set_metrics(five_hour_used, week_used, total_tokens,
                                        reset_in_minutes);
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, lv_align_t align, int x, int y) {
    lv_obj_t *result = lv_label_create(parent);
    lv_label_set_text(result, text);
    lv_obj_set_style_text_font(result, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_color(result, CODEX_OLED_FOREGROUND, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(result, 0, LV_PART_MAIN);
    lv_obj_align(result, align, x, y);
    return result;
}

static lv_obj_t *panel(lv_obj_t *parent, int x, int y, int width, int height) {
    lv_obj_t *result = lv_obj_create(parent);
    lv_obj_set_pos(result, x, y);
    lv_obj_set_size(result, width, height);
    lv_obj_set_style_bg_opa(result, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(result, CODEX_OLED_FOREGROUND, LV_PART_MAIN);
    lv_obj_set_style_border_width(result, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(result, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(result, 0, LV_PART_MAIN);
    return result;
}

int zmk_widget_codex_status_init(struct zmk_widget_codex_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 64);
    lv_obj_set_style_bg_color(widget->obj, CODEX_OLED_BACKGROUND, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    label(widget->obj, "CODEX", LV_ALIGN_TOP_LEFT, 3, 2);
    label(widget->obj, "USB", LV_ALIGN_TOP_RIGHT, -8, 2);
    lv_obj_t *usb_dot = lv_obj_create(widget->obj);
    lv_obj_set_size(usb_dot, 3, 3);
    lv_obj_set_style_bg_color(usb_dot, CODEX_OLED_FOREGROUND, LV_PART_MAIN);
    lv_obj_set_style_border_width(usb_dot, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(usb_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align(usb_dot, LV_ALIGN_TOP_RIGHT, -3, 4);

    lv_obj_t *primary = panel(widget->obj, 2, 12, 61, 26);
    widget->five_hour = label(primary, "5H  --%", LV_ALIGN_TOP_LEFT, 3, 3);
    lv_obj_t *primary_track = lv_obj_create(primary);
    lv_obj_set_pos(primary_track, 3, 17);
    lv_obj_set_size(primary_track, 53, 4);
    lv_obj_set_style_bg_opa(primary_track, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(primary_track, CODEX_OLED_FOREGROUND, LV_PART_MAIN);
    lv_obj_set_style_border_width(primary_track, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(primary_track, 0, LV_PART_MAIN);
    widget->primary_fill = lv_obj_create(primary_track);
    lv_obj_set_pos(widget->primary_fill, 1, 1);
    lv_obj_set_size(widget->primary_fill, 1, 2);
    lv_obj_set_style_bg_color(widget->primary_fill, CODEX_OLED_FOREGROUND, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->primary_fill, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->primary_fill, 0, LV_PART_MAIN);

    lv_obj_t *secondary = panel(widget->obj, 65, 12, 61, 26);
    widget->week = label(secondary, "WK  --%", LV_ALIGN_TOP_LEFT, 3, 3);
    lv_obj_t *secondary_track = lv_obj_create(secondary);
    lv_obj_set_pos(secondary_track, 3, 17);
    lv_obj_set_size(secondary_track, 53, 4);
    lv_obj_set_style_bg_opa(secondary_track, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(secondary_track, CODEX_OLED_FOREGROUND, LV_PART_MAIN);
    lv_obj_set_style_border_width(secondary_track, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(secondary_track, 0, LV_PART_MAIN);
    widget->secondary_fill = lv_obj_create(secondary_track);
    lv_obj_set_pos(widget->secondary_fill, 1, 1);
    lv_obj_set_size(widget->secondary_fill, 1, 2);
    lv_obj_set_style_bg_color(widget->secondary_fill, CODEX_OLED_FOREGROUND, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->secondary_fill, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->secondary_fill, 0, LV_PART_MAIN);

    lv_obj_t *footer = panel(widget->obj, 2, 41, 124, 21);
    widget->tokens = label(footer, "TODAY  --", LV_ALIGN_TOP_LEFT, 3, 2);
    widget->reset = label(footer, "WAITING FOR MAC", LV_ALIGN_TOP_LEFT, 3, 11);

    sys_slist_append(&widgets, &widget->node);
    refresh(widget);
    return 0;
}

static int codex_metrics_init(void) {
#if IS_ENABLED(CONFIG_SETTINGS)
    k_work_init_delayable(&metrics_save_work, codex_metrics_save_work_cb);
    int err = settings_register(&codex_metrics_settings_handler);
    if (err == 0) {
        metrics_settings_ready = true;
    }
#endif
    return 0;
}

SYS_INIT(codex_metrics_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

lv_obj_t *zmk_widget_codex_status_obj(struct zmk_widget_codex_status *widget) {
    return widget->obj;
}
