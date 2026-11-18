// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 FerienBSD aka Marcel Sauder
// Einstellungen zur Laufzeit. Vorgabe sind die Werte aus secrets.h (oder die Standardwerte im Sketch).
// Gespeicherte Werte im Flash (NVS, Bereich "cfg") haben Vorrang vor den Vorgaben.
#pragma once
#include <Arduino.h>
#include <Preferences.h>

#define CFG_MAX_WIFI 5

#ifndef TZ_DEFAULT
#define TZ_DEFAULT "CET-1CEST,M3.5.0,M10.5.0/3"   // Zuerich: MEZ/MESZ mit Sommerzeit
#endif
#ifndef UNIT_NUMBER
#define UNIT_NUMBER 1                              // Nummer des Geraets, 1 bis 999 (Name: bandampel-001)
#endif
#ifndef MENU_PASSWORD
#define MENU_PASSWORD ""                           // leer = kein Passwort
#endif

struct Cfg {
  String  callsign;
  int     unit;                    // 1 bis 999
  String  menuPass;                // leer = kein Passwort
  String  tz;                      // POSIX-Zeitzone
  // WLAN-Liste
  String  wifiSsid[CFG_MAX_WIFI];
  String  wifiPass[CFG_MAX_WIFI];
  int     wifiCount;
  // Seiten
  bool    pOverview, pForecast, pModes, pLive, pDetails, pSun;
  bool    showSsb, showCw, ssbFirst;
  int     autoMs;
  // Helligkeit und Nachtmodus
  int     brightness, brightnessNight;
  int     ldrThreshold, ldrHyst;
  bool    ldrNightHigh;
  // Live-Seite
  String  liveFields;
  int     liveMinutes;
  int     liveFair[9], liveGood[9];
  // Standort fuer Sonnenzeiten
  float   sunLat, sunLon;
};

static Cfg cfg;

static const int CFG_LIVE_FAIR_DEF[9] = { 150, 1000, 8000, 6000, 12000, 3000, 3000, 800, 1500 };
static const int CFG_LIVE_GOOD_DEF[9] = { 500, 3000, 25000, 18000, 35000, 9000, 9000, 2500, 4500 };

static void cfgDefaults() {
  cfg.callsign = CALLSIGN;
  cfg.unit = UNIT_NUMBER;
  cfg.menuPass = MENU_PASSWORD;
  cfg.tz = TZ_DEFAULT;
  cfg.wifiCount = 0;
  for (int i = 0; i < WIFI_COUNT && i < CFG_MAX_WIFI; i++) {
    cfg.wifiSsid[i] = WIFI_LIST[i].ssid;
    cfg.wifiPass[i] = WIFI_LIST[i].pass;
    cfg.wifiCount++;
  }
  cfg.pOverview = PAGE_OVERVIEW; cfg.pForecast = PAGE_FORECAST; cfg.pModes = PAGE_MODES;
  cfg.pLive = PAGE_LIVE;         cfg.pDetails = PAGE_DETAILS;   cfg.pSun = PAGE_SUN;
  cfg.showSsb = SHOW_SSB; cfg.showCw = SHOW_CW; cfg.ssbFirst = SSB_FIRST; 
  cfg.autoMs = AUTO_PAGE_MS;
  cfg.brightness = 180; cfg.brightnessNight = BRIGHTNESS_NIGHT;
  cfg.ldrThreshold = LDR_THRESHOLD; cfg.ldrHyst = LDR_HYST; cfg.ldrNightHigh = LDR_NIGHT_IS_HIGH;
  cfg.liveFields = LIVE_FIELDS; cfg.liveMinutes = LIVE_MINUTES;
  for (int i = 0; i < 9; i++) { cfg.liveFair[i] = CFG_LIVE_FAIR_DEF[i]; cfg.liveGood[i] = CFG_LIVE_GOOD_DEF[i]; }
  cfg.sunLat = SUN_LAT; cfg.sunLon = SUN_LON;
}

// Werte auf sinnvolle Bereiche begrenzen (auch gegen kaputte Eingaben aus dem Menue)
static void cfgSanitize() {
  cfg.unit = constrain(cfg.unit, 1, 999);
  cfg.autoMs = constrain(cfg.autoMs, 2000, 600000);
  cfg.brightness = constrain(cfg.brightness, 10, 255);
  cfg.brightnessNight = constrain(cfg.brightnessNight, 0, 255);
  cfg.ldrHyst = constrain(cfg.ldrHyst, 10, 2000);
  cfg.ldrThreshold = constrain(cfg.ldrThreshold, -1, 4095);
  cfg.liveMinutes = constrain(cfg.liveMinutes, 5, 120);
  cfg.sunLat = constrain(cfg.sunLat, -66.0f, 66.0f);
  cfg.sunLon = constrain(cfg.sunLon, -180.0f, 180.0f);
  if (cfg.tz.length() < 3) cfg.tz = TZ_DEFAULT;
  if (cfg.callsign.length() > 12) cfg.callsign = cfg.callsign.substring(0, 12);
  // Live-Felder: nur Buchstaben, Hochkomma und Komma (geht in die Datenbank-Abfrage)
  String f;
  for (unsigned i = 0; i < cfg.liveFields.length(); i++) {
    char c = cfg.liveFields[i];
    if (isAlpha(c) || c == '\'' || c == ',') f += (char)toupper(c);
  }
  cfg.liveFields = f.length() ? f : String(LIVE_FIELDS);
}

static String cfgHostname() {
  char b[20];
  snprintf(b, sizeof(b), "bandampel-%03d", cfg.unit);
  return String(b);
}

static void cfgLoad() {
  cfgDefaults();
  Preferences p;
  p.begin("cfg", true);
  cfg.callsign = p.getString("call", cfg.callsign);
  cfg.unit = p.getInt("unit", cfg.unit);
  cfg.menuPass = p.getString("pass", cfg.menuPass);
  cfg.tz = p.getString("tz", cfg.tz);
  if (p.isKey("wcount")) {
    cfg.wifiCount = constrain(p.getInt("wcount", 0), 0, CFG_MAX_WIFI);
    for (int i = 0; i < cfg.wifiCount; i++) {
      cfg.wifiSsid[i] = p.getString((String("ws") + i).c_str(), "");
      cfg.wifiPass[i] = p.getString((String("wp") + i).c_str(), "");
    }
  }
  cfg.pOverview = p.getBool("pOv", cfg.pOverview);   cfg.pForecast = p.getBool("pFc", cfg.pForecast);
  cfg.pModes = p.getBool("pMo", cfg.pModes);         cfg.pLive = p.getBool("pLi", cfg.pLive);
  cfg.pDetails = p.getBool("pDe", cfg.pDetails);
  cfg.showSsb = p.getBool("ssb", cfg.showSsb);       cfg.showCw = p.getBool("cw", cfg.showCw);
  cfg.ssbFirst = p.getBool("ssbf", cfg.ssbFirst);    cfg.pSun = p.getBool("pSu", cfg.pSun);
  cfg.autoMs = p.getInt("automs", cfg.autoMs);
  cfg.brightness = p.getInt("br", cfg.brightness);   cfg.brightnessNight = p.getInt("brn", cfg.brightnessNight);
  cfg.ldrThreshold = p.getInt("ldrt", cfg.ldrThreshold); cfg.ldrHyst = p.getInt("ldrh", cfg.ldrHyst);
  cfg.ldrNightHigh = p.getBool("ldrn", cfg.ldrNightHigh);
  cfg.liveFields = p.getString("lf", cfg.liveFields); cfg.liveMinutes = p.getInt("lmin", cfg.liveMinutes);
  for (int i = 0; i < 9; i++) {
    cfg.liveFair[i] = p.getInt((String("lfa") + i).c_str(), cfg.liveFair[i]);
    cfg.liveGood[i] = p.getInt((String("lgo") + i).c_str(), cfg.liveGood[i]);
  }
  cfg.sunLat = p.getFloat("lat", cfg.sunLat); cfg.sunLon = p.getFloat("lon", cfg.sunLon);
  p.end();
  cfgSanitize();
}

static void cfgSave() {
  cfgSanitize();
  Preferences p;
  p.begin("cfg", false);
  p.putString("call", cfg.callsign); p.putInt("unit", cfg.unit);
  p.putString("pass", cfg.menuPass); p.putString("tz", cfg.tz);
  p.putInt("wcount", cfg.wifiCount);
  for (int i = 0; i < cfg.wifiCount; i++) {
    p.putString((String("ws") + i).c_str(), cfg.wifiSsid[i]);
    p.putString((String("wp") + i).c_str(), cfg.wifiPass[i]);
  }
  p.putBool("pOv", cfg.pOverview); p.putBool("pFc", cfg.pForecast); p.putBool("pMo", cfg.pModes);
  p.putBool("pLi", cfg.pLive);     p.putBool("pDe", cfg.pDetails);
  p.putBool("ssb", cfg.showSsb);   p.putBool("cw", cfg.showCw);
  p.putBool("ssbf", cfg.ssbFirst); p.putBool("pSu", cfg.pSun);
  p.putInt("automs", cfg.autoMs);
  p.putInt("br", cfg.brightness);  p.putInt("brn", cfg.brightnessNight);
  p.putInt("ldrt", cfg.ldrThreshold); p.putInt("ldrh", cfg.ldrHyst); p.putBool("ldrn", cfg.ldrNightHigh);
  p.putString("lf", cfg.liveFields); p.putInt("lmin", cfg.liveMinutes);
  for (int i = 0; i < 9; i++) {
    p.putInt((String("lfa") + i).c_str(), cfg.liveFair[i]);
    p.putInt((String("lgo") + i).c_str(), cfg.liveGood[i]);
  }
  p.putFloat("lat", cfg.sunLat); p.putFloat("lon", cfg.sunLon);
  p.end();
}

// Werkseinstellung: gespeicherte Werte loeschen, danach gelten wieder die Vorgaben aus secrets.h
static void cfgFactoryReset() {
  Preferences p;
  p.begin("cfg", false);
  p.clear();
  p.end();
  cfgLoad();
}
