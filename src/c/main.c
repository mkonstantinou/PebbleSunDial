#include <pebble.h>
#include "main.h"

#ifndef PI
#define PI (3.141592)
#endif

static Window *s_main_window;

static GBitmap *BITMAP_WEATHER_SUN;
static GBitmap *BITMAP_WEATHER_MOON;

static BitmapLayer *weatherLayer;
static GBitmap *weatherBitmap;

static BitmapLayer *timeOfDayLayer;
static GBitmap *timeOfDayBitmap;

static TextLayer *logLabel;

GRect getCoordsByAngle(int angle, int w, int h, int circle_offset) {
  
  int newY = (-cos_lookup(angle) * (90 - circle_offset) / TRIG_MAX_RATIO) + 80 ;
  int newX = (sin_lookup(angle) * (90 - circle_offset) / TRIG_MAX_RATIO) + 80 ;
  
  return GRect(newX, newY, w, h);
}

//TODO: Query for sunset and sunrise
short isDaylight(struct tm *tick_time) {
	if (tick_time->tm_hour > 8 || tick_time->tm_hour < 20) 
		return true;
	return false;
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  
  static char buffer[] = "99:99";
  
  if ( isDaylight(tick_time) && bitmap_layer_get_bitmap(timeOfDayLayer) != BITMAP_WEATHER_SUN ) {
	bitmap_layer_set_bitmap(timeOfDayLayer, BITMAP_WEATHER_SUN);
  } else if ( !isDaylight && bitmap_layer_get_bitmap(timeOfDayLayer) != BITMAP_WEATHER_MOON ) {
	bitmap_layer_set_bitmap(timeOfDayLayer, BITMAP_WEATHER_MOON);
  }
  
  //Get position of hour hand (sun/moon)
  int angle = TRIG_MAX_ANGLE * (((tick_time->tm_hour % 12) * 30 + tick_time->tm_min / 2)) / 360;
  GRect coords = getCoordsByAngle(angle, 21, 21, 21);
  layer_set_frame((Layer*) timeOfDayLayer, coords);
  
  //Get position of minute hand (weather)
  angle=TRIG_MAX_ANGLE * tick_time->tm_min / 60;
  coords = getCoordsByAngle(angle, 21, 21, 31);
  layer_set_frame((Layer*) weatherLayer, coords);
  
  if(clock_is_24h_style()) {
    // Use 24 hour format
    strftime(buffer, sizeof("99:99"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("99:99"), "%I:%M", tick_time);
  }
  
  text_layer_set_text(logLabel, buffer);
  
}

static void main_window_load(Window *window) {
  window_set_background_color(window, GColorBlack);
  
  //Get parent layer
  Layer *window_layer = window_get_root_layer(window);
  
  BITMAP_WEATHER_SUN = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_SUN);
  BITMAP_WEATHER_MOON = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_MOON);
  
  timeOfDayLayer = bitmap_layer_create(GRect(80, 10, 21, 21));
  bitmap_layer_set_compositing_mode(timeOfDayLayer, GCompOpSet);
  bitmap_layer_set_bitmap(timeOfDayLayer, BITMAP_WEATHER_SUN);
  
  layer_add_child(window_layer, bitmap_layer_get_layer(timeOfDayLayer));
  
  
  weatherBitmap = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_CLOUDY);
  weatherLayer = bitmap_layer_create(GRect(80, 20, 21, 21));
  bitmap_layer_set_compositing_mode(weatherLayer, GCompOpSet);
  bitmap_layer_set_bitmap(weatherLayer, weatherBitmap);
  
  layer_add_child(window_layer, bitmap_layer_get_layer(weatherLayer));
  
  logLabel = text_layer_create(GRect(60, 80, 60, 20));
  text_layer_set_text(logLabel, "9999");
  text_layer_set_background_color(logLabel, GColorBlack);
  text_layer_set_text_color(logLabel, GColorMelon);
  text_layer_set_font(logLabel, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  
  layer_add_child(window_layer, text_layer_get_layer(logLabel));
}

static void main_window_unload(Window *window) {
  window_stack_remove(s_main_window, true);
  
  bitmap_layer_destroy(timeOfDayLayer);
  gbitmap_destroy(timeOfDayBitmap);
  
  bitmap_layer_destroy(weatherLayer);
  gbitmap_destroy(weatherBitmap);
}

static void init() {
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  //tick_timer_service_subscribe(MINUTE_UNIT, minute_tick_handler);
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}