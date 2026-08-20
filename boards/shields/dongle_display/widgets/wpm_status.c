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
    /* Layer filtering belongs to the original numeric WPM widget only.  The
     * two instrument themes use WPM as live gauge input on every layer. */
    if (widget->display_mode == 0 && state.layer != NULL && disabled_layers[0] != '\0' &&
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

    if (widget->bmw_needle != NULL && widget->display_mode >= 2) {
        int level = CLAMP(value, 0, 120);
        bool right = widget->display_mode == 3;
        /* The BMW scale runs from the lower inner end, through the outer
         * elbow, to the upper inner end. The needle therefore follows the
         * same broken scale instead of behaving like a circular gauge. */
        lv_point_precise_t low = right ? (lv_point_precise_t){14, 36}
                                       : (lv_point_precise_t){28, 36};
        lv_point_precise_t middle = right ? (lv_point_precise_t){36, 21}
                                          : (lv_point_precise_t){6, 21};
        lv_point_precise_t high = right ? (lv_point_precise_t){4, 2}
                                        : (lv_point_precise_t){38, 2};
        lv_point_precise_t target;
        if (level <= 60) {
            target.x = low.x + (middle.x - low.x) * level / 60;
            target.y = low.y + (middle.y - low.y) * level / 60;
        } else {
            target.x = middle.x + (high.x - middle.x) * (level - 60) / 60;
            target.y = middle.y + (high.y - middle.y) * (level - 60) / 60;
        }
        widget->bmw_needle_points[0] = right ? (lv_point_precise_t){17, 32}
                                             : (lv_point_precise_t){25, 32};
        widget->bmw_needle_points[1] = target;
        lv_line_set_points(widget->bmw_needle, widget->bmw_needle_points, 2);
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
        widget->display_mode = 1;
        lv_obj_add_flag(widget->speedometer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(widget->gauge_arc, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < ARRAY_SIZE(widget->gauge_ticks); i++) {
            lv_obj_clear_flag(widget->gauge_ticks[i], LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_add_flag(widget->wpm_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(widget->needle, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widget->bmw_outline, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widget->bmw_needle, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < ARRAY_SIZE(widget->bmw_ticks); i++) {
            lv_obj_add_flag(widget->bmw_ticks[i], LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        widget->display_mode = 0;
        lv_obj_clear_flag(widget->speedometer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widget->gauge_arc, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < ARRAY_SIZE(widget->gauge_ticks); i++) {
            lv_obj_add_flag(widget->gauge_ticks[i], LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_clear_flag(widget->wpm_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widget->needle, LV_OBJ_FLAG_HIDDEN);
    }
    widget->last_value = -1;
    zmk_widget_wpm_status_refresh(widget);
}

void zmk_widget_wpm_status_set_bmw(struct zmk_widget_wpm_status *widget, bool enabled,
                                   bool right_side)
{
    if (!enabled) {
        lv_obj_add_flag(widget->bmw_outline, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(widget->bmw_needle, LV_OBJ_FLAG_HIDDEN);
        for (size_t i = 0; i < ARRAY_SIZE(widget->bmw_ticks); i++) {
            lv_obj_add_flag(widget->bmw_ticks[i], LV_OBJ_FLAG_HIDDEN);
        }
        /* Make the setter safe on its own as well as in the current
         * dashboard->BMW application order.  Do not overwrite dashboard
         * mode when it has already been selected. */
        if (widget->display_mode >= 2) {
            widget->display_mode = 0;
            lv_obj_set_size(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_clear_flag(widget->speedometer, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(widget->wpm_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_text_font(widget->wpm_label, LV_FONT_DEFAULT, 0);
        }
        return;
    }

    widget->display_mode = right_side ? 3 : 2;
    lv_obj_set_size(widget->obj, 42, 40);
    lv_obj_add_flag(widget->speedometer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(widget->wpm_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(widget->gauge_arc, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(widget->needle, LV_OBJ_FLAG_HIDDEN);
    for (size_t i = 0; i < ARRAY_SIZE(widget->gauge_ticks); i++) {
        lv_obj_add_flag(widget->gauge_ticks[i], LV_OBJ_FLAG_HIDDEN);
    }

    lv_point_precise_t left_outline[3] = {{38, 2}, {6, 21}, {28, 36}};
    lv_point_precise_t left_ticks[6][2] = {
        {{33, 5}, {29, 6}}, {{27, 9}, {23, 10}}, {{21, 13}, {17, 14}},
        {{15, 17}, {11, 18}}, {{10, 25}, {14, 26}}, {{17, 30}, {21, 31}},
    };
    for (size_t p = 0; p < ARRAY_SIZE(widget->bmw_outline_points); p++) {
        widget->bmw_outline_points[p].x = right_side ? 42 - left_outline[p].x
                                                      : left_outline[p].x;
        widget->bmw_outline_points[p].y = left_outline[p].y;
    }
    lv_line_set_points(widget->bmw_outline, widget->bmw_outline_points,
                       ARRAY_SIZE(widget->bmw_outline_points));
    for (size_t i = 0; i < ARRAY_SIZE(widget->bmw_ticks); i++) {
        for (size_t p = 0; p < 2; p++) {
            widget->bmw_tick_points[i][p].x = right_side ? 42 - left_ticks[i][p].x
                                                          : left_ticks[i][p].x;
            widget->bmw_tick_points[i][p].y = left_ticks[i][p].y;
        }
        lv_line_set_points(widget->bmw_ticks[i], widget->bmw_tick_points[i], 2);
        lv_obj_clear_flag(widget->bmw_ticks[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_clear_flag(widget->bmw_outline, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(widget->bmw_needle, LV_OBJ_FLAG_HIDDEN);
    widget->last_value = -1;
    zmk_widget_wpm_status_refresh(widget);
}

int zmk_widget_wpm_status_init(struct zmk_widget_wpm_status *widget, lv_obj_t *parent)
{
    widget->obj = lv_obj_create(parent);
    widget->peak_mode = false;
    widget->display_mode = 0;
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

    widget->bmw_outline = lv_line_create(widget->obj);
    lv_obj_set_style_line_width(widget->bmw_outline, 2, 0);
    lv_obj_set_style_line_rounded(widget->bmw_outline, true, 0);
    lv_obj_add_flag(widget->bmw_outline, LV_OBJ_FLAG_HIDDEN);

    for (size_t i = 0; i < ARRAY_SIZE(widget->bmw_ticks); i++) {
        widget->bmw_ticks[i] = lv_line_create(widget->obj);
        lv_obj_set_style_line_width(widget->bmw_ticks[i], 1, 0);
        lv_obj_set_style_line_rounded(widget->bmw_ticks[i], true, 0);
        lv_obj_add_flag(widget->bmw_ticks[i], LV_OBJ_FLAG_HIDDEN);
    }

    widget->bmw_needle = lv_line_create(widget->obj);
    lv_obj_set_style_line_width(widget->bmw_needle, 2, 0);
    lv_obj_set_style_line_rounded(widget->bmw_needle, true, 0);
    lv_obj_add_flag(widget->bmw_needle, LV_OBJ_FLAG_HIDDEN);

    sys_slist_append(&widgets, &widget->node);

    widget_wpm_status_init();
    return 0;
}

lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget)
{
    return widget->obj;
}
