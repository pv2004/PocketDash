#ifndef CLOCK_H
#define CLOCK_H

#include <Adafruit_SSD1306.h>
#include <NTPClient.h>

void drawClock(Adafruit_SSD1306 &display, NTPClient &timeClient);

#endif