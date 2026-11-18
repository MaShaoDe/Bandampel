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
#include <Preferences.h>
#include "secrets.h"     // vor lgfx_cyd.h, damit PANEL_* und andere Einstellungen gelten
#include "lgfx_cyd.h"

#ifndef CALLSIGN
#define CALLSIGN ""   // optional in secrets.h: #define CALLSIGN "HB9XXX"
#endif

// ---------------- Einstellungen ----------------
static const int      ROTATION       = 1;          // 1 oder 3 (180 Grad gedreht)
static const uint32_t FETCH_OK_MS    = 30UL * 60UL * 1000UL;
static const uint32_t FETCH_RETRY_MS = 2UL * 60UL * 1000UL;
static const char*    FEED_URL       = "https://www.hamqsl.com/solarxml.php";
static const int      BTN_PIN       = 0;

// Nachtmodus per Lichtsensor (CYD: GPIO 34). Einstellungen koennen in secrets.h ueberschrieben werden.
// LDR_THRESHOLD = -1: aus (nur Messwerte auf der seriellen Schnittstelle).
#ifndef LDR_THRESHOLD
#define LDR_THRESHOLD -1
#endif
#ifndef LDR_NIGHT_IS_HIGH
#define LDR_NIGHT_IS_HIGH true     // true: grosser Messwert = dunkel
#endif
#ifndef BRIGHT_MODE
#define BRIGHT_MODE 2              // 0 fest (Tageswert), 1 Tag/Nacht mit Schwelle, 2 stufenlos nach Lichtsensor
#endif
#ifndef BRIGHT_DAY_PCT
#define BRIGHT_DAY_PCT 60          // Helligkeit am Tag in Prozent (5 bis 100)
#endif
#ifndef BRIGHT_NIGHT_PCT
#define BRIGHT_NIGHT_PCT 10        // Helligkeit in der Nacht in Prozent (5 bis 100)
#endif
#ifndef LDR_BRIGHT
#define LDR_BRIGHT 100             // Sensorwert bei hellem Raum (Kalibrierung, stufenlos)
#endif
#ifndef LDR_DARK
#define LDR_DARK 700               // Sensorwert bei dunklem Raum (Kalibrierung, stufenlos)
#endif
static const int LDR_PIN  = 34;
#ifndef LDR_HYST
#define LDR_HYST 150               // Toleranz um die Schwelle (grosser = traeger, kleiner = empfindlicher)
#endif
// Seiten ein (1) oder aus (0), in secrets.h einstellbar
#ifndef PAGE_OVERVIEW
#define PAGE_OVERVIEW 1           // Uebersicht (SFI, K, A, Sunspots)
#endif
#ifndef PAGE_FORECAST
#define PAGE_FORECAST 1           // Prognose Baender (HamQSL)
#endif
#ifndef PAGE_MODES
#define PAGE_MODES 1              // Prognose Betriebsart (SSB, CW)
#endif
#ifndef PAGE_LIVE
#define PAGE_LIVE 1               // Live-Aktivitaet (WSPR, inkl. 160m)
#endif
#ifndef PAGE_DETAILS
#define PAGE_DETAILS 1            // Details (X-Ray, Sonnenwind, ...)
#endif
#ifndef SHOW_SSB
#define SHOW_SSB 1                // Seite Betriebsart: SSB zeigen
#endif
#ifndef SHOW_CW
#define SHOW_CW 1                 // Seite Betriebsart: CW zeigen
#endif
#ifndef LIVE_FIELDS
#define LIVE_FIELDS "'JN','JO'"   // Locator-Felder der Region (Sender oder Empfaenger)
#endif
#ifndef LIVE_MINUTES
#define LIVE_MINUTES 30           // Zeitfenster der Live-Aktivitaet
#endif
#define FW_VERSION "1.18.8"         // Firmware-Version (fest im Code)
#define FW_URL "https://github.com/MaShaoDe/Bandampel"
#ifndef PAGE_ABOUT
#define PAGE_ABOUT 1              // Ueber (Urheber, Lizenz, Kontakt, QR-Code)
#endif
#ifndef PAGE_INFO
#define PAGE_INFO 1               // Info (WLAN, IP, Signal, Name, Datenalter, Laufzeit)
#endif
#ifndef PAGE_SUN
#define PAGE_SUN 1                // Sonne (Aufgang, Untergang, Grayline)
#endif
#ifndef SUN_LAT
#define SUN_LAT 47.556            // Standort fuer Sonnenzeiten (Voreinstellung Frauenfeld, Locator JN47)
#endif
#ifndef SUN_LON
#define SUN_LON 8.898
#endif
static const uint32_t LIVE_OK_MS = 15UL * 60UL * 1000UL;   // Abruf wspr.live alle 15 Minuten
#ifndef SSB_FIRST
#define SSB_FIRST 1               // 1: SSB oben und CW unten, 0: umgekehrt (in secrets.h einstellbar)
#endif
#ifndef AUTO_PAGE_MS
#define AUTO_PAGE_MS 10000        // Seitenwechsel im Auto-Modus
#endif
#include "settings.h"     // Laufzeit-Einstellungen (cfg), Vorgabe aus den Makros oben
static const uint32_t LONG_PRESS_MS  = 800;       // langer Druck: Auto/Halten umschalten

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
static bool     autoMode    = true;      // true: Seiten wechseln automatisch
static uint32_t lastPageChange = 0;
static String   statusMsg   = "Start";
static uint32_t nextFetch   = 0;
static bool     fetchedOnce = false;
static bool     apActive    = false;   // Notfall-WLAN Bandampel-Setup aktiv (siehe webcfg.h)

enum PageId { PG_OVERVIEW, PG_FORECAST, PG_MODES, PG_LIVE, PG_DETAILS, PG_SUN, PG_INFO, PG_ABOUT };
static int pageIds[8];
// Seitennamen in der Kopfzeile (ASCII, gleiche Reihenfolge wie PageId)
static const char* const PAGE_NAMES[8] = { "Ampel", "Baender", "Modus", "Live", "Details", "Sonne", "Info", "Lizenz" };
static int pageCount = 0;

static void buildPages() {
  pageCount = 0;
  if (cfg.pOverview) pageIds[pageCount++] = PG_OVERVIEW;
  if (cfg.pSun)      pageIds[pageCount++] = PG_SUN;
  if (cfg.pForecast) pageIds[pageCount++] = PG_FORECAST;
  if (cfg.pModes)    pageIds[pageCount++] = PG_MODES;
  if (cfg.pLive)     pageIds[pageCount++] = PG_LIVE;
  if (cfg.pDetails)  pageIds[pageCount++] = PG_DETAILS;
  if (cfg.pInfo)     pageIds[pageCount++] = PG_INFO;
  if (cfg.pAbout)    pageIds[pageCount++] = PG_ABOUT;
  if (pageCount == 0) pageIds[pageCount++] = PG_OVERVIEW;   // mindestens eine Seite
}

// Live-Aktivitaet (WSPR, wspr.live). Bandcodes der Datenbank: 1=160m, 3=80m, 7=40m, 10=30m, 14=20m, 18=17m, 21=15m, 24=12m, 28=10m.
// Schwellen (Spots in der Region im Zeitfenster) sind ein Vorschlag fuer 30 Minuten und noch einzustellen.
static const int   LIVE_WSPR_BAND[9] = { 1, 3, 7, 10, 14, 18, 21, 24, 28 };
static const char* LIVE_LABELS[9]    = { "160", "80", "40", "30", "20", "17", "15", "12", "10" };
static int      liveCount[9]    = { 0 };
static int      ldrRaw          = -1;     // letzter Messwert des Lichtsensors (fuer das Web-Menue)
static bool     haveLive        = false;
static bool     liveFetchedOnce = false;
static uint32_t nextLiveFetch   = 0;

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

// ---------------- Live-Aktivitaet (wspr.live) ----------------
static String urlEncode(const String& in) {
  static const char* hex = "0123456789ABCDEF";
  String out;
  for (size_t i = 0; i < in.length(); i++) {
    uint8_t c = (uint8_t)in[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += (char)c;
    else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
  }
  return out;
}

static bool fetchLive() {
  String f = cfg.liveFields;
  String q = String("SELECT band, count() FROM wspr.rx WHERE time > now() - ") + String(cfg.liveMinutes * 60) +
             " AND (substring(tx_loc,1,2) IN (" + f + ") OR substring(rx_loc,1,2) IN (" + f + "))" +
             " GROUP BY band ORDER BY band FORMAT TSV";
  String url = String("https://db1.wspr.live/?query=") + urlEncode(q);
  WiFiClientSecure client;
  client.setInsecure();   // offen: Zertifikatspruefung
  HTTPClient http;
  http.setTimeout(20000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code != 200) {
    Serial.printf("wspr.live HTTP %d\n", code);
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();
  Serial.printf("wspr.live (%u Bytes):\n%s\n", (unsigned)body.length(), body.c_str());

  int counts[9] = { 0 };
  int pos = 0;
  while (pos < (int)body.length()) {
    int nl = body.indexOf('\n', pos);
    if (nl < 0) nl = body.length();
    String line = body.substring(pos, nl);
    pos = nl + 1;
    int tab = line.indexOf('\t');
    if (tab < 0) continue;
    int band = line.substring(0, tab).toInt();
    int n = line.substring(tab + 1).toInt();
    for (int i = 0; i < 9; i++) if (LIVE_WSPR_BAND[i] == band) counts[i] = n;
  }
  for (int i = 0; i < 9; i++) liveCount[i] = counts[i];
  haveLive = true;
  return true;
}

// ---------------- Zeichnen ----------------
static void drawClock() {
  tft.fillRect(196, 0, 124, 26, COL_HDR);   // Uhr beginnt rechts von x=202, links davon steht der Seitenname
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
  const char* cs = cfg.callsign.length() ? cfg.callsign.c_str() : "BANDAMPEL";
  tft.drawString(cs, 6, 3);
  int x = 6 + tft.textWidth(cs) + 10;                 // Seitenname hinter dem Rufzeichen, nur wenn er vor die Uhr passt
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_YELLOW, COL_HDR);
  const char* nm = PAGE_NAMES[pageIds[page]];
  if (x + tft.textWidth(nm) <= 194) tft.drawString(nm, x, 7);
  drawClock();
}

// Alter der Daten aus dem HamQSL-Zeitstempel ("18 Nov 2026 1611 GMT")
static long daysFromCivil(int y, int m, int d) {
  y -= m <= 2;
  long era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long)doe - 719468;
}
static bool parseUpdated(const String& s, time_t& out) {
  int d, y, hh, mm;
  char mon[4] = { 0 };
  if (sscanf(s.c_str(), "%d %3s %d %2d%2d", &d, mon, &y, &hh, &mm) != 5) return false;
  const char* M = "JanFebMarAprMayJunJulAugSepOctNovDec";
  int mi = -1;
  for (int i = 0; i < 12; i++) if (strncmp(mon, M + i * 3, 3) == 0) mi = i;
  if (mi < 0) return false;
  out = (time_t)(daysFromCivil(y, mi + 1, d) * 86400L + hh * 3600L + mm * 60L);
  return true;
}
static String ageShort(long sec) {
  if (sec < 0) sec = 0;
  long m = sec / 60;
  if (m < 1)    return String("<1 min");
  if (m < 60)   return String(m) + " min";
  if (m < 1440) return String(m / 60) + " h " + String(m % 60) + " min";
  return String(m / 1440) + " d";
}
static String dataAgeText() {
  time_t u, now = time(nullptr);
  if (now > 1700000000 && parseUpdated(solar.updated, u)) return ageShort((long)(now - u));
  return String("");
}

// WLAN-Symbol: 3 Baegen, gruen = verbunden (Staerke nach RSSI), rot = keine Verbindung
static void drawWifiIcon(int cx, int cy, bool ok, int rssi) {
  int lvl = !ok ? 0 : (rssi >= -60 ? 3 : (rssi >= -72 ? 2 : 1));
  for (int L = 1; L <= 3; L++) {
    uint16_t col = !ok ? COL_RED : (L <= lvl ? COL_GREEN : COL_GREY);
    int r = 3 + L * 4;
    int px = 0, py = 0;
    for (int k = 0; k <= 8; k++) {
      float a = (50 + k * 10) * 0.0174533f;
      int x = cx + (int)(r * cosf(a)), y = cy - (int)(r * sinf(a));
      if (k) { tft.drawLine(px, py, x, y, col); tft.drawLine(px, py + 1, x, y + 1, col); }
      px = x; py = y;
    }
  }
  tft.fillCircle(cx, cy, 2, ok ? COL_GREEN : COL_RED);
}

static void drawFooter() {
  tft.fillRect(0, 220, 320, 20, COL_BG);
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  String left = statusMsg;
  if (haveData) { String a = dataAgeText(); left = a.length() ? String("Daten vor ") + a : String("Daten OK"); }
  if (left.length() > 20) left = left.substring(0, 20);
  tft.drawString(left, 6, 222);
  tft.setTextColor(autoMode ? COL_DIM : COL_YELLOW, COL_BG);
  tft.drawString(autoMode ? "AUTO" : "HALT", 206, 222);
  tft.setTextColor(COL_DIM, COL_BG);
  bool ok = (WiFi.status() == WL_CONNECTED);
  drawWifiIcon(262, 235, ok, ok ? WiFi.RSSI() : 0);
  char pg[8];
  snprintf(pg, sizeof(pg), "%d/%d", page + 1, pageCount);
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

// Prognose-Seiten: 8 Baender in 4 HamQSL-Gruppen. HamQSL liefert kein 160m, das steht auf der Live-Seite.
static const char* FC_LABELS[8] = { "80", "40", "30", "20", "17", "15", "12", "10" };
static const int   FC_GROUP[8]  = { 0, 0, 1, 1, 2, 2, 3, 3 };   // Index in bandDay/bandNight

static Cond stepDown(Cond c) { return c == C_GOOD ? C_FAIR : (c == C_FAIR ? C_POOR : c); }
static Cond stepUp(Cond c)   { return c == C_POOR ? C_FAIR : (c == C_FAIR ? C_GOOD : c); }

static void drawBand(int x, int y, int w, int h, Cond c) {
  tft.fillRoundRect(x, y, w, h, 4, colFor(c));
}

// Sonnenaufgang und -untergang (Naeherung nach Almanac, auf etwa 1 bis 2 Minuten genau).
// Rueckgabe: Stunden in UTC, oder -1 bei Polartag oder Polarnacht.
static double sunEventUtc(int yday, bool rise) {
  const double D2R = 0.017453292519943295, R2D = 57.29577951308232;
  double lngH = cfg.sunLon / 15.0;
  double t = (yday + 1) + ((rise ? 6.0 : 18.0) - lngH) / 24.0;
  double M = 0.9856 * t - 3.289;
  double L = M + 1.916 * sin(M * D2R) + 0.020 * sin(2 * M * D2R) + 282.634;
  L = fmod(L + 720.0, 360.0);
  double RA = fmod(R2D * atan(0.91764 * tan(L * D2R)) + 720.0, 360.0);
  RA += floor(L / 90.0) * 90.0 - floor(RA / 90.0) * 90.0;
  RA /= 15.0;
  double sinDec = 0.39782 * sin(L * D2R);
  double cosDec = cos(asin(sinDec));
  double cosH = (cos(90.833 * D2R) - sinDec * sin(cfg.sunLat * D2R)) / (cosDec * cos(cfg.sunLat * D2R));
  if (cosH > 1.0 || cosH < -1.0) return -1;
  double H = (rise ? 360.0 - R2D * acos(cosH) : R2D * acos(cosH)) / 15.0;
  double T = H + RA - 0.06571 * t - 6.622;
  return fmod(T - lngH + 48.0, 24.0);
}

// Setzt Aufgang und Untergang von heute als Epoche. false, wenn die Uhr noch nicht stimmt.
static bool sunToday(time_t& rise, time_t& set) {
  time_t now = time(nullptr);
  if (now < 1700000000) return false;
  struct tm g; gmtime_r(&now, &g);
  time_t midnight = now - (g.tm_hour * 3600 + g.tm_min * 60 + g.tm_sec);
  double r = sunEventUtc(g.tm_yday, true), u = sunEventUtc(g.tm_yday, false);
  if (r < 0 || u < 0) return false;
  rise = midnight + (time_t)(r * 3600.0);
  set  = midnight + (time_t)(u * 3600.0);
  if (set < rise) set += 86400;
  return true;
}

static String hhmm(time_t t) {
  struct tm l; localtime_r(&t, &l);
  char b[8]; snprintf(b, sizeof(b), "%02d:%02d", l.tm_hour, l.tm_min);
  return String(b);
}

static void drawPageBands() {
  const int X0 = 40, CW = 34, BW = 28;
  const int YD = 52, YN = 106, BH = 46;
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  for (int i = 0; i < 8; i++) {
    int cx = X0 + i * CW + CW / 2;
    int tw = tft.textWidth(FC_LABELS[i]);
    tft.drawString(FC_LABELS[i], cx - tw / 2, 30);
    int g = FC_GROUP[i];
    drawBand(cx - BW / 2, YD, BW, BH, haveData ? bandDay[g]   : C_UNK);
    drawBand(cx - BW / 2, YN, BW, BH, haveData ? bandNight[g] : C_UNK);
  }
  // Aktuelle Zeile (Tag oder Nacht) hell, die andere gedaempft
  time_t sr = 0, ss = 0, tnow = time(nullptr);
  bool haveSun = sunToday(sr, ss);
  bool isDay = haveSun && tnow >= sr && tnow < ss;
  drawIcon(1, 18, YD + BH / 2, (!haveSun || isDay) ? COL_YELLOW : COL_DIM);
  drawIcon(5, 18, YN + BH / 2, (haveSun && !isDay) ? COL_TEXT : COL_DIM);

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

// Dauer als Text: "2 h 14 min" oder "35 min"
static String fmtDur(long sec) {
  if (sec < 0) sec = 0;
  long m = (sec + 30) / 60;
  if (m >= 60) return String(m / 60) + " h " + String(m % 60) + " min";
  return String(m) + " min";
}

static void drawCentered(const String& t, int cx, int y) {
  tft.drawString(t, cx - tft.textWidth(t) / 2, y);
}

// Seite Sonne: Bogen mit Sonnen- oder Mondposition, Zeiten, Tageslaenge, Countdown, Grayline
static void drawPageSun() {
  const int CX = 160, BASE = 124, R = 96, RN = 22;   // Mitte, Horizont, Radius Tag, Tiefe Nacht
  time_t sr = 0, ss = 0, now = time(nullptr);
  bool have = sunToday(sr, ss);

  // Horizont und Tagesbogen
  tft.drawFastHLine(24, BASE, 272, COL_GREY);
  float f = -1;                       // Anteil des Tages (0 bis 1), -1 = Nacht
  float fn = 0;                       // Anteil der Nacht (0 bis 1)
  bool isDay = false;
  if (have) {
    isDay = now >= sr && now < ss;
    if (isDay) f = (float)(now - sr) / (float)(ss - sr);
    else {
      time_t ns, ne;                  // Nachtspanne: von Untergang bis naechstem Aufgang
      if (now < sr) { ns = ss - 86400; ne = sr; } else { ns = ss; ne = sr + 86400; }
      fn = (float)(now - ns) / (float)(ne - ns);
    }
  }
  int px = CX - R, py = BASE;
  for (int k = 1; k <= 48; k++) {
    float a = 3.1415927f * k / 48.0f;
    int nx = CX - (int)(R * cosf(a)), ny = BASE - (int)(R * sinf(a));
    bool passed = isDay && (k / 48.0f) <= f;
    tft.drawLine(px, py, nx, ny, passed ? COL_YELLOW : COL_GREY);
    px = nx; py = ny;
  }
  // Nachtbogen flach unter dem Horizont
  px = CX - R; py = BASE;
  for (int k = 1; k <= 48; k++) {
    float a = 3.1415927f * k / 48.0f;
    int nx = CX - (int)(R * cosf(a)), ny = BASE + (int)(RN * sinf(a));
    tft.drawLine(px, py, nx, ny, COL_HDR);
    px = nx; py = ny;
  }
  if (have) {
    if (isDay) {
      float a = 3.1415927f * f;
      drawIcon(1, CX - (int)(R * cosf(a)), BASE - (int)(R * sinf(a)), COL_YELLOW);
    } else {
      float a = 3.1415927f * fn;
      drawIcon(5, CX - (int)(R * cosf(a)), BASE + (int)(RN * sinf(a)), COL_TEXT);
    }
  }

  tft.setFont(&lgfx::fonts::Font2);
  if (!have) {
    tft.setTextColor(COL_DIM, COL_BG);
    drawCentered("Warte auf Uhrzeit", CX, 90);
    return;
  }

  // Grayline: +-30 Minuten um Aufgang und Untergang (Faustregel fuer tiefe Baender)
  const long GL = 1800;
  time_t ev[3] = { sr, ss, sr + 86400 };
  bool gl = false;
  long untilGl = 999999;
  for (int i = 0; i < 3; i++) {
    long d = (long)(now - ev[i]);
    if (d >= -GL && d <= GL) gl = true;
    long start = (long)(ev[i] - GL - now);
    if (start > 0 && start < untilGl) untilGl = start;
  }
  if (gl) {
    tft.setTextColor(COL_YELLOW, COL_BG);
    drawCentered("GRAYLINE AKTIV", CX, 92);
    tft.setTextColor(COL_DIM, COL_BG);
    drawCentered("160, 80, 40 m oft gut", CX, 108);
  } else {
    tft.setTextColor(COL_DIM, COL_BG);
    drawCentered("Grayline in " + fmtDur(untilGl), CX, 100);
  }

  // Zeiten unter dem Horizont
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString("Aufgang", 14, 160);
  String lu = "Untergang";
  tft.drawString(lu, 306 - tft.textWidth(lu), 160);
  drawCentered("Tageslaenge", CX, 160);
  tft.setFont(&lgfx::fonts::Font4);
  tft.setTextColor(COL_YELLOW, COL_BG);
  tft.drawString(hhmm(sr), 14, 178);
  String ts = hhmm(ss);
  tft.drawString(ts, 306 - tft.textWidth(ts), 178);
  tft.setTextColor(COL_TEXT, COL_BG);
  long dl = (long)(ss - sr);
  String dls = String(dl / 3600) + ":" + (((dl % 3600) / 60) < 10 ? "0" : "") + String((dl % 3600) / 60);
  drawCentered(dls, CX, 178);

  // Countdown zum naechsten Ereignis
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  time_t nextEv = isDay ? ss : (now < sr ? sr : sr + 86400);
  drawCentered(String(isDay ? "Untergang in " : "Aufgang in ") + fmtDur((long)(nextEv - now)), CX, 204);
}

// Betriebsart (Vorschlag, noch zu pruefen): HamQSL kennt keine Betriebsarten.
// CW und Digital (FT8) kommen mit ca. 10 bis 15 dB weniger Signal aus als SSB, daher eine Stufe besser.
// Bei K >= 5 (gestoert) fuer beide eine Stufe schlechter.
static Cond modeCw(Cond c)    { return c == C_UNK ? c : stepUp(c); }
static Cond disturbed(Cond c) { return (c != C_UNK && solar.k.toInt() >= 5) ? stepDown(c) : c; }

static void drawModeBlock(const char* name, bool cw, int y, int bh) {
  const int X0 = 56, CWID = 31, BW = 25;
  for (int i = 0; i < 8; i++) {
    int cx = X0 + i * CWID + CWID / 2;
    int g = FC_GROUP[i];
    Cond d = haveData ? disturbed(bandDay[g])   : C_UNK;
    Cond n = haveData ? disturbed(bandNight[g]) : C_UNK;
    if (cw) { d = modeCw(d); n = modeCw(n); }
    drawBand(cx - BW / 2, y,          BW, bh, d);
    drawBand(cx - BW / 2, y + bh + 4, BW, bh, n);
  }
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(name, 2, y + (2 * bh + 4) / 2 - tft.fontHeight() / 2);
  drawIcon(1, 42, y + bh / 2, COL_YELLOW);
  drawIcon(5, 42, y + bh + 4 + bh / 2, COL_DIM);
}

static void drawPageModes() {
  const int X0 = 56, CWID = 31;
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  for (int i = 0; i < 8; i++) {
    int cx = X0 + i * CWID + CWID / 2;
    int tw = tft.textWidth(FC_LABELS[i]);
    tft.drawString(FC_LABELS[i], cx - tw / 2, 28);
  }
  bool showSsb = cfg.showSsb, showCw = cfg.showCw;
  if (!showSsb && !showCw) showSsb = showCw = true;
  int n = (showSsb ? 1 : 0) + (showCw ? 1 : 0);
  int bh = (n == 2) ? 26 : 44;
  int y = 48;
  if (cfg.ssbFirst) {
  if (showSsb) { drawModeBlock("SSB", false, y, bh); y += 2 * bh + 4 + 14; }
  if (showCw)  { drawModeBlock("CW",  true,  y, bh); y += 2 * bh + 4 + 14; }
  } else {
  if (showCw)  { drawModeBlock("CW",  true,  y, bh); y += 2 * bh + 4 + 14; }
  if (showSsb) { drawModeBlock("SSB", false, y, bh); y += 2 * bh + 4 + 14; }
  }
  if (n == 2) tft.drawFastHLine(8, 108, 304, COL_GREY);
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  if (showCw) tft.drawString("CW inkl. Digital (FT8)", 8, 184);
  tft.drawString("Faustregel, keine Prognose", 8, 200);
}

// Live-Aktivitaet: gemessene WSPR-Spots, alle 9 Baender inkl. 160m
static String shortCount(int c) {
  if (c >= 10000) return String(c / 1000) + "k";
  if (c >= 1000)  return String(c / 1000) + "." + String((c % 1000) / 100) + "k";
  return String(c);
}

static Cond liveCond(int i) {
  if (!haveLive) return C_UNK;
  int c = liveCount[i];
  if (c >= cfg.liveGood[i]) return C_GOOD;
  if (c >= cfg.liveFair[i]) return C_FAIR;
  return C_POOR;
}

static void drawPageLive() {
  const int X0 = 8, CW = 34, BW = 26, BASE = 150, HMAX = 100;
  for (int i = 0; i < 9; i++) {
    int cx = X0 + i * CW + CW / 2;
    tft.setFont(&lgfx::fonts::Font2);
    tft.setTextColor(COL_DIM, COL_BG);
    int tw = tft.textWidth(LIVE_LABELS[i]);
    tft.drawString(LIVE_LABELS[i], cx - tw / 2, 30);
    int c = haveLive ? liveCount[i] : 0;
    int h = 0;
    if (haveLive) {
      h = (c >= cfg.liveGood[i]) ? HMAX : (int)((long)c * HMAX / cfg.liveGood[i]);
      if (c > 0 && h < 4) h = 4;
    }
    tft.drawRoundRect(cx - BW / 2, BASE - HMAX, BW, HMAX, 3, COL_GREY);
    if (h > 0) tft.fillRoundRect(cx - BW / 2, BASE - h, BW, h, 2, colFor(liveCond(i)));
    String sc = haveLive ? shortCount(c) : String("--");
    int sw = tft.textWidth(sc);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.drawString(sc, cx - sw / 2, BASE + 6);
  }
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString(String("LIVE: WSPR-Spots der letzten ") + cfg.liveMinutes + " min", 8, 184);
  tft.drawString(String("Sender oder Empfaenger in ") + cfg.liveFields, 8, 200);
}

// Details-Seite: 4 Karten mit farbigem Rahmen. Einstufung ist eine Faustregel (Vorschlag), keine Prognose.
static Cond levelXray(const String& x) {
  if (!x.length()) return C_UNK;
  char c = toupper(x[0]);
  if (c == 'A' || c == 'B') return C_GOOD;
  if (c == 'C') return C_FAIR;
  if (c == 'M' || c == 'X') return C_POOR;
  return C_UNK;
}
static Cond levelWind(const String& w) {
  if (!w.length()) return C_UNK;
  int v = w.toInt();
  return v < 400 ? C_GOOD : (v < 600 ? C_FAIR : C_POOR);
}
static Cond levelBz(const String& b) {
  if (!b.length()) return C_UNK;
  float v = b.toFloat();
  return v >= -3 ? C_GOOD : (v >= -8 ? C_FAIR : C_POOR);   // negativ (nach Sueden) stoert das Erdmagnetfeld
}
static Cond levelNoise(const String& n) {
  int d = -1;
  for (unsigned i = 0; i < n.length(); i++) if (isdigit(n[i])) { d = n[i] - '0'; break; }   // "S0-S1": erste Zahl
  if (d < 0) return C_UNK;
  return d <= 1 ? C_GOOD : (d <= 3 ? C_FAIR : C_POOR);
}

static void drawDetailCard(int x, int y, const char* label, const String& val, const char* unit, Cond c, int icon,
                           const char* good, const char* fair, const char* poor) {
  const int w = 156, ch = 86;
  uint16_t col = colFor(c);
  tft.fillRoundRect(x, y, w, ch, 8, COL_HDR);
  tft.drawRoundRect(x, y, w, ch, 8, col);
  tft.drawRoundRect(x + 1, y + 1, w - 2, ch - 2, 7, col);
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_HDR);
  tft.drawString(label, x + 8, y + 6);
  drawIcon(icon, x + w - 22, y + 18, col);
  tft.setFont(&lgfx::fonts::Font4);
  tft.setTextColor(col, COL_HDR);
  String v = val.length() ? val : String("--");
  if (unit[0]) v += String(" ") + unit;
  tft.drawString(v, x + (w - tft.textWidth(v)) / 2, y + 34);
  tft.setFont(&lgfx::fonts::Font2);
  const char* word = (c == C_GOOD) ? good : (c == C_FAIR ? fair : (c == C_POOR ? poor : "keine Daten"));
  tft.setTextColor(col, COL_HDR);
  tft.drawString(word, x + (w - tft.textWidth(word)) / 2, y + 64);
}

static void drawPageDetails() {
  bool d = haveData;
  drawDetailCard(4,   30,  "X-Ray (Flares)", solar.xray,  "",     d ? levelXray(solar.xray)   : C_UNK, 1, "ruhig", "leicht erhoeht", "Funkstoerung");
  drawDetailCard(160, 30,  "Sonnenwind",     solar.wind,  "km/s", d ? levelWind(solar.wind)   : C_UNK, 4, "ruhig", "erhoeht", "schnell");
  drawDetailCard(4,   124, "Magnetfeld Bz",  solar.bz,    "nT",   d ? levelBz(solar.bz)       : C_UNK, 2, "ruhig", "unruhig", "gestoert");
  drawDetailCard(160, 124, "Rauschen",       solar.noise, "",     d ? levelNoise(solar.noise) : C_UNK, 3, "leise", "mittel", "laut");
}

// Seite Info: WLAN, IP, Signal, Name im Netz, Alter der Daten, Laufzeit
static void drawInfoRow(int y, const char* label, const String& val, uint16_t col) {
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString(label, 10, y + 4);
  tft.setFont(&lgfx::fonts::Font4);
  tft.setTextSize(0.8f);                          // eine Stufe kleiner, damit 7 Zeilen passen
  tft.setTextColor(col, COL_BG);
  tft.drawString(val, 310 - tft.textWidth(val), y);
  tft.setTextSize(1.0f);
}

static void drawPageInfo() {
  bool ok = WiFi.status() == WL_CONNECTED;
  String ssid = ok ? WiFi.SSID() : (apActive ? String("Bandampel-Setup") : String("getrennt"));
  if (ssid.length() > 16) ssid = ssid.substring(0, 16);
  String ip = ok ? WiFi.localIP().toString() : (apActive ? WiFi.softAPIP().toString() : String("--"));
  int rssi = ok ? WiFi.RSSI() : 0;
  uint16_t sc = !ok ? COL_RED : (rssi >= -60 ? COL_GREEN : (rssi >= -72 ? COL_YELLOW : COL_RED));
  String sig = !ok ? String("--") : String(rssi) + " dBm " + (rssi >= -60 ? "gut" : (rssi >= -72 ? "mittel" : "schwach"));
  String age = haveData ? dataAgeText() : String("");
  long up = (long)(millis() / 1000UL);
  String upt = (up >= 86400) ? String(up / 86400) + " d " + String((up % 86400) / 3600) + " h"
             : (up >= 3600  ? String(up / 3600) + " h " + String((up % 3600) / 60) + " min" : String(up / 60) + " min");
  drawInfoRow(30,  "Firmware",   String(FW_VERSION), COL_YELLOW);
  drawInfoRow(57,  "WLAN",       ssid, ok ? COL_GREEN : COL_RED);
  drawInfoRow(84,  "IP-Adresse", ip,   ok || apActive ? COL_TEXT : COL_DIM);
  drawInfoRow(111, "Signal",     sig,  sc);
  drawInfoRow(138, "Name",       cfgHostname() + ".local", COL_TEXT);
  drawInfoRow(165, "Daten",      age.length() ? String("vor ") + age : String("--"), COL_TEXT);
  drawInfoRow(192, "Laufzeit",   upt,  COL_TEXT);
}

// Seite Ueber: Urheber, Lizenz, Kontakt (fest im Code, nicht aenderbar)
static void drawPageAbout() {
  tft.setFont(&lgfx::fonts::Font4);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("BANDAMPEL", 8, 28);
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString("Version " FW_VERSION, 8, 56);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("(c) 2026", 8, 80);
  tft.drawString("FerienBSD aka", 8, 96);
  tft.drawString("Marcel Sauder", 8, 112);
  tft.setFont(&lgfx::fonts::Font4);
  tft.setTextColor(COL_YELLOW, COL_BG);
  tft.drawString("HE9QCU", 8, 130);

  tft.qrcode(FW_URL, 190, 28, 120, 3);                       // QR-Code zum GitHub-Projekt
  tft.setFont(&lgfx::fonts::Font2);
  tft.setTextColor(COL_DIM, COL_BG);
  drawCentered("GitHub", 250, 152);

  tft.drawFastHLine(8, 166, 304, COL_GREY);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Firmware GPL-3.0-only", 8, 172);
  tft.drawString("Hardware CERN-OHL-P-2.0", 8, 188);
  tft.drawString("Doku CC-BY-4.0", 8, 204);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString("Daten:", 196, 172);
  tft.drawString("HamQSL, NOAA", 196, 188);
  tft.drawString("wspr.live", 196, 204);
}

static void drawAll() {
  tft.fillScreen(COL_BG);
  drawHeader();
  switch (pageIds[page]) {
    case PG_OVERVIEW: drawPageOverview(); break;
    case PG_FORECAST: drawPageBands();    break;
    case PG_MODES:    drawPageModes();    break;
    case PG_LIVE:     drawPageLive();     break;
    case PG_SUN:      drawPageSun();      break;
    case PG_INFO:     drawPageInfo();     break;
    case PG_ABOUT:    drawPageAbout();    break;
    default:          drawPageDetails();  break;
  }
  drawFooter();
}

// ---------------- Eingabe ----------------
static void saveAutoMode() {
  Preferences prefs;
  prefs.begin("bandampel", false);
  prefs.putBool("auto", autoMode);
  prefs.end();
}

// Kurz tippen oder BOOT-Taste: naechste Seite. Lang druecken (0,8 s): Auto/Halten umschalten.
static void handleInput() {
  static bool     down = false, longDone = false;
  static uint32_t downAt = 0, lastSeen = 0;
  static uint32_t btnAt = 0;            // BOOT-Taste gedrueckt seit (nur die Taste, nicht der Touch)
  static int      lastCount = -1;
  static bool     undone = false;
  uint32_t now = millis();
  int32_t tx, ty;

  // BOOT-Taste 10 s halten: Werkseinstellung (ab 5 s Countdown, loslassen bricht ab)
  bool btn = (digitalRead(BTN_PIN) == LOW);
  if (btn && !btnAt) { btnAt = now; lastCount = -1; undone = false; }
  if (!btn && btnAt) {
    bool shown = lastCount >= 0;
    btnAt = 0; lastCount = -1;
    if (shown) drawAll();
  }
  if (btn && btnAt && now - btnAt >= 5000) {
    if (!undone) {                      // der lange Druck hat Auto/Halten umgeschaltet: zurueck
      undone = true;
      if (longDone) { autoMode = !autoMode; saveAutoMode(); }
    }
    int remain = 10 - (int)((now - btnAt) / 1000);
    if (remain < 0) remain = 0;
    if (remain != lastCount) {
      lastCount = remain;
      tft.fillRoundRect(24, 76, 272, 92, 10, COL_HDR);
      tft.drawRoundRect(24, 76, 272, 92, 10, COL_RED);
      tft.drawRoundRect(25, 77, 270, 90, 9, COL_RED);
      tft.setFont(&lgfx::fonts::Font4);
      tft.setTextColor(COL_RED, COL_HDR);
      drawCentered(String("Zuruecksetzen in ") + remain + " s", 160, 96);
      tft.setFont(&lgfx::fonts::Font2);
      tft.setTextColor(COL_DIM, COL_HDR);
      drawCentered("Taste loslassen = abbrechen", 160, 134);
    }
    if (now - btnAt >= 10000) {
      cfgFactoryReset();
      tft.fillRoundRect(24, 76, 272, 92, 10, COL_HDR);
      tft.setFont(&lgfx::fonts::Font4);
      tft.setTextColor(COL_GREEN, COL_HDR);
      drawCentered("Werkseinstellung", 160, 96);
      tft.setFont(&lgfx::fonts::Font2);
      tft.setTextColor(COL_DIM, COL_HDR);
      drawCentered("Neustart ...", 160, 134);
      delay(1500);
      ESP.restart();
    }
  }
  bool pressed = (digitalRead(BTN_PIN) == LOW) || tft.getTouch(&tx, &ty);
  if (pressed) lastSeen = now;
  if (pressed && !down) {
    down = true; longDone = false; downAt = now;
  } else if (down && !longDone && pressed && now - downAt >= LONG_PRESS_MS) {
    longDone = true;
    autoMode = !autoMode;
    saveAutoMode();
    lastPageChange = now;
    drawFooter();
  } else if (down && !pressed && now - lastSeen > 60) {   // losgelassen (mit Entprellung)
    down = false;
    if (!longDone && lastSeen - downAt >= 30) {
      page = (page + 1) % pageCount;
      lastPageChange = now;
      drawAll();
    }
  }
}

static void autoAdvance() {
  uint32_t now = millis();
  if (autoMode && now - lastPageChange >= (uint32_t)cfg.autoMs) {
    page = (page + 1) % pageCount;
    lastPageChange = now;
    drawAll();
  }
}

// ---------------- Nachtmodus ----------------
static uint8_t curBrightness = 150;

static uint8_t pctToRaw(int pct) { return (uint8_t)(constrain(pct, 5, 100) * 255 / 100); }

static void updateNight() {
  static uint32_t lastRead = 0, lastPrint = 0;
  static bool night = false;
  static float filt = -1;
  uint32_t now = millis();
  if (now - lastRead < 1000) return;
  lastRead = now;
  long sum = 0;
  for (int i = 0; i < 8; i++) sum += analogRead(LDR_PIN);
  int v = (int)(sum / 8);
  ldrRaw = v;
  filt = (filt < 0) ? (float)v : filt * 0.85f + v * 0.15f;   // glaetten gegen Flackern
  int target = pctToRaw(cfg.brightPct);
  if (cfg.brightMode == 1 && cfg.ldrThreshold >= 0) {
    int dark = cfg.ldrNightHigh ? (v - cfg.ldrThreshold) : (cfg.ldrThreshold - v);   // positiv = dunkler als Schwelle
    if (!night && dark > cfg.ldrHyst) night = true;
    else if (night && dark < -cfg.ldrHyst) night = false;
    target = pctToRaw(night ? cfg.brightNightPct : cfg.brightPct);
  } else if (cfg.brightMode == 2) {
    float span = (float)(cfg.ldrDark - cfg.ldrBright);
    float t = (span != 0) ? (filt - cfg.ldrBright) / span : 0;   // 0 = hell, 1 = dunkel
    t = constrain(t, 0.0f, 1.0f);
    night = t > 0.5f;
    target = pctToRaw((int)(cfg.brightPct + t * (cfg.brightNightPct - cfg.brightPct) + 0.5f));
  }
  if (target != curBrightness) {
    int step = constrain(target - (int)curBrightness, -8, 8);   // sanfter Uebergang, 8 Stufen pro Sekunde
    curBrightness = (uint8_t)(curBrightness + step);
    tft.setBrightness(curBrightness);
  }
  if (now - lastPrint > 5000) {
    lastPrint = now;
    Serial.printf("LDR roh: %d  Nacht: %s\n", v, night ? "ja" : "nein");
  }
}

#include "webcfg.h"       // Web-Menue und Notfall-WLAN

// ---------------- Setup und Loop ----------------
void setup() {
  Serial.begin(115200);
  cfgLoad();
  buildPages();
  pinMode(BTN_PIN, INPUT_PULLUP);
  {
    Preferences prefs;
    prefs.begin("bandampel", true);
    autoMode = prefs.getBool("auto", true);
    prefs.end();
  }
  tft.init();
  tft.setRotation(ROTATION);
  curBrightness = pctToRaw(cfg.brightPct);
  tft.setBrightness(curBrightness);
  initColors();
  statusMsg = "WLAN suchen...";
  drawAll();

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(cfgHostname().c_str());
  Serial.printf("Geraet: %s\n", cfgHostname().c_str());
  for (int i = 0; i < cfg.wifiCount; i++) {
    if (!cfg.wifiSsid[i].length()) continue;
    wifiMulti.addAP(cfg.wifiSsid[i].c_str(), cfg.wifiPass[i].c_str());
    Serial.printf("Konfiguriert: [%s] (Passwort %u Zeichen)\n", cfg.wifiSsid[i].c_str(), (unsigned)cfg.wifiPass[i].length());
  }
  int n = WiFi.scanNetworks();
  Serial.printf("WLAN-Scan: %d Netze (nur 2,4 GHz sichtbar)\n", n);
  for (int i = 0; i < n; i++) {
    Serial.printf("  [%s]  RSSI %d  Kanal %d\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
  }
  configTzTime(cfg.tz.c_str(), "pool.ntp.org", "time.nist.gov");
  webInit();
}

void loop() {
  static uint32_t lastWifiTry = 0;
  static int      lastMin     = -1;
  static bool     lastWifiOk  = false;
  uint32_t now = millis();

  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  if (!wifiOk && (now - lastWifiTry > (apActive ? 300000UL : (uint32_t)cfg.wifiRetryS * 1000UL) || lastWifiTry == 0)) {
    lastWifiTry = now ? now : 1;
    statusMsg = "WLAN suchen...";
    drawFooter();
    if (apActive) WiFi.mode(WIFI_AP_STA);
    wl_status_t r = (wl_status_t)wifiMulti.run(20000);   // 20 s Zeit, iPhone-Hotspot ist langsam
    lastWifiTry = millis() ? millis() : 1;
    wifiOk = (WiFi.status() == WL_CONNECTED);
    Serial.printf("WLAN-Versuch: Status %d (0 = laeuft noch, 1 = Netz nicht gefunden, 3 = verbunden, 4 = fehlgeschlagen, 6 = getrennt)\n", (int)r);
    if (wifiOk) Serial.printf("Verbunden mit [%s], IP %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  }
  if (wifiOk != lastWifiOk) {
    lastWifiOk = wifiOk;
    if (wifiOk) statusMsg = WiFi.localIP().toString();
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

  if (wifiOk && cfg.pLive && (!liveFetchedOnce || (int32_t)(millis() - nextLiveFetch) >= 0)) {
    liveFetchedOnce = true;
    bool ok = fetchLive();
    nextLiveFetch = millis() + (ok ? LIVE_OK_MS : FETCH_RETRY_MS);
    if (pageIds[page] == PG_LIVE) drawAll();
  }

  // Notfall-WLAN: 60 Sekunden ohne bekanntes Netz, dann Bandampel-Setup oeffnen
  static uint32_t offSince = 0;
  if (wifiOk) {
    offSince = 0;
    if (apActive) { apStop(); statusMsg = WiFi.localIP().toString(); drawFooter(); }
  } else {
    if (!offSince) offSince = now ? now : 1;
    if (!apActive && now - offSince > 60000UL) { apStart(); statusMsg = "AP Bandampel-Setup"; drawFooter(); }
  }
  webLoop();

  handleInput();
  autoAdvance();

  struct tm t;
  if (getLocalTime(&t, 0) && t.tm_year > 120 && t.tm_min != lastMin) {
    lastMin = t.tm_min;
    drawClock();
    if (pageIds[page] == PG_SUN || pageIds[page] == PG_INFO) drawAll();   // Countdown, Laufzeit minuetlich auffrischen
    else drawFooter();                                                    // Alter der Daten
  }
  updateNight();
  delay(20);
}
