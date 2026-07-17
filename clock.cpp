#include "clock.h"
#include <WiFi.h>
#include <time.h>

const char* days[] = {
  "Sunday","Monday","Tuesday","Wednesday",
  "Thursday","Friday","Saturday"
};

void drawClock(Adafruit_SSD1306 &display, NTPClient &timeClient)
{
  timeClient.update();

  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = localtime(&epochTime);

  char dateBuffer[20];
  sprintf(dateBuffer, "%02d/%02d/%04d",
          ptm->tm_mday,
          ptm->tm_mon + 1,
          ptm->tm_year + 1900);

  display.clearDisplay();

  // Status Bar
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  if (WiFi.status() == WL_CONNECTED)
    display.print("WiFi");
  else
    display.print("OFF");

  // display.setCursor(100, 0);
  // display.print("[ ]");

  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Time
  String currentTime = timeClient.getFormattedTime();
  currentTime = currentTime.substring(0, 5);

  int16_t x1, y1;
  uint16_t w, h;

  display.setTextSize(2);
  display.getTextBounds(currentTime, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 18);
  display.print(currentTime);

  // Day
  display.setTextSize(1);
  display.getTextBounds(days[ptm->tm_wday], 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 42);
  display.print(days[ptm->tm_wday]);

  // Date
  display.getTextBounds(dateBuffer, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 54);
  display.print(dateBuffer);

  display.display();
}