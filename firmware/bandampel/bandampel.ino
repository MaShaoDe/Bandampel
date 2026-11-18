// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 FerienBSD aka Marcel Sauder
//
// Bandampel V1: HF-Ausbreitung als Farbampel auf dem CYD (ESP32-2432S028)
// Datenquelle: HamQSL (N0NBH), Abruf hoechstens alle 30 Minuten.
// Seiten: 0 Uebersicht, 1 Baender, 2 Details. Umschalten per Touch oder BOOT-Taste.

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include "lgfx_cyd.h"
#include "secrets.h"

#ifndef CALLSIGN
#define CALLSIGN ""   // optional in secrets.h: #define CALLSIGN "HB9XXX"
#endif

// ---------------- Einstellungen ----------------
static const int      ROTATION       = 1;          // 1 oder 3 (180 Grad gedreht)
static const uint8_t  BRIGHTNESS     = 180;        // 0 bis 255
static const uint32_t FETCH_OK_MS    = 30UL * 60UL * 1000UL;
static const uint32_t FETCH_RETRY_MS = 2UL * 60UL * 1000UL;
static const char*    FEED_URL       = "https://www.hamqsl.com/solarxml.php";
static const int      BTN_PIN       = 0;
static const char*    TZ_ZURICH      = "CET-1CEST,M3.5.0,M10.5.0/3";   // MEZ/MESZ mit Sommerzeit

// Nachtmodus per Lichtsensor (CYD: GPIO 34). Einstellungen koennen in secrets.h ueberschrieben werden.
// LDR_THRESHOLD = -1: aus (nur Messwerte auf der seriellen Schnittstelle).
#ifndef LDR_THRESHOLD
#define LDR_THRESHOLD -1
#endif
#ifndef LDR_NIGHT_IS_HIGH
#define LDR_NIGHT_IS_HIGH true     // true: grosser Messwert = dunkel
#endif
#ifndef BRIGHTNESS_NIGHT
#define BRIGHTNESS_NIGHT 25        // 0 bis 255
#endif
static const int LDR_PIN  = 34;
static const int LDR_HYST = 150;
static const int      PAGES          = 4;

// ---------------- Typen und Zustand ----------------
enum Cond { C_UNK, C_POOR, C_FAIR, C_GOOD };

struct Solar {
  String sfi, a, k, ssn, xray, wind, bz, noise, updated;
};

static LGFX_CYD tft;
static WiFiMulti wifiMulti;

static Solar solar;
static bool  haveData = false;
static const char* BAND_NAMES[4] = { "80m-40m", "30m-20m", "17m-15m", "12m-10m" };
static Cond bandDay[4]   = { C_UNK, C_UNK, C_UNK, C_UNK };
static Cond bandNight[4] = { C_UNK, C_UNK, C_UNK, C_UNK };

static int      page        = 0;
static String   statusMsg   = "Start";
static uint32_t nextFetch   = 0;
static bool     fetchedOnce = false;

// ---------------- Farben ----------------
static uint16_t COL_BG, COL_HDR, COL_GREEN, COL_YELLOW, COL_RED, COL_GREY, COL_TEXT, COL_DIM;

static void initColors() {
  COL_BG     = tft.color565(10, 14, 24);
  COL_HDR    = tft.color565(24, 32, 52);
  COL_GREEN  = tft.color565(0, 170, 70);
  COL_YELLOW = tft.color565(240, 190, 0);
  COL_RED    = tft.color565(200, 40, 40);
  COL_GREY   = tft.color565(70, 74, 84);
  COL_TEXT   = tft.color565(235, 238, 245);
  COL_DIM    = tft.color565(140, 148, 165);
}

static uint16_t colFor(Cond c) {
  switch (c) {
    case C_GOOD: return COL_GREEN;
    case C_FAIR: return COL_YELLOW;
    case C_POOR: return COL_RED;
    default:     return COL_GREY;
  }
}

// Schwellen (Vorschlag, noch zu pruefen)
static Cond levelSfi(int v) { return v >= 120 ? C_GOOD : (v >= 90 ? C_FAIR : C_POOR); }
static Cond levelK(int v)   { return v <= 2 ? C_GOOD : (v == 3 ? C_FAIR : C_POOR); }
static Cond levelA(int v)   { return v < 10 ? C_GOOD : (v < 30 ? C_FAIR : C_POOR); }
static Cond levelSsn(int v) { return v >= 80 ? C_GOOD : (v >= 40 ? C_FAIR : C_POOR); }

// ---------------- XML ----------------
static String tagValue(const String& xml, const char* tag) {
  String open  = String("<") + tag + ">";
  String close = String("</") + tag + ">";
  int s = xml.indexOf(open);
  if (s < 0) return "";
  s += open.length();
  int e = xml.indexOf(close, s);
  if (e < 0) return "";
  String v = xml.substring(s, e);
  v.trim();
  return v;
}

static Cond condFor(const String& xml, const char* name, const char* tod) {
  String key = String("<band name=\"") + name + "\" time=\"" + tod + "\">";
  int s = xml.indexOf(key);
  if (s < 0) return C_UNK;
  s += key.length();
  int e = xml.indexOf("</band>", s);
  if (e < 0) return C_UNK;
  String v = xml.substring(s, e);
  v.trim();
  v.toLowerCase();
  if (v == "good") return C_GOOD;
  if (v == "fair") return C_FAIR;
  if (v == "poor") return C_POOR;
  return C_UNK;
}

static bool fetchSolar() {
  WiFiClientSecure client;
  client.setInsecure();   // offen: Zertifikatspruefung
  HTTPClient http;
  http.setTimeout(10000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, FEED_URL)) return false;
  int code = http.GET();
  if (code != 200) {
    Serial.printf("HamQSL HTTP %d\n", code);
    http.end();
    return false;
  }
  String xml = http.getString();
  http.end();
  Serial.printf("HamQSL %u Bytes\n", (unsigned)xml.length());

  Solar s;
  s.sfi     = tagValue(xml, "solarflux");
  s.a       = tagValue(xml, "aindex");
  s.k       = tagValue(xml, "kindex");
  s.ssn     = tagValue(xml, "sunspots");
  s.xray    = tagValue(xml, "xray");
  s.wind    = tagValue(xml, "solarwind");
  s.bz      = tagValue(xml, "magneticfield");
  s.noise   = tagValue(xml, "signalnoise");
  s.updated = tagValue(xml, "updated");
  if (s.sfi.length() == 0 && s.k.length() == 0) return false;   // Format unerwartet

  solar = s;
  for (int i = 0; i < 4; i++) {
    bandDay[i]   = condFor(xml, BAND_NAMES[i], "day");
    bandNight[i] = condFor(xml, BAND_NAMES[i], "night");
  }
  haveData = true;
  return true;
}

// ---------------- Zeichnen ----------------
static void drawClock() {
  tft.fillRect(160, 0, 160, 26, COL_HDR);
  struct tm t;
  char loc[8] = "--:--";
  char utc[10] = "";
  if (getLocalTime(&t, 0) && t.tm_year > 120) {
    strftime(loc, sizeof(loc), "%H:%M", &t);          // Zuerich (MEZ/MESZ)
    time_t now = time(nullptr);
    struct tm u;
    gmtime_r(&now, &u);
    strftime(utc, sizeof(utc), "%H:%MZ", &u);          // UTC fuer den Funk
  }
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_HDR);
  int wu = tft.textWidth(utc);
  tft.drawString(utc, 314 - wu, 7);
  tft.setFont(&lgfx::fonts::Font4);
  tft.setTextColor(COL_TEXT, COL_HDR);
  int wl = tft.textWidth(loc);
  tft.drawString(loc, 314 - wu - (wu ? 8 : 0) - wl, 3);
}

static void drawHeader() {
  tft.fillRect(0, 0, 320, 26, COL_HDR);
  tft.setFont(&lgfx::fonts::Font4);
  tft.setTextColor(COL_TEXT, COL_HDR);
  tft.drawString(strlen(CALLSIGN) ? CALLSIGN : "BANDAMPEL", 6, 3);
  drawClock();
}

static void drawFooter() {
  tft.fillRect(0, 220, 320, 20, COL_BG);
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  String left = haveData ? solar.updated : statusMsg;
  if (left.length() > 26) left = left.substring(0, 26);
  tft.drawString(left, 6, 222);
  bool ok = (WiFi.status() == WL_CONNECTED);
  tft.fillCircle(262, 230, 5, ok ? COL_GREEN : COL_RED);
  char pg[8];
  snprintf(pg, sizeof(pg), "%d/%d", page + 1, PAGES);
  int w = tft.textWidth(pg);
  tft.drawString(pg, 314 - w, 222);
}

// Symbole: 1 Sonne, 2 Sonnenflecken, 3 Pegel, 4 Welle, 5 Mond
static void drawIcon(int type, int cx, int cy, uint16_t col) {
  if (type == 1) {
    tft.fillCircle(cx, cy, 5, col);
    for (int k = 0; k < 8; k++) {
      float a = k * 0.7853982f;
      tft.drawLine(cx + (int)(8 * cosf(a)), cy + (int)(8 * sinf(a)),
                   cx + (int)(12 * cosf(a)), cy + (int)(12 * sinf(a)), col);
    }
  } else if (type == 2) {
    tft.drawCircle(cx, cy, 10, col);
    tft.fillCircle(cx - 3, cy - 2, 2, col);
    tft.fillCircle(cx + 4, cy + 3, 3, col);
  } else if (type == 3) {
    tft.fillRect(cx - 9, cy + 1, 5, 6, col);
    tft.fillRect(cx - 2, cy - 3, 5, 10, col);
    tft.fillRect(cx + 5, cy - 7, 5, 14, col);
  } else if (type == 4) {
    int px = cx - 12, py = cy;
    for (int k = 1; k <= 20; k++) {
      int nx = cx - 12 + k * 24 / 20;
      int ny = cy + (int)(6 * sinf(k * 0.3141593f * 1.5f));
      tft.drawLine(px, py, nx, ny, col);
      px = nx; py = ny;
    }
  } else if (type == 5) {
    tft.fillCircle(cx, cy, 7, col);
    tft.fillCircle(cx + 4, cy - 3, 6, COL_BG);
  }
}

// Filigrane Kachel: dunkle Karte, Farbe nur im Rahmen, im Symbol und im Wert
static void drawTile(int x, int y, int w, int h, const char* label, const String& val, Cond c, int icon) {
  uint16_t col = colFor(c);
  int ch = h - 4;
  tft.fillRoundRect(x, y, w, ch, 8, COL_HDR);
  tft.drawRoundRect(x, y, w, ch, 8, col);
  tft.drawRoundRect(x + 1, y + 1, w - 2, ch - 2, 7, col);
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_HDR);
  tft.drawString(label, x + 8, y + 6);
  drawIcon(icon, x + w - 24, y + 20, col);
  tft.setFont(&lgfx::fonts::Font7);
  tft.setTextColor(col, COL_HDR);
  String v = val.length() ? val : String("--");
  int tw = tft.textWidth(v);
  tft.drawString(v, x + (w - tw) / 2, y + h - tft.fontHeight() - 12);
}

static void drawPageOverview() {
  bool d = haveData;
  drawTile(4,   28,  156, 90, "SFI (Solar Flux)", solar.sfi, d && solar.sfi.length() ? levelSfi(solar.sfi.toInt()) : C_UNK, 1);
  drawTile(160, 28,  156, 90, "K-Index",          solar.k,   d && solar.k.length()   ? levelK(solar.k.toInt())     : C_UNK, 3);
  drawTile(4,   122, 156, 90, "Sunspots",         solar.ssn, d && solar.ssn.length() ? levelSsn(solar.ssn.toInt()) : C_UNK, 2);
  drawTile(160, 122, 156, 90, "A-Index",          solar.a,   d && solar.a.length()   ? levelA(solar.a.toInt())     : C_UNK, 4);
}

// 9 Baender in 4 HamQSL-Gruppen. 160m liefert HamQSL nicht: Schaetzung (Vorschlag), hohl gezeichnet.
static const char* BAND9_LABELS[9] = { "160", "80", "40", "30", "20", "17", "15", "12", "10" };
static const int   BAND9_GROUP[9]  = { -1, 0, 0, 1, 1, 2, 2, 3, 3 };   // Index in bandDay/bandNight, -1 = Schaetzung

static Cond stepDown(Cond c) { return c == C_GOOD ? C_FAIR : (c == C_FAIR ? C_POOR : c); }
static Cond est160Day()   { return C_POOR; }
static Cond est160Night() {
  if (solar.k.toInt() >= 4) return C_POOR;
  return stepDown(bandNight[0]);   // eine Stufe schlechter als 80m-40m Nacht
}

static void drawBand(int x, int y, int w, int h, Cond c, bool estimated) {
  uint16_t col = colFor(c);
  if (estimated) {
    tft.fillRoundRect(x, y, w, h, 4, COL_BG);
    tft.drawRoundRect(x, y, w, h, 4, col);
    tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 3, col);
  } else {
    tft.fillRoundRect(x, y, w, h, 4, col);
  }
}

static void drawPageBands() {
  const int X0 = 40, CW = 30, BW = 24;
  const int YD = 52, YN = 106, BH = 46;
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  for (int i = 0; i < 9; i++) {
    int cx = X0 + i * CW + CW / 2;
    int tw = tft.textWidth(BAND9_LABELS[i]);
    tft.drawString(BAND9_LABELS[i], cx - tw / 2, 30);
    int g = BAND9_GROUP[i];
    bool est = (g < 0);
    Cond d = !haveData ? C_UNK : (est ? est160Day()   : bandDay[g]);
    Cond n = !haveData ? C_UNK : (est ? est160Night() : bandNight[g]);
    drawBand(cx - BW / 2, YD, BW, BH, d, est);
    drawBand(cx - BW / 2, YN, BW, BH, n, est);
  }
  drawIcon(1, 18, YD + BH / 2, COL_YELLOW);
  drawIcon(5, 18, YN + BH / 2, COL_DIM);

  // Kennwerte darunter
  tft.drawFastHLine(8, 162, 304, COL_GREY);
  const char* lab[3] = { "SFI", "K", "A" };
  String val[3] = { solar.sfi, solar.k, solar.a };
  Cond cc[3] = {
    haveData && solar.sfi.length() ? levelSfi(solar.sfi.toInt()) : C_UNK,
    haveData && solar.k.length()   ? levelK(solar.k.toInt())     : C_UNK,
    haveData && solar.a.length()   ? levelA(solar.a.toInt())     : C_UNK };
  for (int i = 0; i < 3; i++) {
    int x = 8 + i * 104;
    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(COL_DIM, COL_BG);
    tft.drawString(lab[i], x, 178);
    int lw = tft.textWidth(lab[i]);
    tft.setFont(&lgfx::fonts::Font4);
    tft.setTextColor(colFor(cc[i]), COL_BG);
    tft.drawString(val[i].length() ? val[i] : String("--"), x + lw + 8, 172);
  }
}

// Betriebsart (Vorschlag, noch zu pruefen): HamQSL kennt keine Betriebsarten.
// CW und Digital (FT8) kommen mit ca. 10 bis 15 dB weniger Signal aus als SSB, daher eine Stufe besser.
// Bei K >= 5 (gestoert) fuer beide eine Stufe schlechter.
static Cond stepUp(Cond c)      { return c == C_POOR ? C_FAIR : (c == C_FAIR ? C_GOOD : c); }
static Cond modeCw(Cond c)      { return c == C_UNK ? c : stepUp(c); }
static Cond disturbed(Cond c)   { return (c != C_UNK && solar.k.toInt() >= 5) ? stepDown(c) : c; }

static void drawPageModes() {
  const int X0 = 56, CW = 28, BW = 22, BH = 26;
  const int Y_CW_D = 48, Y_CW_N = 78, Y_SSB_D = 118, Y_SSB_N = 148;
  tft.setFont(&lgfx::fonts::Font2);
  for (int i = 0; i < 9; i++) {
    int cx = X0 + i * CW + CW / 2;
    tft.setTextColor(COL_DIM, COL_BG);
    int tw = tft.textWidth(BAND9_LABELS[i]);
    tft.drawString(BAND9_LABELS[i], cx - tw / 2, 28);
    int g = BAND9_GROUP[i];
    bool est = (g < 0);
    Cond d = !haveData ? C_UNK : disturbed(est ? est160Day()   : bandDay[g]);
    Cond n = !haveData ? C_UNK : disturbed(est ? est160Night() : bandNight[g]);
    drawBand(cx - BW / 2, Y_CW_D,  BW, BH, modeCw(d), est);
    drawBand(cx - BW / 2, Y_CW_N,  BW, BH, modeCw(n), est);
    drawBand(cx - BW / 2, Y_SSB_D, BW, BH, d, est);
    drawBand(cx - BW / 2, Y_SSB_N, BW, BH, n, est);
  }
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("CW",  2, Y_CW_D  + 28 - tft.fontHeight() / 2);
  tft.drawString("SSB", 2, Y_SSB_D + 28 - tft.fontHeight() / 2);
  drawIcon(1, 42, Y_CW_D  + BH / 2, COL_YELLOW);
  drawIcon(5, 42, Y_CW_N  + BH / 2, COL_DIM);
  drawIcon(1, 42, Y_SSB_D + BH / 2, COL_YELLOW);
  drawIcon(5, 42, Y_SSB_N + BH / 2, COL_DIM);
  tft.drawFastHLine(8, 108, 304, COL_GREY);
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString("CW inkl. Digital (FT8)", 8, 184);
  tft.drawString("Faustregel, keine Prognose", 8, 200);
}

static void drawRow(int y, const char* label, const String& val, const char* unit) {
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString(label, 10, y + 6);
  tft.setFont(&lgfx::fonts::Font4);
  tft.setTextColor(COL_TEXT, COL_BG);
  String v = (val.length() ? val : String("--")) + " " + unit;
  int w = tft.textWidth(v);
  tft.drawString(v, 310 - w, y);
}

static void drawPageDetails() {
  drawRow(40,  "X-Ray",             solar.xray,  "");
  drawRow(80,  "Sonnenwind",        solar.wind,  "km/s");
  drawRow(120, "Magnetfeld Bz",     solar.bz,    "nT");
  drawRow(160, "Signal-Rauschen",   solar.noise, "");
}

static void drawAll() {
  tft.fillScreen(COL_BG);
  drawHeader();
  if (page == 0)      drawPageOverview();
  else if (page == 1) drawPageBands();
  else if (page == 2) drawPageModes();
  else                drawPageDetails();
  drawFooter();
}

// ---------------- Eingabe ----------------
static bool pageSwitchRequested() {
  static uint32_t lockUntil = 0;
  uint32_t now = millis();
  if ((int32_t)(now - lockUntil) < 0) return false;
  int32_t tx, ty;
  bool pressed = (digitalRead(BTN_PIN) == LOW) || tft.getTouch(&tx, &ty);
  if (pressed) {
    lockUntil = now + 350;
    return true;
  }
  return false;
}

// ---------------- Nachtmodus ----------------
static uint8_t curBrightness = BRIGHTNESS;

static void updateNight() {
  static uint32_t lastRead = 0, lastPrint = 0;
  static bool night = false;
  uint32_t now = millis();
  if (now - lastRead < 1000) return;
  lastRead = now;
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(LDR_PIN);
  int v = (int)(sum / 8);
  if (LDR_THRESHOLD >= 0) {
    int dark = LDR_NIGHT_IS_HIGH ? (v - LDR_THRESHOLD) : (LDR_THRESHOLD - v);   // positiv = dunkler als Schwelle
    if (!night && dark > LDR_HYST) night = true;
    else if (night && dark < -LDR_HYST) night = false;
    uint8_t target = night ? BRIGHTNESS_NIGHT : BRIGHTNESS;
    if (target != curBrightness) {
      curBrightness = target;
      tft.setBrightness(target);
    }
  }
  if (now - lastPrint > 5000) {
    lastPrint = now;
    Serial.printf("LDR roh: %d  Nacht: %s\n", v, night ? "ja" : "nein");
  }
}

// ---------------- Setup und Loop ----------------
void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);
  tft.init();
  tft.setRotation(ROTATION);
  tft.setBrightness(BRIGHTNESS);
  initColors();
  statusMsg = "WLAN suchen...";
  drawAll();

  WiFi.mode(WIFI_STA);
  for (int i = 0; i < WIFI_COUNT; i++) {
    wifiMulti.addAP(WIFI_LIST[i].ssid, WIFI_LIST[i].pass);
    Serial.printf("Konfiguriert: [%s] (Passwort %u Zeichen)\n", WIFI_LIST[i].ssid, (unsigned)strlen(WIFI_LIST[i].pass));
  }
  int n = WiFi.scanNetworks();
  Serial.printf("WLAN-Scan: %d Netze (nur 2,4 GHz sichtbar)\n", n);
  for (int i = 0; i < n; i++) {
    Serial.printf("  [%s]  RSSI %d  Kanal %d\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
  }
  configTzTime(TZ_ZURICH, "pool.ntp.org", "time.nist.gov");
}

void loop() {
  static uint32_t lastWifiTry = 0;
  static int      lastMin     = -1;
  static bool     lastWifiOk  = false;
  uint32_t now = millis();

  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  if (!wifiOk && (now - lastWifiTry > 5000 || lastWifiTry == 0)) {
    lastWifiTry = now ? now : 1;
    statusMsg = "WLAN suchen...";
    drawFooter();
    wl_status_t r = (wl_status_t)wifiMulti.run(20000);   // 20 s Zeit, iPhone-Hotspot ist langsam
    lastWifiTry = millis() ? millis() : 1;
    wifiOk = (WiFi.status() == WL_CONNECTED);
    Serial.printf("WLAN-Versuch: Status %d (0 = laeuft noch, 1 = Netz nicht gefunden, 3 = verbunden, 4 = fehlgeschlagen, 6 = getrennt)\n", (int)r);
    if (wifiOk) Serial.printf("Verbunden mit [%s], IP %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  }
  if (wifiOk != lastWifiOk) {
    lastWifiOk = wifiOk;
    if (wifiOk) statusMsg = "WLAN " + WiFi.SSID();
    drawFooter();
  }

  if (wifiOk && (!fetchedOnce || (int32_t)(now - nextFetch) >= 0)) {
    fetchedOnce = true;
    statusMsg = "Lade Daten...";
    drawFooter();
    bool ok = fetchSolar();
    nextFetch = millis() + (ok ? FETCH_OK_MS : FETCH_RETRY_MS);
    if (!ok) statusMsg = "Abruf fehlgeschlagen";
    drawAll();
  }

  if (pageSwitchRequested()) {
    page = (page + 1) % PAGES;
    drawAll();
  }

  struct tm t;
  if (getLocalTime(&t, 0) && t.tm_year > 120 && t.tm_min != lastMin) {
    lastMin = t.tm_min;
    drawClock();
  }
  updateNight();
  delay(20);
}
