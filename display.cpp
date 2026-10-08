#include "display.h"

void showMessage(Adafruit_SSD1306 &display, String title, String msg)
{
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(15, 5);
    display.println(title);

    display.drawLine(0, 24, 128, 24, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(5, 40);
    display.println(msg);

    display.display();
}

#include <WiFi.h>

void drawWiFiSignal(Adafruit_SSD1306 &display)
{
    int rssi = WiFi.RSSI();
    int bars = 0;
    
    if (WiFi.status() == WL_CONNECTED) {
        if (rssi > -55) bars = 4;
        else if (rssi > -65) bars = 3;
        else if (rssi > -75) bars = 2;
        else if (rssi > -85) bars = 1;
        else bars = 1;
    }
    
    // Draw in top right corner (x=114, y=8 is the base)
    int x = 114;
    int y = 8; 
    
    for (int i = 0; i < 4; i++) {
        if (i < bars) {
            display.fillRect(x + i*3, y - i*2, 2, i*2 + 2, SSD1306_WHITE);
        } else {
            display.drawRect(x + i*3, y - i*2, 2, i*2 + 2, SSD1306_WHITE);
        }
    }
}