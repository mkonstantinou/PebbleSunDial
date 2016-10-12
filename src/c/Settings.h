#pragma once


typedef struct ClaySettings {
  GColor backgroundColor;
  GColor clockColor;
  short useLocalWeather;
  char* weatherProvider;
  int weatherUpdateInterval;
  int optionLayer;
  char* dateFormat;
} __attribute__((__packed__)) ClaySettings;

void default_settings();
ClaySettings load_settings();
void save_settings();