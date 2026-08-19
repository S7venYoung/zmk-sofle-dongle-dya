#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/wpm.h>

#include "wpm_status.h"

#if IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS)
#include <zmk/display_settings.h>
#endif

LV_IMG_DECLARE(sym_speedometer);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

#define PEAK_WPM_HOLD_MS 3000

static int peak_wpm;
static int64_t peak_wpm_updated_at;

/* Seven inward-facing ticks follow a true 40 px circular arc. */
static const lv_point_precise_t dashboard_tick_points[7][2] = {
    {{2, 32}, {7, 32}},
    {{6, 23}, {10, 26}},
    {{13, 18}, {15, 23}},
    {{21, 16}, {21, 21}},
    {{29, 18}, {27, 23}},
    {{36, 23}, {32, 26}},
    {{40, 32}, {35, 32}},
};

static int update_peak_wpm(int current)
{
    int64_t now = k_uptime_get();

    if (current >= peak_wpm) {
        peak_wpm = current;
        peak_wpm_updated_at = now;
    } else if (now - peak_wpm_updated_at >= PEAK_WPM_HOLD_MS) {
        peak_wpm = current;
        peak_wpm_updated_at = now;
    }

    return peak_wpm;
}
struct wpm_status_state
{
    int wpm;
    int peak;
    const char *layer;
};

static struct wpm_status_state get_state(const zmk_event_t *_eh)
{
    const struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(_eh);
    uint8_t index = zmk_keymap_highest_layer_active();

    int current = ev ? ev->state : zmk_wpm_get_state();
    int peak = update_peak_wpm(current);
    return (struct wpm_status_state){
        .wpm = current,
        .peak = peak,
        .layer = zmk_keymap_layer_name(index)
    };
}

static void set_wpm(struct zmk_widget_wpm_status *widget, struct wpm_status_state state)
{
    int value = widget->peak_mode ? state.peak : state.wpm;
    if (value == widget->last_value) {
        return;
    }
    widget->last_value = value;

    const char *disabled_layers = CONFIG_ZMK_DONGLE_DISPLAY_WPM_DISABLED_LAYERS;
#if IS_ENABLED(CONFIG_ZMK_CUSTOM_SETTINGS)
    disabled_layers = zmk_display_settings_wpm_disabled_layers();
#endif
    if (state.layer != NULL && disabled_layers[0] != '\0' &&
        strstr(disabled_layers, state.layer) != NULL) {
        lv_label_set_text(widget->wpm_label, "-");
        return;
    }

    char wpm_text[12];
    snprintf(wpm_text, sizeof(wpm_text), widget->peak_mode ? "M%i" : "%i", value);
    lv_label_set_text(widget->wpm_label, wpm_text);

    if (widget->needle != NULL) {
        int level = CLAMP(value, 0, 120);
        static const int x[] = {2, 7, 21, 35, 40};
        static const int y[] = {36, 22, 16, 22, 36};
        int segment = MIN(level / 30, 3);
        int rem = level % 30;
        widget->needle_points[1].x = x[segment] + (x[segment + 1] - x[segment]) * rem / 30;
        widget->needle_points[1].y = y[segment] + (y[segment + 1] - y[segment]) * rem / 30;
        lv_line_set_points(widget->needle, widget->needle_points, 2);
    }
}

static void wpm_status_update_cb(struct wpm_status_state state)
{
    struct zmk_widget_wpm_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node)
    {
        set_wpm(widget, state);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state,
                            wpm_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

void zmk_widget_wpm_status_refresh(struct zmk_widget_wpm_status *widget)
{
    widget->last_value = -1;
    int current = zmk_wpm_get_state();
    int peak = update_peak_wpm(current);
    set_wpm(widget, (struct wpm_status_state) {
        .wpm = current,
        .peak = peak,
        .layer = zmk_keymap_layer_name(zmk_keymap_highest_layer_active()),
    });
}

void zmk_widget_wpm_status_set_peak(struct zmk_widget_wpm_status *widget, bool peak)
{
    widget->peak_mode = peak;
    widget->last_value = -1;
    zmk_widget_wpm_status_refresh(widget);
}

void zmk_widget_wpm_status_set_dashboard(struct zmk_widget_wpm_status *widget, bool enabled)
{
    /* Keep the native dashboard arc and its needle in one coordinate system. */
    lv_obj_set_size(widget->obj, enabled ? 44 : LV_SIZE_CONTENT,
                    enabled ? 42 : LV_SIZE_CONTENT);
    lv_img_set_zoom(widget->speedometer, LV_ZOOM_NONE);
    lv_obj_set_style_text_font(widget->wpm_label, enabled ? &lv_font_unscii_16 : LV_FONT_DEFAULT, 0);
    if (enabled) {
        lv_obj_add_flag(widget->speedometer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(widget->gauge_arc, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < ARRAY_SIZE(widget->gauge_ticks); i++) {
            lv_obj_clear_flag(widget->gauge_ticks[i], LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_add_flag(widget->wpm_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(widget->needle, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(widget->speedometer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widget->gauge_arc, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < ARRAY_SIZE(widget->gauge_ticks); i++) {
            lv_obj_add_flag(widget->gauge_ticks[i], LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_clear_flag(widget->wpm_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widget->needle, LV_OBJ_FLAG_HIDDEN);
    }
}

int zmk_widget_wpm_status_init(struct zmk_widget_wpm_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    widget->peak_mode = false;
    widget->last_value = -1;
    lv_obj_set_size(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    widget->speedometer = lv_img_create(widget->obj);
    lv_obj_align(widget->speedometer, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_img_set_src(widget->speedometer, &sym_speedometer);

    widget->gauge_arc = lv_arc_create(widget->obj);
    lv_obj_set_size(widget->gauge_arc, 40, 40);
    lv_obj_set_pos(widget->gauge_arc, 1, 16);
    lv_arc_set_bg_angles(widget->gauge_arc, 180, 360);
    lv_obj_set_style_arc_width(widget->gauge_arc, 1, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(widget->gauge_arc, false, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(widget->gauge_arc, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_remove_style(widget->gauge_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(widget->gauge_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(widget->gauge_arc, LV_OBJ_FLAG_HIDDEN);

    for (size_t i = 0; i < ARRAY_SIZE(widget->gauge_ticks); i++) {
        widget->gauge_ticks[i] = lv_line_create(widget->obj);
        lv_line_set_points(widget->gauge_ticks[i], dashboard_tick_points[i], 2);
        lv_obj_set_style_line_width(widget->gauge_ticks[i], 1, 0);
        lv_obj_set_style_line_rounded(widget->gauge_ticks[i], true, 0);
        lv_obj_add_flag(widget->gauge_ticks[i], LV_OBJ_FLAG_HIDDEN);
    }

    widget->wpm_label = lv_label_create(widget->obj);
    lv_obj_align_to(widget->wpm_label, widget->speedometer, LV_ALIGN_OUT_RIGHT_MID, 2, 1);

    widget->needle = lv_line_create(widget->obj);
    widget->needle_points[0] = (lv_point_precise_t){21, 36};
    widget->needle_points[1] = (lv_point_precise_t){21, 7};
    lv_line_set_points(widget->needle, widget->needle_points, 2);
    lv_obj_set_style_line_width(widget->needle, 1, 0);
    lv_obj_add_flag(widget->needle, LV_OBJ_FLAG_HIDDEN);

    sys_slist_append(&widgets, &widget->node);

    widget_wpm_status_init();
    return 0;
}

lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget)
{
    return widget->obj;
}
