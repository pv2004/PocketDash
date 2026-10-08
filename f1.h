#ifndef F1_H
#define F1_H

#include <Adafruit_SSD1306.h>

void fetchF1(time_t currentLocalEpoch);
void drawF1(Adafruit_SSD1306 &display);

#endif
