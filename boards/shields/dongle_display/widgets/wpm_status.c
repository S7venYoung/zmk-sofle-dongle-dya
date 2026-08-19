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
static int peak_wpm;
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
    if (current > peak_wpm) {
        peak_wpm = current;
    }
    return (struct wpm_status_state){
        .wpm = current,
        .peak = peak_wpm,
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
    if (current > peak_wpm) {
        peak_wpm = current;
    }
    set_wpm(widget, (struct wpm_status_state) {
        .wpm = current,
        .peak = peak_wpm,
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
    lv_obj_set_size(widget->obj, enabled ? 52 : LV_SIZE_CONTENT,
                    enabled ? 32 : LV_SIZE_CONTENT);
    lv_img_set_zoom(widget->speedometer, enabled ? 220 : LV_IMG_ZOOM_NONE);
    lv_obj_set_style_text_font(widget->wpm_label, enabled ? &lv_font_unscii_16 : LV_FONT_DEFAULT, 0);
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

    widget->wpm_label = lv_label_create(widget->obj);
    lv_obj_align_to(widget->wpm_label, widget->speedometer, LV_ALIGN_OUT_RIGHT_MID, 2, 1);

    sys_slist_append(&widgets, &widget->node);

    widget_wpm_status_init();
    return 0;
}

lv_obj_t *zmk_widget_wpm_status_obj(struct zmk_widget_wpm_status *widget)
{
    return widget->obj;
}
