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
    int days = 0;
    for (int y = 1970; y < year; y++) {
        days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
    }
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
        daysInMonth[1] = 29;
    }
    for (int m = 0; m < month - 1; m++) {
        days += daysInMonth[m];
    }
    days += day - 1;
    
    return ((time_t)days * 86400) + (hour * 3600) + (min * 60) + sec;
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
    auto addSession = [&](const char* jsonKey, const char* name, int durationHours) {
        if (race.containsKey(jsonKey)) {
            const char* dateStr = race[jsonKey]["date"];
            const char* timeStr = race[jsonKey]["time"];
            
            int year, month, day, hour, min, sec;
            sscanf(dateStr, "%d-%d-%d", &year, &month, &day);
            sscanf(timeStr, "%d:%d:%d", &hour, &min, &sec);
            
            // Add 19800 seconds (5.5 hours) to UTC epoch to get IST epoch
            time_t ist = my_utc_to_epoch(year, month, day, hour, min, sec) + 19800;
            time_t ist_end = ist + (durationHours * 3600);
            
            struct tm *ptm = gmtime(&ist);
            int s_wday = ptm->tm_wday;
            int s_mday = ptm->tm_mday;
            int s_mon = ptm->tm_mon + 1;
            int s_hour = ptm->tm_hour;
            int s_min = ptm->tm_min;
            
            ptm = gmtime(&ist_end);
            int e_hour = ptm->tm_hour;
            int e_min = ptm->tm_min;
            
            const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            char buf[32];
            sprintf(buf, "%s %02d/%02d %02d:%02d-%02d:%02d", days[s_wday], s_mday, s_mon, s_hour, s_min, e_hour, e_min);
            
            sessions[count].name = name;
            sessions[count].epoch = ist;
            sessions[count].displayStr = String(buf);
            count++;
        }
    };
    
    addSession("FirstPractice", "FP1", 1);
    addSession("SecondPractice", "FP2", 1);
    addSession("ThirdPractice", "FP3", 1);
    addSession("SprintQualifying", "Sprint Q", 1);
    addSession("Sprint", "Sprint", 1);
    addSession("Qualifying", "Quali", 1);
    
    // The main race is directly at the root of the race object
    if (race.containsKey("date") && race.containsKey("time")) {
        const char* dateStr = race["date"];
        const char* timeStr = race["time"];
        int year, month, day, hour, min, sec;
        sscanf(dateStr, "%d-%d-%d", &year, &month, &day);
        sscanf(timeStr, "%d:%d:%d", &hour, &min, &sec);
        
        time_t ist = my_utc_to_epoch(year, month, day, hour, min, sec) + 19800;
        time_t ist_end = ist + (2 * 3600); // Race is usually 2 hours
        
        struct tm *ptm = gmtime(&ist);
        int s_wday = ptm->tm_wday;
        int s_mday = ptm->tm_mday;
        int s_mon = ptm->tm_mon + 1;
        int s_hour = ptm->tm_hour;
        int s_min = ptm->tm_min;
        
        ptm = gmtime(&ist_end);
        int e_hour = ptm->tm_hour;
        int e_min = ptm->tm_min;
        
        const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        char buf[32];
        sprintf(buf, "%s %02d/%02d %02d:%02d-%02d:%02d", days[s_wday], s_mday, s_mon, s_hour, s_min, e_hour, e_min);
        
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
