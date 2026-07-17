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