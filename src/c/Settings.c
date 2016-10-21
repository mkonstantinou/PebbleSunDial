#include <pebble.h>
#include "Settings.h"

ClaySettings settings;

//Clay settings
 void default_settings() {
  settings.backgroundColor = GColorBlack;
  settings.dateColor = GColorWhite;
  settings.clockColor = GColorElectricBlue;
  settings.tempColor = GColorWhite;
  settings.weatherUpdateInterval = 15;
  settings.dateFormat = "%d";
}

// Read settings from persistent storage
ClaySettings load_settings() {
  // Load the default settings
  default_settings();
  // Read settings from persistent storage, if they exist
  persist_read_data(MESSAGE_KEY_SETTINGS_KEY, &settings, sizeof(settings));
  return settings;
}

// Save the settings to persistent storage
void save_settings() {
  persist_write_data(MESSAGE_KEY_SETTINGS_KEY, &settings, sizeof(settings));
}
