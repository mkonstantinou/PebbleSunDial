#include <pebble.h>

#ifndef PI
#define PI (3.141592)
#endif

#ifndef NULL
#define NULL (0)
#endif

static Window *s_main_window;

static GBitmap *BITMAP_WEATHER_SUN;
static GBitmap *BITMAP_WEATHER_MOON;

static BitmapLayer *weatherLayer;
static GBitmap *weatherBitmap;

static BitmapLayer *timeOfDayLayer;
static GBitmap *timeOfDayBitmap;

static TextLayer *timeLayer;
static TextLayer *degreeLayer;

static AppSync s_sync;
static uint8_t s_sync_buffer[64];

const char* time_format = "%H:%M:%S";

struct tm* sunrise_local;
struct tm* sunset_local;

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,
  WEATHER_TEMPERATURE_KEY = 0x1,
  //WEATHER_CITY_KEY = 0x2,
  WEATHER_SUNRISE_KEY = 0X2,
  WEATHER_SUNSET_KEY = 0x3
};

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_WEATHER_CLOUDY,
  RESOURCE_ID_WEATHER_CLOUDY,
  RESOURCE_ID_WEATHER_CLOUDY
};

static int stringToTM(const char* str, struct tm* time) {
  unsigned int i;
  int digit;
  int timeBuffer = 0;
  int formatCounter = 0;
  const int asciiDiff = 48;
  
  if (str == NULL || time == NULL || strlen(str) < sizeof(time_format)) 
    return -1;
  
  for (i = 0; i < strlen(str); i++) {
    if (str[i] == ':') {
      timeBuffer /= 10;
      
      switch (formatCounter) {
        case 0:
          time->tm_hour = timeBuffer;
          break;
        case 1:
          time->tm_min = timeBuffer;
          break;
      }
      
      timeBuffer = 0;
      formatCounter++;
      
    } else {
      digit = str[i] - asciiDiff;
      timeBuffer += digit;
      timeBuffer *= 10;
    }
  }
  
  timeBuffer /= 10;
  time->tm_sec = timeBuffer;
  return i;
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "sync_tuple_changed_callback");
  const char* sun_string;
  
  switch (key) {
    case WEATHER_TEMPERATURE_KEY:
      // App Sync keeps new_tuple in s_sync_buffer, so we may use it directly
      text_layer_set_text(degreeLayer, new_tuple->value->cstring);
      break;
    
    case WEATHER_ICON_KEY:
      if (weatherBitmap) {
        gbitmap_destroy(weatherBitmap);
      }
      
      weatherBitmap = gbitmap_create_with_resource(WEATHER_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_compositing_mode(weatherLayer, GCompOpSet);
      bitmap_layer_set_bitmap(weatherLayer, weatherBitmap);
      break;
    /*
    case WEATHER_CITY_KEY:
      //text_layer_set_text(degreeLayer, new_tuple->value->cstring);
      break;
    */
    case WEATHER_SUNRISE_KEY:
      sun_string = malloc(sizeof(time_format));
      sun_string = &(new_tuple->value->cstring[0]);
      stringToTM(sun_string, sunrise_local);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "C sunrise: %s", (char*)sun_string);
      memory_cache_flush((char*)sun_string, sizeof(sun_string));
      break;
    
    case WEATHER_SUNSET_KEY:
      sun_string = malloc(sizeof(time_format));
      sun_string = &(new_tuple->value->cstring[0]);
      stringToTM(sun_string, sunset_local);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "C sunset: %s", (char*)sun_string);
      memory_cache_flush((char*)sun_string, sizeof(sun_string));
      break;
  }
}

static void request_weather(void) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  
  if (!iter) {
    return;
  }
  
  int value = 1;
  dict_write_int(iter, 1, &value, sizeof(int), true);
  dict_write_end(iter);
  
  app_message_outbox_send();
}

GRect getCoordsByAngle(int angle, int w, int h, int circle_offset) {
  
  int newY = (-cos_lookup(angle) * (90 - circle_offset) / TRIG_MAX_RATIO) + 80 ;
  int newX = (sin_lookup(angle) * (90 - circle_offset) / TRIG_MAX_RATIO) + 80 ;
  
  return GRect(newX, newY, w, h);
}

short isDaylight(struct tm *tick_time) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "isDaylight: %i >= %i", tick_time->tm_hour, sunrise_local->tm_hour);
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "isDaylight: %i <= %i", tick_time->tm_hour, sunset_local->tm_hour);
  
  // If hour hour between hours of sunrise and sunset
	if (tick_time->tm_hour >= sunrise_local->tm_hour && tick_time->tm_hour <= sunset_local->tm_hour) {
    if (tick_time->tm_hour == sunrise_local->tm_hour && tick_time->tm_min >= sunrise_local->tm_min) {
      // If hour is same but minute is greater than sunrise
      return true;
    } else if (tick_time->tm_hour == sunset_local->tm_hour && tick_time->tm_min <= sunset_local->tm_min) {
      // If hour is same but minute is less than sunset
      return true;
    } else if (tick_time->tm_hour != sunrise_local->tm_hour && tick_time->tm_hour != sunset_local->tm_hour) {
      // If hours are different
      return true;
    }
  }
  
	return false;
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  
  static char buffer[] = "99:99";
  
  //update sun/moon
  if ( isDaylight(tick_time) && bitmap_layer_get_bitmap(timeOfDayLayer) != BITMAP_WEATHER_SUN ) {
	  bitmap_layer_set_bitmap(timeOfDayLayer, BITMAP_WEATHER_SUN);
  } else if ( !isDaylight(tick_time) && bitmap_layer_get_bitmap(timeOfDayLayer) != BITMAP_WEATHER_MOON ) {
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
  
  text_layer_set_text(timeLayer, buffer);
  
}

static void main_window_load(Window *window) {
  GColor backgroundColor = GColorBlack;
  window_set_background_color(window, backgroundColor);
  
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
  
  timeLayer = text_layer_create(GRect(60, 70, 60, 20));
  text_layer_set_text(timeLayer, "9999");
  text_layer_set_background_color(timeLayer, backgroundColor);
  text_layer_set_text_color(timeLayer, GColorElectricBlue);
  text_layer_set_text_alignment(timeLayer, GTextAlignmentCenter);
  text_layer_set_font(timeLayer, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  
  degreeLayer = text_layer_create(GRect(66, 90, 55, 20));
  text_layer_set_text(degreeLayer, "HOT");
  text_layer_set_background_color(degreeLayer, backgroundColor);
  text_layer_set_text_color(degreeLayer, GColorWhite);
  text_layer_set_text_alignment(degreeLayer, GTextAlignmentCenter);
  text_layer_set_font(degreeLayer, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  
  layer_add_child(window_layer, text_layer_get_layer(timeLayer));
  layer_add_child(window_layer, text_layer_get_layer(degreeLayer));
  
  sunrise_local = malloc(sizeof(struct tm));
  sunset_local = malloc(sizeof(struct tm));
  sunrise_local->tm_hour = 8;
  sunrise_local->tm_min = 0;
  sunrise_local->tm_sec = 0;
  
  sunset_local->tm_hour = 20;
  sunset_local->tm_min = 0;
  sunset_local->tm_sec = 0;
  
  Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 0),
    TupletCString(WEATHER_TEMPERATURE_KEY, "---\u00B0"),
    //TupletCString(WEATHER_CITY_KEY, "St Pebblesburg"),
    TupletCString(WEATHER_SUNRISE_KEY, "08:00:00"),
    TupletCString(WEATHER_SUNSET_KEY, "20:00:00")
  };
  
  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
  
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  request_weather();
}

static void main_window_unload(Window *window) {
  window_stack_remove(s_main_window, true);
  
  bitmap_layer_destroy(timeOfDayLayer);
  gbitmap_destroy(timeOfDayBitmap);
  
  bitmap_layer_destroy(weatherLayer);
  gbitmap_destroy(weatherBitmap);
  gbitmap_destroy(BITMAP_WEATHER_SUN);
  gbitmap_destroy(BITMAP_WEATHER_MOON);
  
  text_layer_destroy(degreeLayer);
  text_layer_destroy(timeLayer);
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