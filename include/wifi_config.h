#pragma once

#include "config_env.h"  // Generated from .env by scripts/build_env.py.

constexpr const char *kWifiSsid = WIFI_SSID;
constexpr const char *kWifiPassword = WIFI_PASSWORD;
constexpr const char *kNtpServer = NTP_SERVER;

// Attempt at startup and once every 24 hours, including after a failed attempt.
constexpr unsigned long kNtpIntervalMs = 24UL * 60UL * 60UL * 1000UL;
constexpr unsigned long kWifiTimeoutMs = 30000UL;
constexpr unsigned long kNtpTimeoutMs = 15000UL;
