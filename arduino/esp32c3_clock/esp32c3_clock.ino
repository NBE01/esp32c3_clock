#include <U8g2lib.h>
#include <Wire.h>
#include <sys/time.h>
#include <time.h>

constexpr int kSdaPin = 6;  // XIAO D4
constexpr int kSclPin = 7;  // XIAO D5

// 0: SSD1306 128x64, 1: SH1106 128x64, 2: SSD1306 128x32
#define OLED_MODEL 0

// 0 automatically selects 0x3C or 0x3D; otherwise specify a 7-bit address.
constexpr unsigned char kOledAddress = 0;
constexpr const char *kTimezone = "JST-9";

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
char input[64];
size_t inputLength = 0;
bool inputOverflow = false;

time_t getCompileTime() {
  char monthText[4] = {};
  int day = 0;
  int year = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;

  if (sscanf(__DATE__, "%3s %d %d", monthText, &day, &year) != 3 ||
      sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second) != 3) {
    return 1704067200;  // 2024-01-01 fallback
  }

  static const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  const char *monthPosition = strstr(months, monthText);
  if (!monthPosition) return 1704067200;

  tm build = {};
  build.tm_year = year - 1900;
  build.tm_mon = (monthPosition - months) / 3;
  build.tm_mday = day;
  build.tm_hour = hour;
  build.tm_min = minute;
  build.tm_sec = second;
  build.tm_isdst = -1;

  return mktime(&build);
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
      checked.tm_mday != d || checked.tm_hour != h || checked.tm_min != min ||
      checked.tm_sec != s) {
    Serial.println("Invalid calendar date.");
    return;
  }

  timeval value = {epoch, 0};
  if (settimeofday(&value, nullptr) == 0) {
    buildTimeUsed = false;
    Serial.println("Clock set (JST).");
  } else {
    Serial.println("Clock update failed.");
  }
}

void readCommands() {
  while (Serial.available()) {
    const char c = Serial.read();

    if (c == '\r' || c == '\n') {
      if (inputOverflow) {
        Serial.println("Command too long; discarded.");
      } else if (inputLength) {
        input[inputLength] = '\0';
        handleCommand(input);
      }

      inputLength = 0;
      inputOverflow = false;
    } else if (!inputOverflow) {
      if (inputLength < sizeof(input) - 1) {
        input[inputLength++] = c;
      } else {
        inputOverflow = true;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  setenv("TZ", kTimezone, 1);
  tzset();

  // Arduino IDE has no PlatformIO build-time script, so use the compiler
  // timestamp as an initial estimate when the system clock is unset.
  if (time(nullptr) < 1704067200) {
    timeval initial = {getCompileTime(), 0};
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
          (!selected && (address == 0x3C || address == 0x3D))) {
        selected = address;
      }
    }
  }

  Serial.println("I2C scan complete.");

  if (selected) {
    oled.setI2CAddress(selected << 1);
    oled.setBusClock(100000);
    Serial.printf("OLED init start (model=%d, address=0x%02X).\n", OLED_MODEL, selected);

    Wire.end();
    Serial.println("Scanner I2C bus released.");

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
}

void loop() {
  static bool firstLoop = true;
  if (firstLoop) {
    Serial.println("Clock loop entered.");
    firstLoop = false;
  }

  readCommands();

  static time_t previous = 0;
  const time_t now = time(nullptr);

  if (now != previous) {
    previous = now;

    tm local = {};
    localtime_r(&now, &local);

    char date[16];
    char clock[16];
    strftime(date, sizeof(date), "%Y-%m-%d", &local);
    strftime(clock, sizeof(clock), "%H:%M:%S", &local);

    Serial.printf("%s %s JST%s\n",
                  date,
                  clock,
                  buildTimeUsed ? " (build time estimate)" : "");

    if (displayReady) {
      oled.clearBuffer();

      oled.setFont(u8g2_font_6x10_tf);
      oled.drawStr(0, 10, date);
      oled.drawStr(108, 10, "JST");

      oled.setFont(u8g2_font_logisoso16_tn);
      oled.drawStr((128 - oled.getStrWidth(clock)) / 2,
                   oled.getDisplayHeight() == 32 ? 31 : 39,
                   clock);

      if (oled.getDisplayHeight() >= 64) {
        oled.setFont(u8g2_font_6x10_tf);
        oled.drawStr(0, 60, buildTimeUsed ? "BUILD TIME / SET USB" : "INTERNAL CLOCK");
      }

      oled.sendBuffer();
    }
  }

  delay(10);
}
