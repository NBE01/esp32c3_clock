#pragma once

constexpr int kSdaPin = 6;  // XIAO D4
constexpr int kSclPin = 7;  // XIAO D5
// 0: SSD1306 128x64, 1: SH1106 128x64, 2: SSD1306 128x32
#define OLED_MODEL 0
// 0 automatically selects 0x3C or 0x3D; otherwise specify a 7-bit address.
constexpr unsigned char kOledAddress = 0;
constexpr const char *kTimezone = "JST-9";
