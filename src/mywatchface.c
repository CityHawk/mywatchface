#include "mywatchface.h"
#include <pebble.h>

static Window *s_window;
static Layer *s_hands_layer;
static Layer *s_digital_layer;
static GFont s_time_font;
static GPath *s_minute_arrow, *s_hour_arrow;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void hands_update_proc(Layer *layer, GContext *ctx);
static void digital_update_proc(Layer *layer, GContext *ctx);

static void prv_window_load(Window *window) {

    window_set_background_color(window, GColorBlack);
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_18));

    s_hands_layer = layer_create(GRect(0,0,144,144));
    layer_set_update_proc(s_hands_layer, hands_update_proc);
    layer_add_child(window_layer, s_hands_layer);


    s_digital_layer = layer_create(GRect(0, 144, bounds.size.w, bounds.size.h));
    layer_set_update_proc(s_digital_layer, digital_update_proc);
    layer_add_child(window_layer, s_digital_layer);

    // Redraw this as soon as possible
    layer_mark_dirty(s_hands_layer);
    layer_mark_dirty(s_digital_layer);
}

static void prv_window_unload(Window *window) {
    layer_destroy(s_hands_layer);
    layer_destroy(s_digital_layer);
    // Unload GFont
    fonts_unload_custom_font(s_time_font);
}


static void prv_init(void) {
    // Register with TickTimerService
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
            .load = prv_window_load,
            .unload = prv_window_unload,
            });
    const bool animated = true;
    window_stack_push(s_window, animated);


}

static void prv_deinit(void) {
    window_destroy(s_window);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    layer_mark_dirty(s_hands_layer);
    layer_mark_dirty(s_digital_layer);
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
    s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
    s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);
    GPoint center = GPoint(72,72);
    gpath_move_to(s_minute_arrow, center);
    gpath_move_to(s_hour_arrow, center);
  
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
    gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));


    graphics_context_set_fill_color(ctx, GColorRed);
    gpath_draw_filled(ctx, s_hour_arrow);
    gpath_draw_outline(ctx, s_hour_arrow);

    graphics_context_set_fill_color(ctx, GColorWhite);
    gpath_draw_filled(ctx, s_minute_arrow);
    gpath_draw_outline(ctx, s_minute_arrow);

    graphics_draw_circle(ctx, center, 2);
}

static void digital_update_proc(Layer *layer, GContext *ctx) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char s_buffer[20];
    strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?  "%H:%M %a %m/%d" : "%I:%M %a %m/%d", t);

    /* GTextAttributes *text_attrs = graphics_text_attributes_create(); */
    graphics_draw_text(ctx, s_buffer, s_time_font, layer_get_bounds(layer), GTextOverflowModeFill, GTextAlignmentCenter, NULL);

}

int main(void) {
    prv_init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

    app_event_loop();
    prv_deinit();
}
