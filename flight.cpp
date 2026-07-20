#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <math.h>

#include "config.h"
#include "flight.h"

String flightNo = "--";
String aircraft = "--";
String altitude = "--";
String speed = "--";
String distance = "--";
String direction = "";
bool visible = false;

//--------------------------------------------------
// Aircraft Type
//--------------------------------------------------
String getAircraftName(String type)
{
    if(type=="A20N") return "A320neo";
    if(type=="A21N") return "A321neo";
    if(type=="A320") return "A320";
    if(type=="B38M") return "B737MAX8";
    if(type=="B738") return "B737-800";
    if(type=="B77W") return "B777-300";
    if(type=="AT76") return "ATR72";
    if(type=="AT72") return "ATR72";
    if(type=="E75L") return "E175";
    if(type=="C25A") return "Citation";

    return type;
}

//--------------------------------------------------
// Bearing -> Compass
//--------------------------------------------------
String getCompassDirection(float deg)
{
    if(deg >=337.5 || deg <22.5) return "N";
    if(deg <67.5) return "NE";
    if(deg <112.5) return "E";
    if(deg <157.5) return "SE";
    if(deg <202.5) return "S";
    if(deg <247.5) return "SW";
    if(deg <292.5) return "W";
    return "NW";
}

//--------------------------------------------------
// Visibility
//--------------------------------------------------
bool isVisible(float altitudeFt,float distanceKm)
{
    float altitudeMeters = altitudeFt * 0.3048;

    float horizon =
        3.57 * sqrt(altitudeMeters);

    return distanceKm <= horizon;
}

//--------------------------------------------------
// Fetch Flight
//--------------------------------------------------
void fetchFlight()
{
    if(WiFi.status()!=WL_CONNECTED)
    {
        Serial.println("WiFi not connected!");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    String url =
        "https://api.adsb.lol/v2/lat/" +
        String(MY_LAT,4) +
        "/lon/" +
        String(MY_LON,4) +
        "/dist/60";

    Serial.println("Fetching Flights...");
    Serial.println(url);

    if(!http.begin(client,url))
    {
        Serial.println("Connection Failed");
        return;
    }

    http.setTimeout(10000);

    int httpCode = http.GET();

    if(httpCode != HTTP_CODE_OK)
    {
        Serial.print("HTTP Error: ");
        Serial.println(http.errorToString(httpCode));
        http.end();
        return;
    }

    String payload = http.getString();

    JsonDocument doc;

    DeserializationError error =
        deserializeJson(doc,payload);

    if(error)
    {
        Serial.println(error.c_str());
        http.end();
        return;
    }

    JsonArray flights = doc["ac"];

    if(flights.size()==0)
    {
        flightNo="No Flight";
        aircraft="";
        altitude="";
        speed="";
        distance="";
        direction="";
        visible=false;

        http.end();
        return;
    }

    JsonObject bestVisible;
    JsonObject bestNearest;

    float visibleDist=99999;
    float nearestDist=99999;

        for (JsonObject f : flights)
    {
        String callsign = f["flight"] | "";
        callsign.trim();

        if (callsign == "")
            continue;

        // Ignore parked aircraft
        if (f["alt_baro"].is<const char*>())
        {
            String alt = f["alt_baro"].as<String>();
            if (alt == "ground")
                continue;
        }

        float gsKnots = f["gs"] | 0.0;

        // Ignore taxiing aircraft
        if (gsKnots < 50)
            continue;

        float dstKm = (f["dst"] | 0.0) * 1.852;

        float altFt = 0;

        if (f["alt_baro"].is<int>())
            altFt = f["alt_baro"];

        bool canSee = (dstKm <= 10.0);

        if (canSee && dstKm < visibleDist)
        {
            visibleDist = dstKm;
            bestVisible = f;
        }

        if (dstKm < nearestDist)
        {
            nearestDist = dstKm;
            bestNearest = f;
        }
    }

    JsonObject chosen;

    if (!bestVisible.isNull())
    {
        chosen = bestVisible;
        visible = true;
    }
    else if (!bestNearest.isNull())
    {
        chosen = bestNearest;
        visible = false;
    }
    else
    {
        flightNo = "No Flight";
        aircraft = "";
        altitude = "";
        speed = "";
        distance = "";
        direction = "";

        http.end();
        return;
    }

    String callsign = chosen["flight"] | "--";
    callsign.trim();
    flightNo = callsign;

    aircraft = getAircraftName(chosen["t"] | "--");

    int altFt = chosen["alt_baro"] | 0;
    altitude = String(altFt) + "ft";

    int speedKm = (int)((chosen["gs"] | 0.0) * 1.852);
    speed = String(speedKm) + "km/h";

    float dstKm = (chosen["dst"] | 0.0) * 1.852;
    distance = String(dstKm, 1) + "km";

    float bearing = chosen["dir"] | 0.0;
    direction =
        getCompassDirection(bearing) +
        " (" +
        String((int)bearing) +
        (char)247 + ")";

    Serial.println("Selected Aircraft");
    Serial.print("Callsign : ");
    Serial.println(flightNo);
    Serial.print("Type     : ");
    Serial.println(aircraft);
    Serial.print("Altitude : ");
    Serial.println(altitude);
    Serial.print("Speed    : ");
    Serial.println(speed);
    Serial.print("Distance : ");
    Serial.println(distance);

    http.end();
}

//--------------------------------------------------
// Draw OLED
//--------------------------------------------------
void drawFlight(Adafruit_SSD1306 &display)
{
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(28,0);
    display.println("Flight Radar");

    display.drawLine(0,10,127,10,SSD1306_WHITE);

    display.setCursor(0,18);
    display.println(flightNo);

    display.setCursor(70,18);
    display.println(aircraft);

    display.setCursor(0,32);
    display.print(altitude);

    display.setCursor(72,32);
    display.print(speed);

    display.setCursor(0,46);
    display.print(distance);

    if(visible)
    {
        display.setCursor(0,56);
        display.print("VISIBLE");      // simple arrow indicator
        display.print(" Look ");
        display.print(direction);
    }

    display.display();
}