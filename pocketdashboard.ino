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
#include "f1.h"

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

// ----------------------
// F1 Refresh
// ----------------------
unsigned long lastF1Fetch = 0;
const unsigned long f1Interval = 3600000UL; // 1 hour

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
  fetchF1(timeClient.getEpochTime());

  lastQuoteFetch = millis();
  lastFlightFetch = millis();
  lastWeatherFetch = millis();
  lastF1Fetch = millis();
}

void loop()
{
  // Change screen every 6 seconds
  if (millis() - lastSwitch >= switchInterval)
  {
    lastSwitch = millis();

    currentScreen++;

    if (currentScreen > 4)
      currentScreen = 0;
  }

  // Refresh quote every 5 minutes (only when on Quote screen)
  if (millis() - lastQuoteFetch >= quoteInterval && currentScreen == 1)
  {
    fetchQuote();
    lastQuoteFetch = millis();
  }

  // Refresh flight every 60 seconds (only when on Flight screen)
  if (millis() - lastFlightFetch >= flightInterval && currentScreen == 2)
  {
    fetchFlight();
    lastFlightFetch = millis();
  }

  // Weather fetch (only when on Weather screen)
  if (millis() - lastWeatherFetch >= weatherInterval && currentScreen == 3)
  {
      fetchWeather();
      lastWeatherFetch = millis();
  }

  // F1 fetch (only when on F1 screen)
  if (millis() - lastF1Fetch >= f1Interval && currentScreen == 4)
  {
      fetchF1(timeClient.getEpochTime());
      lastF1Fetch = millis();
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

    case 4:
      drawF1(display);
      break;
  }

  delay(100);
}