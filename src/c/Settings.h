#pragma once


typedef struct ClaySettings {
  GColor backgroundColor;
  GColor dateColor;
  GColor clockColor;
  GColor tempColor;
  int weatherUpdateInterval;
  char* dateFormat;
} __attribute__((__packed__)) ClaySettings;

void default_settings();
ClaySettings load_settings();
void save_settings();