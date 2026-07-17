#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <math.h>

#include "config.h"
#include "flight.h"

String flightNo = "--";
String airline = "--";
String departure = "--";
String destination = "--";
String altitude = "--";
String speed = "--";
String distance = "--";

// -----------------------------------------------------
// Calculate distance (Haversine)
// -----------------------------------------------------
float calculateDistance(float lat1, float lon1, float lat2, float lon2)
{
    const float R = 6371.0;

    float dLat = radians(lat2 - lat1);
    float dLon = radians(lon2 - lon1);

    float a =
        sin(dLat / 2) * sin(dLat / 2) +
        cos(radians(lat1)) *
        cos(radians(lat2)) *
        sin(dLon / 2) *
        sin(dLon / 2);

    float c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return R * c;
}

String getAirlineName(String code)
{
    if(code=="6E") return "IndiGo";
    if(code=="AI") return "AirIndia";
    if(code=="IX") return "AIExpr";
    if(code=="SG") return "SpiceJet";
    if(code=="QP") return "Akasa";
    if(code=="UK") return "Vistara";

    return code;
}

// -----------------------------------------------------
// Fetch Flight
// -----------------------------------------------------
void fetchFlight()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected!");
        return;
    }

    HTTPClient http;
    WiFiClientSecure client;

    client.setInsecure();

    String bbox =
      String(MY_LAT - 0.55, 4) + "," +
      String(MY_LON - 0.55, 4) + "," +
      String(MY_LAT + 0.55, 4) + "," +
      String(MY_LON + 0.55, 4);

    String url =
        "https://airlabs.co/api/v9/flights?api_key=" +
        String(AIRLABS_API_KEY) +
        "&bbox=" + bbox;

    Serial.println("Fetching Flights...");
    Serial.println(url);

    http.begin(client, url);
    http.setTimeout(10000);   // 10 seconds

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();

        JsonDocument doc;

        DeserializationError error = deserializeJson(doc, payload);

        if (error)
        {
            Serial.print("JSON Error: ");
            Serial.println(error.c_str());

            http.end();
            return;
        }

        JsonArray flights = doc["response"];

        if (flights.size() == 0)
        {
            flightNo = "No Flight";
            airline = "--";
            departure = "--";
            destination = "--";
            altitude = "--";
            speed = "--";
            distance = "--";

            http.end();
            return;
        }

        float nearestDistance = 999999.0;
        JsonObject nearestFlight;

        for (JsonObject flight : flights)
        {
            float lat = flight["lat"] | 0.0;
            float lon = flight["lng"] | 0.0;

            float d = calculateDistance(
                MY_LAT,
                MY_LON,
                lat,
                lon);

            if (d < nearestDistance && d <= 60.0)
            {
                nearestDistance = d;
                nearestFlight = flight;
            }
        }

       if(nearestDistance == 999999)
{
    flightNo="No Flight";
    airline="";
    departure="";
    destination="";
    altitude="";
    speed="";
    distance="";

    http.end();
    return;
}

        flightNo = nearestFlight["flight_iata"] | "--";

        airline = getAirlineName(
            nearestFlight["airline_iata"] | "--"
        );

        departure = nearestFlight["dep_iata"] | "--";
        destination = nearestFlight["arr_iata"] | "--";

        int altFeet =
        (int)((nearestFlight["alt"] | 0)*3.28084);

        altitude = String(altFeet);

        speed = String((int)(nearestFlight["speed"]|0));

        distance = String(nearestDistance,1);

        Serial.println("Nearest Flight:");
        Serial.println(flightNo);
    }
    else
    {
        Serial.print("HTTP Error: ");
        Serial.println(httpCode);
    }

    http.end();
}

// -----------------------------------------------------
// Draw Flight Screen
// -----------------------------------------------------
void drawFlight(Adafruit_SSD1306 &display)
{
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(18, 0);
    display.println("Nearest Flight");

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0,18);
    display.print(flightNo);

    display.setCursor(55,18);
    display.print(airline);

    display.setCursor(0,30);
    display.print(departure);
    display.print("->");
    display.print(destination);

    display.setCursor(0,42);
    display.print(speed);
    display.print("km");

    display.setCursor(72,42);
    display.print(altitude);
    display.print("ft");

    display.setCursor(0,54);
    display.print(distance);
    display.print("km");
    display.display();
}