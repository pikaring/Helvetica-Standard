#include "watch.h"

#include "pebble.h"

static Window *window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_num_label;
static BitmapLayer *s_background_layer, *s_BT_layer, *s_battery_layer;
static GBitmap *s_background_bitmap, *s_BT_ON_bitmap, *s_BT_OFF_bitmap, *s_battery_bitmap[6];
static const uint32_t segments[] = { 400, 200, 400, 200, 400 };

static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static GPath *s_second_arrow, *s_minute_arrow, *s_hour_arrow;
static char s_day_buffer[32];

static void bt_handler(bool connected) {
  // Show current connection state
  if (connected) {
	bitmap_layer_set_bitmap( s_BT_layer, s_BT_ON_bitmap);
  } else {
	bitmap_layer_set_bitmap( s_BT_layer, s_BT_OFF_bitmap);
	VibePattern pat = {
  .durations = segments,
  .num_segments = ARRAY_LENGTH(segments),
};
vibes_enqueue_custom_pattern(pat);
  }
}

static void battery_handler(BatteryChargeState new_state) {
  if(new_state.charge_percent<20){
		bitmap_layer_set_bitmap( s_battery_layer, s_battery_bitmap[0]);
	}else if(new_state.charge_percent<40){
		bitmap_layer_set_bitmap( s_battery_layer, s_battery_bitmap[1]);
	}else if(new_state.charge_percent<70){
		bitmap_layer_set_bitmap( s_battery_layer, s_battery_bitmap[2]);
	}else{
		bitmap_layer_set_bitmap( s_battery_layer, s_battery_bitmap[3]);
	}
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}
static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

	// second/minute/hour hand
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  //gpath_draw_outline(ctx, s_minute_arrow);

  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  //gpath_draw_outline(ctx, s_hour_arrow);

	#ifdef PBL_COLOR
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_context_set_stroke_color(ctx, GColorRed);
#else
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);
#endif

  gpath_rotate_to(s_second_arrow, TRIG_MAX_ANGLE * t->tm_sec / 60);
  gpath_draw_filled(ctx, s_second_arrow);
//  gpath_draw_outline(ctx, s_second_arrow);

  // dot in the middle
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 4, bounds.size.h / 2 - 14, 8, 8), 4, GCornersAll);
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(s_day_buffer, sizeof(s_day_buffer), "%a %b %d %Y", t);
  text_layer_set_text(s_day_label, s_day_buffer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(window));
}

static void window_load(Window *window) {
	GFont day_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_14));

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
#ifdef PBL_SDK_2
  window_set_fullscreen(window, true);
#endif
	s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

	s_background_layer = bitmap_layer_create( GRect(0, -10, 144, 168));
#ifdef PBL_PLATFORM_APLITE
	s_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BACKGROUND_MONO );
  //bitmap_layer_set_compositing_mode(s_background_layer, GCompOpSet);
#elif PBL_PLATFORM_BASALT
	s_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BACKGROUND );
  bitmap_layer_set_compositing_mode(s_background_layer, GCompOpSet);
#endif
	bitmap_layer_set_bitmap( s_background_layer, s_background_bitmap);
	layer_add_child( window_get_root_layer( window ) , bitmap_layer_get_layer(s_background_layer));

	s_BT_layer = bitmap_layer_create( GRect(2,2,15,20));
	s_BT_ON_bitmap = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BT_ON );
	s_BT_OFF_bitmap = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BT_OFF );
#ifdef PBL_PLATFORM_BASALT
  bitmap_layer_set_compositing_mode(s_BT_layer, GCompOpSet);
#endif
	layer_add_child( window_get_root_layer( window ) , bitmap_layer_get_layer(s_BT_layer));
	
	s_battery_layer = bitmap_layer_create( GRect(130,1,10,21));
	s_battery_bitmap[0] = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BATTERY_0 );
	s_battery_bitmap[1] = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BATTERY_1 );
	s_battery_bitmap[2] = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BATTERY_2 );
	s_battery_bitmap[3] = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_BATTERY_3 );
	bitmap_layer_set_bitmap( s_battery_layer, s_battery_bitmap[5]);
#ifdef PBL_PLATFORM_BASALT
  bitmap_layer_set_compositing_mode(s_battery_layer, GCompOpSet);
#endif
	layer_add_child( window_get_root_layer( window ) , bitmap_layer_get_layer(s_battery_layer));

	s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);
  layer_add_child(window_layer, s_date_layer);

  s_day_label = text_layer_create(GRect(0, 148, 144, 20));
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, GColorBlack);
  text_layer_set_text_color(s_day_label, GColorWhite);
	text_layer_set_font ( s_day_label ,  day_font);
	text_layer_set_text_alignment(s_day_label, GTextAlignmentCenter);
  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));

	s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
	
	bt_handler(bluetooth_connection_service_peek());
	
	battery_handler(battery_state_service_peek());
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_date_layer);

  text_layer_destroy(s_day_label);
  text_layer_destroy(s_num_label);

  layer_destroy(s_hands_layer);
}

static void init() {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  s_day_buffer[0] = '\0';
//  s_num_buffer[0] = '\0';

  // init hand paths
  s_second_arrow = gpath_create(&SECOND_HAND_POINTS);
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

//Layer *window_layer = window_get_root_layer(window);
//  GRect bounds = layer_get_bounds(window_layer);
  GRect bounds = GRect(0,2,144,144);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_second_arrow, center);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
	
	bluetooth_connection_service_subscribe(bt_handler);
	
  battery_state_service_subscribe(battery_handler);
}

static void deinit() {
  gpath_destroy(s_second_arrow);
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(s_tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}