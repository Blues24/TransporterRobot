#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

// ============================================================
//  SETUP
//  @param ipAddress  IP dari WiFi.softAPIP().toString()
// ============================================================
bool displaySetup(const String& ipAddress);

// ============================================================
//  LAYAR
// ============================================================
void showIP(const String& ipAddress);   // default boot screen — tampilkan IP
void showLogo();                         // logo static (SELECT)
void showBattStatus(int battLevel, const String& battMessage);  // PS button
void showEStop(bool active);             // Emergency Stop overlay

// ============================================================
//  BITMAP CUSTOM (upload via WebServer)
// ============================================================
bool saveBitmapToPrefs(const String& base64data);
void loadCustomBitmap();

// ============================================================
//  CONFIG PERSISTENCE
// ============================================================
void loadDisplayConfig();
void saveDisplayConfig();

#endif // DISPLAY_OLED_H