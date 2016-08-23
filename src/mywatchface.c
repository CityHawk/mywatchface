#include "mywatchface.h"
#include <pebble.h>

static Window *s_window;
static Layer *s_ticks_layer;
static Layer *s_hands_layer;
static Layer *s_digital_layer;
static Layer *s_date_layer;
static GFont s_time_font;
static GFont s_label_font;
static GPath *s_minute_arrow, *s_hour_arrow;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void hands_update_proc(Layer *layer, GContext *ctx);
static void ticks_update_proc(Layer *layer, GContext *ctx);
static void digital_update_proc(Layer *layer, GContext *ctx);
static void date_update_proc(Layer *layer, GContext *ctx);

static void prv_window_load(Window *window) {

    window_set_background_color(window, GColorBlack);
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_20));
    s_label_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_14));

    s_ticks_layer = layer_create(GRect(0, 0, 144, 144));
    layer_set_update_proc(s_ticks_layer, ticks_update_proc);
    layer_add_child(window_layer, s_ticks_layer);

    s_hands_layer = layer_create(GRect(12,12,120,120));
    layer_set_update_proc(s_hands_layer, hands_update_proc);
    layer_add_child(window_layer, s_hands_layer);


    s_digital_layer = layer_create(GRect(0, 146, 48, 20));
    layer_set_update_proc(s_digital_layer, digital_update_proc);
    layer_add_child(window_layer, s_digital_layer);

    s_date_layer = layer_create(GRect(48, 146, 96, 20));
    layer_set_update_proc(s_date_layer, date_update_proc);
    layer_add_child(window_layer, s_date_layer);

    // Redraw this as soon as possible
    layer_mark_dirty(s_ticks_layer);
    layer_mark_dirty(s_hands_layer);
    layer_mark_dirty(s_digital_layer);
    layer_mark_dirty(s_date_layer);
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
    layer_mark_dirty(s_ticks_layer);
    layer_mark_dirty(s_hands_layer);
    layer_mark_dirty(s_digital_layer);
    layer_mark_dirty(s_date_layer);
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
    s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
    s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);
    GRect bounds = layer_get_bounds(layer);
    GPoint center = GPoint(bounds.size.w/2,bounds.size.h/2);
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

    graphics_draw_circle(ctx, center, 1);
}

static void digital_update_proc(Layer *layer, GContext *ctx) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char s_buffer[6];
    strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?  "%H:%M" : "%I:%M", t);

    /* GTextAttributes *text_attrs = graphics_text_attributes_create(); */
    graphics_draw_text(ctx, s_buffer, s_time_font,
            layer_get_bounds(layer), GTextOverflowModeFill, GTextAlignmentLeft, NULL);

}

static void date_update_proc(Layer *layer, GContext *ctx) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char s_buffer[10];
    strftime(s_buffer, sizeof(s_buffer), "%a %m/%d", t);

    /* GTextAttributes *text_attrs = graphics_text_attributes_create(); */
    graphics_draw_text(ctx, s_buffer, s_time_font,
            layer_get_bounds(layer), GTextOverflowModeFill, GTextAlignmentRight, NULL);

}

static void ticks_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    GPoint center = GPoint(bounds.size.w/2,bounds.size.h/2);
    graphics_context_set_fill_color(ctx, GColorLightGray);
    graphics_context_set_stroke_color(ctx, GColorLightGray);

    int16_t ray_length = 111;

    for (int i = 0; i < 59; ++i) {
        GPoint ray_endpoint = {
            .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * i / 60) * (int32_t)ray_length / TRIG_MAX_RATIO) + center.x,
            .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * i / 60) * (int32_t)ray_length / TRIG_MAX_RATIO) + center.y,
        };
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "X %d, Y %d", ray_endpoint.x, ray_endpoint.y);

        graphics_draw_line(ctx, ray_endpoint, center);
    }

    graphics_context_set_fill_color(ctx, GColorOrange);
    graphics_context_set_stroke_color(ctx, GColorOrange);

    int ray_angles[12] = { 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55};
    for (int i = 0; i < 12; ++i) {
        GPoint ray_endpoint = {
            .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * ray_angles[i] / 60) * (int32_t)ray_length / TRIG_MAX_RATIO) + center.x,
            .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * ray_angles[i] / 60) * (int32_t)ray_length / TRIG_MAX_RATIO) + center.y,
        };
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "X %d, Y %d", ray_endpoint.x, ray_endpoint.y);

        /* GPathInfo bold_tick_points = {4, (GPoint[]) {
           {center.x+1, center.y+1},
           {center.x-1, center.y-1},
           {ray_endpoint.x-1, ray_endpoint.y-1},
           {ray_endpoint.x+1, ray_endpoint.y+1}
           }
           };
           GPath bold_tick = gpath_create({4, (GPoint[]) { {center.x+1, center.y+1}, {center.x-1, center.y-1}, {ray_endpoint.x-1, ray_endpoint.y-1}, {ray_endpoint.x+1, ray_endpoint.y+1} } });
           gpath_draw_filled(ctx, bold_tick); */
        graphics_draw_line(ctx, ray_endpoint, center);
    }


    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(6, 6, bounds.size.w - 12, bounds.size.h - 12), 5, GCornersAll);

    //numbers
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    char s_buffer[3];
    for (int i = 1; i <= 12; ++i) {
        GRect label_rect = grect_centered_from_polar(GRect(14,14, bounds.size.w - 28, bounds.size.h - 28),GOvalScaleModeFitCircle, i * (TRIG_MAX_ANGLE / 12), GSize(12,12));
        //graphics_fill_rect(ctx, label_rect, 0, GCornersAll);
        snprintf(s_buffer, 3, "%d", i);
        graphics_draw_text(ctx, s_buffer, s_label_font, label_rect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }

}

int main(void) {
    prv_init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

    app_event_loop();
    prv_deinit();
}
