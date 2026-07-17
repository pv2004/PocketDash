#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "config.h"
#include "clock.h"
#include "display.h"
#include "quote.h"
#include "flight.h"
#include "weather.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);

// ----------------------
// Screen Manager
// ----------------------
int currentScreen = 0;

unsigned long lastSwitch = 0;
const unsigned long switchInterval = 6000;   // 6 seconds

// ----------------------
// Quote Refresh
// ----------------------
unsigned long lastQuoteFetch = 0;
const unsigned long quoteInterval = 300000UL;   // 5 minutes

// ----------------------
// Flight Refresh
// ----------------------
unsigned long lastFlightFetch = 0;
const unsigned long flightInterval = 60000UL;   // 60 seconds

// ----------------------
// weather Refresh
// ----------------------
unsigned long lastWeatherFetch = 0;
const unsigned long weatherInterval = 60000UL; //60 seconds

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println("OLED Failed");
    while (true);
  }

  display.setTextColor(SSD1306_WHITE);

  showMessage(display, "Pocket", "Starting...");
  delay(1500);

  showMessage(display, "WiFi", "Connecting...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");

  showMessage(display, "WiFi", "Connected!");
  delay(1500);

  delay(2000); 

  timeClient.begin();
  timeClient.update();


  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());


  // Initial data fetch
  fetchQuote();
  fetchFlight();
  fetchWeather();


  lastQuoteFetch = millis();
  lastFlightFetch = millis();
  lastWeatherFetch = millis();
}

void loop()
{
  // Refresh quote every 5 minutes
  if (millis() - lastQuoteFetch >= quoteInterval)
  {
    fetchQuote();
    lastQuoteFetch = millis();
  }

  // Refresh flight every 60 seconds
  if (millis() - lastFlightFetch >= flightInterval)
  {
    fetchFlight();
    lastFlightFetch = millis();
  }

  //weather fetch
  if (millis() - lastWeatherFetch >= weatherInterval)
  {
      fetchWeather();
      lastWeatherFetch = millis();
  }

  // Change screen every 6 seconds
  if (millis() - lastSwitch >= switchInterval)
  {
    lastSwitch = millis();

    currentScreen++;

    if (currentScreen > 3)
      currentScreen = 0;
  }

  switch (currentScreen)
  {
    case 0:
      drawClock(display, timeClient);
      break;

    case 1:
      drawQuote(display);
      break;

    case 2:
      drawFlight(display);
      break;

    case 3:
      drawWeather(display);
      break;
  }

  delay(100);
}