#include "mywatchface.h"
#include <pebble.h>

static Window *s_window;
static TextLayer *s_text_layer;
static Layer *s_hands_layer;
static GFont s_time_font;
static GPath *s_minute_arrow, *s_hour_arrow;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void hands_update_proc(Layer *layer, GContext *ctx);

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_text_layer, s_buffer);
}


static void prv_window_load(Window *window) {

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_hands_layer = layer_create(GRect(0,0,144,144));
    layer_set_update_proc(s_hands_layer, hands_update_proc);
    layer_add_child(window_layer, s_hands_layer);

    // Redraw this as soon as possible
    layer_mark_dirty(s_hands_layer);


    s_text_layer = text_layer_create(GRect(0, 144, bounds.size.w, bounds.size.h));
    text_layer_set_text_alignment(s_text_layer, GTextAlignmentLeft);
    layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_22));

    // Apply to TextLayer
    text_layer_set_font(s_text_layer, s_time_font);

}

static void prv_window_unload(Window *window) {
    text_layer_destroy(s_text_layer);
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
  update_time();
  layer_mark_dirty(s_hands_layer);
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
    gpath_rotate_to(s_hour_arrow, TRIG_MAX_ANGLE * t->tm_hour / 60);


    gpath_draw_filled(ctx, s_minute_arrow);
    gpath_draw_outline(ctx, s_minute_arrow);

    gpath_draw_filled(ctx, s_hour_arrow);
    gpath_draw_outline(ctx, s_hour_arrow);
}

int main(void) {
    prv_init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

    app_event_loop();
    prv_deinit();
}
