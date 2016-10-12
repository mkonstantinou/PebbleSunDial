#include <pebble.h>
#include "src/c/Settings.h"

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
static TextLayer *optionLayer;

const char* time_format = "%H:%M:%S";

struct tm* sunrise_local;
struct tm* sunset_local;

int weather_update_interval = 15;
int weather_update_interval_counter;

ClaySettings settings;

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,
  WEATHER_TEMPERATURE_KEY = 0x1,
  WEATHER_SUNRISE_KEY = 0X2,
  WEATHER_SUNSET_KEY = 0x3
};

enum OptionLayerKey {
  DATE_KEY = 0,
  LOCATION_KEY = 1
};

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_WEATHER_THUNDER,
  RESOURCE_ID_WEATHER_RAIN,
  RESOURCE_ID_WEATHER_SNOW,
  RESOURCE_ID_WEATHER_CLEAR,
  RESOURCE_ID_WEATHER_CLOUDY  
};

/* 
 * stringToTM
 * Pre: Takes string and time structure
 * Post: converts time string to struct tm*
 */
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
    } else if (str[i] == 'P') {    // Handle 'PM' at end of string
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

/* 
 * request_weather
 * Pre: Takes nothing
 * Post: Send outbound message to js to request weather
 */
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

/* 
 * layers_set_background_color
 * Pre: Takes GColor
 * Post: applies background color to window and all text layers
 */
static void layers_set_background_color(GColor color) {
  window_set_background_color(s_main_window, color);
  text_layer_set_background_color(timeLayer, color);
  text_layer_set_background_color(degreeLayer, color);
  text_layer_set_background_color(optionLayer, color);
}

/* 
 * app_inbox_received_handler
 * Pre: Takes DictionaryIterator and context reference
 * Post: Update settings and variables based js messages (config/weather)
 */
static void app_inbox_received_handler(DictionaryIterator *iter, void *context) {
  
  // ------------------------SETTINGS------------------------ //
  
  // Background color settings
  Tuple *bg_color_tuple = dict_find(iter, MESSAGE_KEY_BACKGROUND_COLOR);
  if(bg_color_tuple) {
    GColor bg_color = GColorFromHEX(bg_color_tuple->value->int32);
    settings.backgroundColor = bg_color;
    layers_set_background_color(bg_color);
  }
  
  // Clock color settings
  Tuple *clock_color_tuple = dict_find(iter, MESSAGE_KEY_CLOCK_COLOR);
  if(clock_color_tuple) {
    GColor clock_color = GColorFromHEX(clock_color_tuple->value->int32);
    settings.clockColor = clock_color;
    text_layer_set_text_color(timeLayer, clock_color);
  }
  
  // Weather update preferences
  Tuple *weather_interval_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_INTERVAL);
  if (weather_interval_tuple) {
    weather_update_interval = weather_interval_tuple->value->uint32;
    settings.weatherUpdateInterval = weather_update_interval;
    request_weather();
  }
  
  // Get option layer type
  Tuple *option_layer_tuple = dict_find(iter, MESSAGE_KEY_OPTION_LAYER);
  if (option_layer_tuple) {
    uint32_t option = option_layer_tuple->value->uint32;
    settings.optionLayer = option;
  }
  
  // Get date format
  Tuple *date_format_tuple = dict_find(iter, MESSAGE_KEY_DATE_FORMAT);
  if (date_format_tuple) {
    char* date_format = date_format_tuple->value->cstring;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "date format string: %s", date_format);
    strcpy(settings.dateFormat, date_format);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "date setting: %s", settings.dateFormat);
  }
  
  save_settings();
  
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
    stringToTM(sun_string, sunset_local);
    memory_cache_flush((char*)sun_string, sizeof(sun_string));
  }
}

/* 
 * getCoordsByAngle
 * Pre: angle in radians, width of icon, height of icon, offset from circle edge
 * Post: Returns GRect with x and y based on angle, and width and height from paramters
 */
GRect getCoordsByAngle(int angle, int w, int h, int circle_offset) {
  
  int newY = (-cos_lookup(angle) * (90 - circle_offset) / TRIG_MAX_RATIO) + 80 ;
  int newX = (sin_lookup(angle) * (90 - circle_offset) / TRIG_MAX_RATIO) + 80 ;
  
  return GRect(newX, newY, w, h);
}

/* 
 * isDaylight
 * Pre: Takes reference to time struct
 * Post: returns 1 if the sun is out, 0 otherwise
 */
short isDaylight(struct tm *tick_time) {
  // If  hour between hours of sunrise and sunset
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

/* 
 * tick_handler
 * Pre: Takes reference to time struct and units changed since last tick
 * Post: Update weather, text, and rotating icons
 */
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
          
  weather_update_interval_counter += 1;
  if (weather_update_interval_counter >= weather_update_interval) {
    request_weather();
    weather_update_interval_counter = 0;
  }
  
  static char buffer[] = "99:99";
  static char dateBuffer[] = "01/01";
  
  //Update sun/moon depending on time
  if ( isDaylight(tick_time)) {
    if ( bitmap_layer_get_bitmap(timeOfDayLayer) != BITMAP_WEATHER_SUN ) {
      bitmap_layer_set_bitmap(timeOfDayLayer, BITMAP_WEATHER_SUN);
    }
  } else {
    if ( bitmap_layer_get_bitmap(timeOfDayLayer) != BITMAP_WEATHER_MOON ) {
      bitmap_layer_set_bitmap(timeOfDayLayer, BITMAP_WEATHER_MOON);
    }
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
  
  // Update time
  text_layer_set_text(timeLayer, buffer);
  
  // Update option layer
  //if (settings.optionLayer == 0) {
    strftime(dateBuffer, sizeof(dateBuffer), settings.dateFormat, tick_time);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating date - %s", settings.dateFormat);
    text_layer_set_text(optionLayer, dateBuffer);
  //}
  
}

/* 
 * main_window_load
 * Pre: Takes reference to main window
 * Post: Initialize all assets
 */
static void main_window_load(Window *window) {
  
  //Get parent layer
  Layer *window_layer = window_get_root_layer(window);
  
  BITMAP_WEATHER_SUN = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_SUN);
  BITMAP_WEATHER_MOON = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_MOON);
  
  timeOfDayLayer = bitmap_layer_create(GRect(80, 10, 21, 21));
  bitmap_layer_set_compositing_mode(timeOfDayLayer, GCompOpSet);
  bitmap_layer_set_bitmap(timeOfDayLayer, BITMAP_WEATHER_SUN);
  
  weatherBitmap = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_CLOUDY);
  weatherLayer = bitmap_layer_create(GRect(80, 20, 21, 21));
  bitmap_layer_set_compositing_mode(weatherLayer, GCompOpSet);
  bitmap_layer_set_bitmap(weatherLayer, weatherBitmap);
  
  // Top Layer
  optionLayer = text_layer_create(GRect(55, 60, 70, 20));
  text_layer_set_text(optionLayer, "01/01");
  text_layer_set_text_color(optionLayer, GColorWhite);
  text_layer_set_text_alignment(optionLayer, GTextAlignmentCenter);
  text_layer_set_font(optionLayer, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  
  // Middle Layer
  timeLayer = text_layer_create(GRect(60, 80, 60, 20));
  text_layer_set_text(timeLayer, "9999");
  text_layer_set_text_color(timeLayer, settings.clockColor);
  text_layer_set_text_alignment(timeLayer, GTextAlignmentCenter);
  text_layer_set_font(timeLayer, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  
  // Bottom Layer
  degreeLayer = text_layer_create(GRect(66, 100, 55, 20));
  text_layer_set_text(degreeLayer, "");
  text_layer_set_text_color(degreeLayer, GColorWhite);
  text_layer_set_text_alignment(degreeLayer, GTextAlignmentCenter);
  text_layer_set_font(degreeLayer, fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  
  layers_set_background_color(settings.backgroundColor);  
  layer_add_child(window_layer, bitmap_layer_get_layer(timeOfDayLayer));
  layer_add_child(window_layer, bitmap_layer_get_layer(weatherLayer));
  layer_add_child(window_layer, text_layer_get_layer(optionLayer));
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
  
  // Register message inbox handler
  app_message_register_inbox_received(app_inbox_received_handler);
  // TODO: Appropriate message size
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  load_settings();
  request_weather();
}

/* 
 * main_window_unload
 * Pre: Takes reference to main windo
 * Post: Frees all memory
 */
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

/* 
 * init
 * Pre: Takes nothing
 * Post: Initialize window, register window loader and unloader, subscribe to time service
 */
static void init() {
  settings = load_settings();
  
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

/* 
 * deinit
 * Pre: Takes nothing
 * Post: Destroys all windows
 */
static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

/* 
 * main
 * Pre: Takes nothing; called on start
 * Post: Main loop
 */
int main(void) {
  init();
  app_event_loop();
  deinit();
}