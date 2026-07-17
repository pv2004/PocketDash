#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SSD1306.h>

void showMessage(Adafruit_SSD1306 &display, String title, String msg);

#endif