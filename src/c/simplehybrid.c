#include "simplehybrid.h"
#include <pebble.h>

static Window *s_window;
static Layer *s_ticks_layer;
static Layer *s_hands_layer;
static Layer *s_digital_layer;
static Layer *s_date_layer;
static BitmapLayer *s_bt_icon_layer;
static GFont s_time_font;
static GFont s_label_font;
static GPath *s_minute_arrow, *s_hour_arrow;
static GBitmap *s_bt_icon_bitmap;


static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void hands_update_proc(Layer *layer, GContext *ctx);
static void ticks_update_proc(Layer *layer, GContext *ctx);
static void digital_update_proc(Layer *layer, GContext *ctx);
static void date_update_proc(Layer *layer, GContext *ctx);
static void bluetooth_callback(bool connected);

static void prv_window_load(Window *window) {

    window_set_background_color(window, GColorBlack);
    Layer *window_layer = window_get_root_layer(window);
    
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_20));
    //s_time_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    s_label_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_16));
    //s_label_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

    s_ticks_layer = layer_create(GRect(1, 1, 144, 144));
    layer_set_update_proc(s_ticks_layer, ticks_update_proc);
    layer_add_child(window_layer, s_ticks_layer);
    
    // Create the Bluetooth icon GBitmap
    s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_ICON);

    // Create the BitmapLayer to display the GBitmap
    s_bt_icon_layer = bitmap_layer_create(GRect(59, 24, 30, 30));
    bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
    
    // Register for Bluetooth connection updates
    connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = bluetooth_callback
    });



    s_hands_layer = layer_create(GRect(1,1,144,144));
    layer_set_update_proc(s_hands_layer, hands_update_proc);
    layer_add_child(window_layer, s_hands_layer);


    s_digital_layer = layer_create(GRect(1, 146, 48, 20));
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
    // Show the correct state of the BT connection from the start
    bluetooth_callback(connection_service_peek_pebble_app_connection());

}

static void prv_window_unload(Window *window) {
    layer_destroy(s_hands_layer);
    layer_destroy(s_digital_layer);
    layer_destroy(s_ticks_layer);
    layer_destroy(s_date_layer);
    bitmap_layer_destroy(s_bt_icon_layer);
    gbitmap_destroy(s_bt_icon_bitmap);

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


    #if defined(PBL_COLOR)
    graphics_context_set_fill_color(ctx, GColorRed);
    #else
    graphics_context_set_fill_color(ctx, GColorLightGray);
    #endif
    
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

    graphics_draw_text(ctx, s_buffer, s_time_font,
            layer_get_bounds(layer), GTextOverflowModeFill, GTextAlignmentLeft, NULL);

}

static void date_update_proc(Layer *layer, GContext *ctx) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char s_buffer[10];
    strftime(s_buffer, sizeof(s_buffer), "%a %m/%d", t);

    graphics_draw_text(ctx, s_buffer, s_time_font,
            layer_get_bounds(layer), GTextOverflowModeFill, GTextAlignmentRight, NULL);

}

static void ticks_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    GPoint center = GPoint(bounds.size.w/2,bounds.size.h/2);
    graphics_context_set_fill_color(ctx, GColorLightGray);
    graphics_context_set_stroke_color(ctx, GColorLightGray);

    for (int i = 1; i < 60; ++i) {
        GPoint ray_endpoint = gpoint_from_polar(GRect(-50, -50, 244, 244), GOvalScaleModeFitCircle, i * (TRIG_MAX_ANGLE / 60));
        graphics_draw_line(ctx, ray_endpoint, center);
    }

    #if defined(PBL_COLOR)
    graphics_context_set_fill_color(ctx, GColorChromeYellow);
    graphics_context_set_stroke_color(ctx, GColorChromeYellow);

    int ray_angles[12] = { 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60};
    for (int i = 0; i < 12; ++i) {
        GPoint ray_endpoint = gpoint_from_polar(GRect(-50, -50, 244, 244), GOvalScaleModeFitCircle, ray_angles[i] * (TRIG_MAX_ANGLE / 12));
        graphics_draw_line(ctx, ray_endpoint, center);
    }
    #endif


    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(3, 3, bounds.size.w - 6, bounds.size.h - 6), 5, GCornersAll);

    //numbers
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    // char s_buffer[3];
    struct Label LABELS[] = {
        {"12", (GPoint) {(144/2)-(LABELSIZE/2),4}},
        {"3", (GPoint) {144-4-LABELSIZE, (144/2)-(LABELSIZE/2)}},
        {"6", (GPoint) {(144/2)-(LABELSIZE/2),144-4-LABELSIZE}},
        {"9", (GPoint) {4, (144/2)-(LABELSIZE/2)}},
        {"1", (GPoint) {108-(LABELSIZE/2),4+ANGLEDOFFSET}},
        {"2", (GPoint) {144-4-LABELSIZE-ANGLEDOFFSET,36-(LABELSIZE/2)}},
        {"4", (GPoint) {144-4-LABELSIZE-ANGLEDOFFSET,108-(LABELSIZE/2)}},
        {"5", (GPoint) {108-(LABELSIZE/2),144-4-LABELSIZE-ANGLEDOFFSET}},
        {"7", (GPoint) {36-(LABELSIZE/2),144-4-LABELSIZE-ANGLEDOFFSET}},
        {"8", (GPoint) {4+ANGLEDOFFSET, 108-(LABELSIZE/2)}},
        {"10", (GPoint) {4+ANGLEDOFFSET, 36-(LABELSIZE/2)}},
        {"11", (GPoint) {36-(LABELSIZE/2),4+ANGLEDOFFSET}}
        
    };

    for (int i=0; i <12; i++) {
        graphics_draw_text(ctx, LABELS[i].num, s_label_font, GRect(LABELS[i].point.x,LABELS[i].point.y, LABELSIZE, LABELSIZE), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }

}

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}


int main(void) {
    prv_init();


    app_event_loop();
    prv_deinit();
}
