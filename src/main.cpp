#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>
#include "config.h"
#include "wifi_config.h"

#if OLED_MODEL == 0
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, kSclPin, kSdaPin);
#elif OLED_MODEL == 1
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, kSclPin, kSdaPin);
#elif OLED_MODEL == 2
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE, kSclPin, kSdaPin);
#else
#error Unsupported OLED_MODEL
#endif

bool displayReady = false;
bool buildTimeUsed = false;
bool ntpTimeUsed = false;
enum class SyncState { Idle, Connecting, WaitingForNtp };
SyncState syncState = SyncState::Idle;
uint32_t lastSyncAttempt = 0;
uint32_t syncStageStarted = 0;
char input[64];
size_t inputLength = 0;
bool inputOverflow = false;

void startTimeSync() {
    lastSyncAttempt = millis();
    syncStageStarted = lastSyncAttempt;
    syncState = SyncState::Connecting;
    Serial.println("NTP: connecting to Wi-Fi.");
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.begin(kWifiSsid, kWifiPassword);
}

void finishTimeSync(const char *message) {
    // Stop SNTP's automatic polling; only our daily schedule starts it again.
    if (syncState == SyncState::WaitingForNtp) esp_sntp_stop();
    WiFi.disconnect(true);
    syncState = SyncState::Idle;
    Serial.println(message);
}

void updateTimeSync() {
    if (!kWifiSsid[0]) return;
    const uint32_t nowMs = millis();
    if (syncState == SyncState::Idle) {
        // Unsigned subtraction also works across the millis() rollover.
        if (static_cast<uint32_t>(nowMs - lastSyncAttempt) >= kNtpIntervalMs) {
            startTimeSync();
        }
    } else if (syncState == SyncState::Connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("NTP: requesting time from %s.\n", kNtpServer);
            sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
            sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
            sntp_set_sync_interval(kNtpIntervalMs);
            configTzTime(kTimezone, kNtpServer);
            syncStageStarted = millis();
            syncState = SyncState::WaitingForNtp;
        } else if (static_cast<uint32_t>(nowMs - syncStageStarted) >= kWifiTimeoutMs) {
            finishTimeSync("NTP: Wi-Fi timeout; keeping clock, next attempt in 24-hour cycle.");
        }
    } else {
        // Check actual SNTP completion, not whether the build-time clock is valid.
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            buildTimeUsed = false;
            ntpTimeUsed = true;
            finishTimeSync("NTP: synchronized (JST); Wi-Fi off until next daily attempt.");
        } else if (WiFi.status() != WL_CONNECTED) {
            finishTimeSync("NTP: Wi-Fi disconnected; keeping clock, next attempt in 24-hour cycle.");
        } else if (static_cast<uint32_t>(nowMs - syncStageStarted) >= kNtpTimeoutMs) {
            finishTimeSync("NTP: response timeout; keeping clock, next attempt in 24-hour cycle.");
        }
    }
}

void handleCommand(const char *line) {
    if (strcmp(line, "help") == 0) {
        Serial.println("Set JST: set YYYY-MM-DD HH:MM:SS (then Enter)");
        return;
    }
    int y, m, d, h, min, s;
    char extra;
    if (sscanf(line, "set %d-%d-%d %d:%d:%d %c", &y, &m, &d, &h, &min, &s, &extra) != 6 ||
        y < 2024 || y > 2037 || m < 1 || m > 12 || d < 1 || d > 31 ||
        h < 0 || h > 23 || min < 0 || min > 59 || s < 0 || s > 59) {
        Serial.println("Invalid command/date. Use: set YYYY-MM-DD HH:MM:SS (2024-2037, JST)");
        return;
    }
    tm requested = {};
    requested.tm_year = y - 1900;
    requested.tm_mon = m - 1;
    requested.tm_mday = d;
    requested.tm_hour = h;
    requested.tm_min = min;
    requested.tm_sec = s;
    requested.tm_isdst = -1;
    const time_t epoch = mktime(&requested);
    tm checked = {};
    localtime_r(&epoch, &checked);
    if (epoch < 0 || checked.tm_year != y - 1900 || checked.tm_mon != m - 1 ||
        checked.tm_mday != d || checked.tm_hour != h || checked.tm_min != min || checked.tm_sec != s) {
        Serial.println("Invalid calendar date.");
        return;
    }
    timeval value = {epoch, 0};
    if (settimeofday(&value, nullptr) == 0) {
        buildTimeUsed = false;
        ntpTimeUsed = false;
        Serial.println("Clock set (JST).");
    } else {
        Serial.println("Clock update failed.");
    }
}

void readCommands() {
    while (Serial.available()) {
        const char c = Serial.read();
        if (c == '\r' || c == '\n') {
            if (inputOverflow) Serial.println("Command too long; discarded.");
            else if (inputLength) {
                input[inputLength] = '\0';
                handleCommand(input);
            }
            inputLength = 0;
            inputOverflow = false;
        } else if (!inputOverflow) {
            if (inputLength < sizeof(input) - 1) input[inputLength++] = c;
            else inputOverflow = true;
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1500);
    setenv("TZ", kTimezone, 1);
    tzset();
    // Preserve an existing valid system clock, including after supported resets.
    if (time(nullptr) < 1704067200) {
        timeval initial = {static_cast<time_t>(BUILD_EPOCH), 0};
        settimeofday(&initial, nullptr);
        buildTimeUsed = true;
    }
    Serial.println("XIAO ESP32-C3 clock / JST. Type help for time setting.");
    Wire.begin(kSdaPin, kSclPin);
    Wire.setClock(100000);
    uint8_t selected = 0;
    for (uint8_t address = 1; address < 127; ++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.printf("I2C device: 0x%02X\n", address);
            if (kOledAddress ? address == kOledAddress :
                (!selected && (address == 0x3C || address == 0x3D))) selected = address;
        }
    }
    Serial.println("I2C scan complete.");
    if (selected) {
        oled.setI2CAddress(selected << 1);
        oled.setBusClock(100000);
        Serial.printf("OLED init start (model=%d, address=0x%02X).\n", OLED_MODEL, selected);
        // U8g2 configures the GPIOs before starting Wire. Release the scanner's
        // bus so Wire.begin() restores the hardware I2C pin configuration.
        Wire.end();
        Serial.println("Scanner I2C bus released.");
        // Same steps as oled.begin(), with logs to locate initialization stalls.
        oled.initDisplay();
        Serial.println("OLED init commands complete.");
        oled.clearDisplay();
        Serial.println("OLED clear complete.");
        oled.setPowerSave(0);
        Serial.println("OLED init complete.");
        displayReady = true;
    } else {
        Serial.println("OLED not found. Check power, GND, SDA=D4, SCL=D5; then reset.");
    }
    Serial.println("Setup complete; starting clock loop.");
    WiFi.persistent(false);
    if (kWifiSsid[0]) startTimeSync();
    else Serial.println("NTP disabled: set WIFI_SSID and WIFI_PASSWORD in .env, then upload.");
}

void loop() {
    static bool firstLoop = true;
    if (firstLoop) {
        Serial.println("Clock loop entered.");
        firstLoop = false;
    }
    readCommands();
    updateTimeSync();
    static time_t previous = 0;
    const time_t now = time(nullptr);
    if (now != previous) {
        previous = now;
        tm local = {};
        localtime_r(&now, &local);
        char date[16], clock[16];
        strftime(date, sizeof(date), "%Y-%m-%d", &local);
        strftime(clock, sizeof(clock), "%H:%M:%S", &local);
        Serial.printf("%s %s JST%s\n", date, clock, buildTimeUsed ? " (build time estimate)" : "");
        if (displayReady) {
            oled.clearBuffer();
            oled.setFont(u8g2_font_6x10_tf);
            oled.drawStr(0, 10, date);
            oled.drawStr(108, 10, "JST");
            oled.setFont(u8g2_font_logisoso16_tn);
            oled.drawStr((128 - oled.getStrWidth(clock)) / 2, oled.getDisplayHeight() == 32 ? 31 : 39, clock);
            if (oled.getDisplayHeight() >= 64) {
                oled.setFont(u8g2_font_6x10_tf);
                oled.drawStr(0, 60, buildTimeUsed ? "BUILD TIME / SET USB" :
                    (ntpTimeUsed ? "NTP / INTERNAL CLOCK" : "INTERNAL CLOCK"));
            }
            oled.sendBuffer();
        }
    }
    delay(10);
}
