#ifndef WEATHER_H
#define WEATHER_H

#include <Adafruit_SSD1306.h>

// Fetch weather data from Open-Meteo
void fetchWeather();

// Draw weather screen on OLED
void drawWeather(Adafruit_SSD1306 &display);

#endif