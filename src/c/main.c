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

const char* time_format = "%H:%M:%S";

struct tm* sunrise_local;
struct tm* sunset_local;

int weather_update_interval = 15;
int weather_update_interval_counter;

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,
  WEATHER_TEMPERATURE_KEY = 0x1,
  WEATHER_SUNRISE_KEY = 0X2,
  WEATHER_SUNSET_KEY = 0x3
};

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_WEATHER_THUNDER,
  RESOURCE_ID_WEATHER_RAIN,
  RESOURCE_ID_WEATHER_SNOW,
  RESOURCE_ID_WEATHER_CLEAR,
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
    } else if (str[i] == 'P') {
      time->tm_hour += 12;
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


static void app_inbox_received_handler(DictionaryIterator *iter, void *context) {
  
  // ------------------------SETTINGS------------------------ //
  
  // Read color preferences
  Tuple *bg_color_tuple = dict_find(iter, MESSAGE_KEY_BACKGROUND_COLOR);
  if(bg_color_tuple) {
    GColor bg_color = GColorFromHEX(bg_color_tuple->value->int32);
    window_set_background_color(s_main_window, bg_color);
  }
  
  // Weather update preferences
  Tuple *weather_interval_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_INTERVAL);
  if (weather_interval_tuple) {
    weather_update_interval = weather_interval_tuple->value->uint32;
    request_weather();
  }
  // ------------------------WEATHER------------------------ //
  
  // Read temperature
  Tuple *weather_temp_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_TEMPERATURE_KEY);
  if (weather_temp_tuple) {
    text_layer_set_text(degreeLayer, weather_temp_tuple->value->cstring);
  }
  
  // Read weather icon
  Tuple *weather_icon_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_ICON_KEY);
  if (weather_icon_tuple) {
    if (weatherBitmap) {
        gbitmap_destroy(weatherBitmap);
      }
      
      weatherBitmap = gbitmap_create_with_resource(WEATHER_ICONS[weather_icon_tuple->value->uint8]);
      bitmap_layer_set_compositing_mode(weatherLayer, GCompOpSet);
      bitmap_layer_set_bitmap(weatherLayer, weatherBitmap);
  }
  
  // Read sunrise time
  Tuple *weather_sunrise_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_SUNRISE_KEY);
  if (weather_sunrise_tuple) {
    const char* sun_string = malloc(sizeof(time_format));
    sun_string = &(weather_sunrise_tuple->value->cstring[0]);
    stringToTM(sun_string, sunrise_local);
    memory_cache_flush((char*)sun_string, sizeof(sun_string));
  }
  
  // Read sunset time
  Tuple *weather_sunset_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_SUNSET_KEY);
  if (weather_sunset_tuple) {
    const char* sun_string = malloc(sizeof(time_format));
    sun_string = &(weather_sunset_tuple->value->cstring[0]);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "MESSAGE RECEIVED - Sunset time: %s", sun_string);
    stringToTM(sun_string, sunset_local);
    
    memory_cache_flush((char*)sun_string, sizeof(sun_string));
  }
  
  // Read boolean preferences
  /*
  Tuple *second_tick_t = dict_find(iter, MESSAGE_KEY_SecondTick);
  if(second_tick_t) {
    bool second_ticks = second_tick_t->value->int32 == 1;
  }
  */
  // Read Weather preference

}

GRect getCoordsByAngle(int angle, int w, int h, int circle_offset) {
  
  int newY = (-cos_lookup(angle) * (90 - circle_offset) / TRIG_MAX_RATIO) + 80 ;
  int newX = (sin_lookup(angle) * (90 - circle_offset) / TRIG_MAX_RATIO) + 80 ;
  
  return GRect(newX, newY, w, h);
}

short isDaylight(struct tm *tick_time) {
  // If hour hour between hours of sunrise and sunset
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Current time: %i:%i", tick_time->tm_hour, tick_time->tm_min);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sunrise time: %i:%i", sunrise_local->tm_hour, sunrise_local->tm_min);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sunset time: %i:%i", sunset_local->tm_hour, sunset_local->tm_min);
  
	if (tick_time->tm_hour >= sunrise_local->tm_hour && tick_time->tm_hour <= sunset_local->tm_hour) {
    if (tick_time->tm_hour == sunrise_local->tm_hour && tick_time->tm_min >= sunrise_local->tm_min) {
      // If hour is same but minute is greater than sunrise
      return true;
    } else if (tick_time->tm_hour == sunset_local->tm_hour && tick_time->tm_min < sunset_local->tm_min) {
      // If hour is same but minute is less than sunset
      return true;
    } else if (tick_time->tm_hour > sunrise_local->tm_hour && tick_time->tm_hour < sunset_local->tm_hour) {
      // If hours are different
      return true;
    }
  }
  
	return false;
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  weather_update_interval_counter += 1;
  if (weather_update_interval_counter >= weather_update_interval) {
    request_weather();
    weather_update_interval_counter = 0;
  }
  
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
  
  //Register message inbox handler
  app_message_register_inbox_received(app_inbox_received_handler);
  //TODO: Appropriate message size
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