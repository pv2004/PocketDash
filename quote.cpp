#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "quote.h"

String quote = "Loading...";
String author = "";

void fetchQuote()
{
  Serial.println("Fetching Quote...");

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected!");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();   // Skip certificate verification (OK for development)

  HTTPClient http;

  if (!http.begin(client, "https://zenquotes.io/api/random"))
  {
    Serial.println("Failed to connect.");
    return;
  }
  http.setTimeout(10000);   // 10 seconds
  
  int httpCode = http.GET();

  Serial.print("HTTP Code: ");
  Serial.println(httpCode);

  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();

    Serial.println("Response:");
    Serial.println(payload);

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, payload);

    if (!error)
    {
      quote = doc[0]["q"].as<String>();
      author = doc[0]["a"].as<String>();

      Serial.println("Quote Loaded!");
    }
    else
    {
      Serial.print("JSON Error: ");
      Serial.println(error.c_str());
    }
  }
  else
  {
    Serial.print("HTTP Error: ");
    Serial.println(httpCode);
  }

  http.end();
}

void drawQuote(Adafruit_SSD1306 &display)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(42, 0);
  display.println("Quote");

  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 20);
  display.println(quote);

  display.setCursor(0, 54);
  display.print("- ");
  display.println(author);

  display.display();
}