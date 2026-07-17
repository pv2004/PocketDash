#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include "config.h"
#include "weather.h"

String temperature = "--";
String humidity = "--";
String windSpeed = "--";
String weather = "--";
int aqi = 0;
String aqiStatus = "--";

String getWeatherText(int code)
{
    switch(code)
    {
        case 0: return "Clear";
        case 1: return "Mostly Clear";
        case 2: return "Partly Cloudy";
        case 3: return "Cloudy";
        case 45: return "Fog";
        case 48: return "Fog";
        case 51: return "Drizzle";
        case 53: return "Drizzle";
        case 55: return "Heavy Drizzle";
        case 61: return "Rain";
        case 63: return "Moderate Rain";
        case 65: return "Heavy Rain";
        case 71: return "Snow";
        case 80: return "Rain Shower";
        case 95: return "Thunderstorm";
        default: return "Unknown";
    }
}
String getAQIStatus(int value)
{
    if (value <= 50) return "Good";
    if (value <= 100) return "Moderate";
    if (value <= 150) return "Unhealthy";
    if (value <= 200) return "Poor";
    return "Hazard";
}

void fetchWeather()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected!");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    String url =
        "https://api.open-meteo.com/v1/forecast?latitude=" +
        String(MY_LAT,4) +
        "&longitude=" +
        String(MY_LON,4) +
        "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code";

    Serial.println("Fetching Weather...");
    Serial.println(url);

    if(!http.begin(client,url))
    {
        Serial.println("Weather connection failed");
        return;
    }

    http.setTimeout(10000);

    int httpCode = http.GET();

    if(httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();

        JsonDocument doc;

        DeserializationError error = deserializeJson(doc,payload);

        if(!error)
        {
            JsonObject current = doc["current"];

            temperature =
                String(current["temperature_2m"].as<float>(),1) + "C";

            humidity =
                String(current["relative_humidity_2m"].as<int>()) + "%";

            windSpeed =
                String(current["wind_speed_10m"].as<float>(),0) + "km";

            int code = current["weather_code"];

            weather = getWeatherText(code);

            Serial.println("Weather Loaded!");
        }
        else
        {
            Serial.println(error.c_str());
        }
    }
    else
    {
        Serial.print("Weather Error: ");
        Serial.println(http.errorToString(httpCode));
    }

    http.end();

// ----------------------------
// Fetch AQI
// ----------------------------

WiFiClientSecure clientAQI;
clientAQI.setInsecure();

HTTPClient httpAQI;

String aqiUrl =
    "https://air-quality-api.open-meteo.com/v1/air-quality?latitude=" +
    String(MY_LAT,4) +
    "&longitude=" +
    String(MY_LON,4) +
    "&current=us_aqi";

Serial.println("Fetching AQI...");
Serial.println(aqiUrl);

if(httpAQI.begin(clientAQI, aqiUrl))
{
    httpAQI.setTimeout(10000);

    int aqiCode = httpAQI.GET();

    if(aqiCode == HTTP_CODE_OK)
    {
        String payload = httpAQI.getString();

        JsonDocument docAQI;

        DeserializationError errorAQI =
            deserializeJson(docAQI, payload);

        if(!errorAQI)
        {
            aqi = docAQI["current"]["us_aqi"].as<int>();

            aqiStatus = getAQIStatus(aqi);

            Serial.println("AQI Loaded!");
        }
        else
        {
            Serial.println(errorAQI.c_str());
        }
    }
    else
    {
        Serial.print("AQI Error: ");
        Serial.println(httpAQI.errorToString(aqiCode));
    }

    httpAQI.end();
}
}

void drawWeather(Adafruit_SSD1306 &display)
{
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(38,0);
    display.println("Weather");

    display.drawLine(0,10,127,10,SSD1306_WHITE);

    // Temperature
    display.setTextSize(2);
    display.setCursor(0,16);
    display.print(temperature);

    // Weather condition
    display.setTextSize(1);
    display.setCursor(72,22);
    display.print(weather);

    // AQI
    display.setCursor(0,42);
    display.print("AQI:");
    display.print(aqi);

    display.setCursor(55,42);
    display.print(aqiStatus);

    // Humidity
    display.setCursor(0,54);
    display.print("H:");
    display.print(humidity);

    // Wind
    display.setCursor(72,54);
    display.print("W:");
    display.print(windSpeed);

    display.display();
}