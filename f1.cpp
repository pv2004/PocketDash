#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>

#include "f1.h"
#include "display.h"

String f1RaceName = "Loading F1...";
String f1SessionName = "--";
String f1SessionDate = "--";

// Simple helper to convert UTC components into Unix Epoch
time_t my_utc_to_epoch(int year, int month, int day, int hour, int min, int sec) {
    int y = year;
    int m = month;
    if (m <= 2) {
        m += 12;
        y -= 1;
    }
    int d = day;
    int leapDays = (y / 4) - (y / 100) + (y / 400);
    int daysSince0000 = 365 * y + leapDays + (153 * m - 457) / 5 + d - 306;
    int daysSince1970 = daysSince0000 - 719468;
    time_t epoch = daysSince1970 * 86400ULL + hour * 3600ULL + min * 60ULL + sec;
    return epoch;
}

struct Session {
    String name;
    time_t epoch;
    String displayStr;
};

void checkAndSetNextSession(JsonObject race, time_t currentLocalEpoch) {
    Session sessions[6];
    int count = 0;
    
    // Helper lambda to parse and add a session
    auto addSession = [&](const char* jsonKey, const char* name) {
        if (race.containsKey(jsonKey)) {
            const char* dateStr = race[jsonKey]["date"];
            const char* timeStr = race[jsonKey]["time"];
            
            int year, month, day, hour, min, sec;
            sscanf(dateStr, "%d-%d-%d", &year, &month, &day);
            sscanf(timeStr, "%d:%d:%d", &hour, &min, &sec);
            
            // Add 19800 seconds (5.5 hours) to UTC epoch to get IST epoch
            time_t ist = my_utc_to_epoch(year, month, day, hour, min, sec) + 19800;
            
            struct tm *ptm = gmtime(&ist);
            const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            char buf[32];
            sprintf(buf, "%s %02d/%02d %02d:%02d", days[ptm->tm_wday], ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_hour, ptm->tm_min);
            
            sessions[count].name = name;
            sessions[count].epoch = ist;
            sessions[count].displayStr = String(buf);
            count++;
        }
    };
    
    addSession("FirstPractice", "FP1");
    addSession("SecondPractice", "FP2");
    addSession("ThirdPractice", "FP3");
    addSession("SprintQualifying", "Sprint Q");
    addSession("Sprint", "Sprint");
    addSession("Qualifying", "Quali");
    
    // The main race is directly at the root of the race object
    if (race.containsKey("date") && race.containsKey("time")) {
        const char* dateStr = race["date"];
        const char* timeStr = race["time"];
        int year, month, day, hour, min, sec;
        sscanf(dateStr, "%d-%d-%d", &year, &month, &day);
        sscanf(timeStr, "%d:%d:%d", &hour, &min, &sec);
        time_t ist = my_utc_to_epoch(year, month, day, hour, min, sec) + 19800;
        struct tm *ptm = gmtime(&ist);
        const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        char buf[32];
        sprintf(buf, "%s %02d/%02d %02d:%02d", days[ptm->tm_wday], ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_hour, ptm->tm_min);
        
        sessions[count].name = "Race";
        sessions[count].epoch = ist;
        sessions[count].displayStr = String(buf);
        count++;
    }
    
    // Bubble sort sessions chronologically
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (sessions[j].epoch > sessions[j+1].epoch) {
                Session temp = sessions[j];
                sessions[j] = sessions[j+1];
                sessions[j+1] = temp;
            }
        }
    }
    
    f1SessionName = "TBD";
    f1SessionDate = "--";
    
    // Find the first session that is in the future
    for (int i = 0; i < count; i++) {
        if (sessions[i].epoch > currentLocalEpoch) {
            f1SessionName = sessions[i].name;
            f1SessionDate = sessions[i].displayStr;
            break;
        }
    }
    
    // If all sessions are in the past, just show the last one (usually Race)
    if (f1SessionName == "TBD" && count > 0) {
        f1SessionName = sessions[count-1].name + " (Done)";
        f1SessionDate = sessions[count-1].displayStr;
    }
}

void fetchF1(time_t currentLocalEpoch) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    Serial.println("Fetching F1 Schedule...");
    
    if(!http.begin(client, "https://api.jolpi.ca/ergast/f1/current/next.json")) {
        Serial.println("F1 connection failed");
        return;
    }
    http.setTimeout(10000);

    int httpCode = http.GET();
    if(httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if(!error) {
            JsonObject race = doc["MRData"]["RaceTable"]["Races"][0];
            if (!race.isNull()) {
                f1RaceName = race["raceName"].as<String>();
                checkAndSetNextSession(race, currentLocalEpoch);
                Serial.println("F1 Loaded!");
            }
        } else {
            Serial.println(error.c_str());
        }
    } else {
        Serial.print("F1 Error: ");
        Serial.println(http.errorToString(httpCode));
    }
    http.end();
}

void drawF1(Adafruit_SSD1306 &display) {
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(30, 0);
    display.println("F1 Schedule");

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    // GP Name
    display.setTextSize(1);
    display.setCursor(0, 18);
    display.println(f1RaceName);

    // Next Session Info
    display.setCursor(0, 34);
    display.print("Next: ");
    display.println(f1SessionName);

    // Date & Time
    display.setCursor(0, 48);
    display.println(f1SessionDate);

    drawWiFiSignal(display);
    display.display();
}
