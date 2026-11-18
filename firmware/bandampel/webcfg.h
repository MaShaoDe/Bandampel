// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 FerienBSD aka Marcel Sauder
// Web-Menue (Konfigurator) und Notfall-WLAN. Benutzt cfg aus settings.h.
// Erreichbar unter http://bandampel-001.local oder ueber die IP in der Fusszeile.
// Ohne bekanntes WLAN oeffnet das Geraet nach 60 Sekunden das WLAN "Bandampel-Setup" (http://192.168.4.1).
#pragma once
#include <WebServer.h>
#include <ESPmDNS.h>

static WebServer web(80);
static bool      apActive    = false;
static bool      mdnsStarted = false;
static uint32_t  restartAt   = 0;

static String he(const String& s) {   // HTML maskieren
  String o;
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') o += "&amp;"; else if (c == '<') o += "&lt;"; else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;"; else o += c;
  }
  return o;
}

static bool webAuth() {
  if (cfg.menuPass.length() == 0) return true;
  if (web.authenticate("admin", cfg.menuPass.c_str())) return true;
  web.requestAuthentication(BASIC_AUTH, "Bandampel");
  return false;
}

static String fInput(const String& label, const String& name, const String& val, const char* type = "text", const char* extra = "") {
  return "<label>" + label + "<input type=" + type + " name=" + name + " value=\"" + he(val) + "\" " + extra + "></label>";
}
static String fCheck(const String& label, const String& name, bool on) {
  return String("<label class=c><input type=checkbox name=") + name + (on ? " checked" : "") + "> " + label + "</label>";
}

static void webRoot() {
  if (!webAuth()) return;
  String h;
  h.reserve(9000);
  h += F("<!DOCTYPE html><html lang=de><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
         "<title>Bandampel</title><style>body{font-family:sans-serif;max-width:560px;margin:0 auto;padding:12px;background:#111;color:#eee}"
         "h1{font-size:20px}h2{font-size:16px;margin:18px 0 6px;color:#fc0;border-bottom:1px solid #444}"
         "label{display:block;margin:6px 0;font-size:14px}input[type=text],input[type=password],input[type=number]{width:100%;box-sizing:border-box;padding:6px;margin-top:2px}"
         ".c{display:flex;gap:6px;align-items:center}.r{display:flex;gap:6px}.r label{flex:1}button{padding:10px 16px;font-size:16px;margin:10px 6px 0 0}"
         "small{color:#999}</style></head><body>");
  h += "<h1>" + he(cfgHostname()) + " " + he(cfg.callsign) + "</h1><form method=post action=/save>";

  h += "<h2>Gerät</h2>";
  h += fInput("Rufzeichen (Kopfzeile)", "call", cfg.callsign);
  h += fInput("Gerätenummer 1 bis 999 (Name bandampel-NNN)", "unit", String(cfg.unit), "number", "min=1 max=999");
  h += fInput("Zeitzone (POSIX)", "tz", cfg.tz);
  h += fInput("Neues Menü-Passwort (leer = unverändert)", "pass", "", "password");
  h += fCheck("Menü-Passwort entfernen", "passclear", false);
  h += F("<small>Benutzername im Browser: admin. Das Passwort gilt auch für das Notfall-WLAN, wenn es mindestens 8 Zeichen hat.</small>");

  h += "<h2>WLAN (bis 5, Reihenfolge = Vorrang)</h2>";
  for (int i = 0; i < CFG_MAX_WIFI; i++) {
    h += "<div class=r>" + fInput("Name " + String(i + 1), "ws" + String(i), i < cfg.wifiCount ? cfg.wifiSsid[i] : String(""))
       + fInput("Passwort (leer = unverändert)", "wp" + String(i), "", "password") + "</div>";
  }
  h += F("<small>Nur 2,4 GHz. Beim iPhone-Hotspot «Kompatibilität maximieren» einschalten.</small>");

  h += "<h2>Seiten</h2>";
  h += fCheck("Übersicht", "pOv", cfg.pOverview) + fCheck("Sonne (Aufgang, Untergang, Grayline)", "pSu", cfg.pSun) + fCheck("Prognose Bänder", "pFc", cfg.pForecast)
     + fCheck("Betriebsart", "pMo", cfg.pModes) + fCheck("Live-Aktivität", "pLi", cfg.pLive) + fCheck("Details", "pDe", cfg.pDetails);
  h += fInput("Seitenwechsel im Auto-Modus (Sekunden)", "automs", String(cfg.autoMs / 1000), "number", "min=2 max=600");
  h += fCheck("SSB zeigen", "ssb", cfg.showSsb) + fCheck("CW zeigen", "cw", cfg.showCw)
     + fCheck("SSB oben, CW unten", "ssbf", cfg.ssbFirst);
  h += "<h2>Standort für die Sonnenzeiten</h2><div class=r>" + fInput("Breitengrad", "lat", String(cfg.sunLat, 3), "text") + fInput("Längengrad", "lon", String(cfg.sunLon, 3), "text") + "</div>";

  h += "<h2>Helligkeit und Nachtmodus</h2>";
  h += "<div class=r>" + fInput("Helligkeit Tag (10 bis 255)", "br", String(cfg.brightness), "number") + fInput("Helligkeit Nacht (0 bis 255)", "brn", String(cfg.brightnessNight), "number") + "</div>";
  h += "<div class=r>" + fInput("Schwelle (-1 = aus)", "ldrt", String(cfg.ldrThreshold), "number") + fInput("Toleranz", "ldrh", String(cfg.ldrHyst), "number") + "</div>";
  h += fCheck("Grosser Messwert = dunkel", "ldrn", cfg.ldrNightHigh);
  h += "<small>Aktueller Messwert Lichtsensor: " + String(ldrRaw) + " (Seite neu laden zum Aktualisieren). Schwelle in die Mitte zwischen hell und dunkel legen.</small>";

  h += "<h2>Live-Seite (WSPR)</h2>";
  h += fInput("Locator-Felder der Region", "lf", cfg.liveFields);
  h += fInput("Zeitfenster (Minuten, 5 bis 120)", "lmin", String(cfg.liveMinutes), "number");
  h += F("<small>Schwellen in Spots im Zeitfenster: ab «mittel» gelb, ab «gut» grün. Zur Kalibrierung steht die aktuelle Zahl dahinter.</small>");
  for (int i = 0; i < 9; i++) {
    h += "<div class=r>" + fInput(String(LIVE_LABELS[i]) + " m mittel", "lfa" + String(i), String(cfg.liveFair[i]), "number")
       + fInput("gut", "lgo" + String(i), String(cfg.liveGood[i]), "number")
       + "<label>aktuell<input type=text value=\"" + (haveLive ? String(liveCount[i]) : String("--")) + "\" disabled></label></div>";
  }

  h += F("<button type=submit>Speichern und neu starten</button></form>"
         "<h2>Werkseinstellung</h2><form method=post action=/reset>"
         "<label>Bestätigung: Menü-Passwort eingeben, wenn keines gesetzt ist 000000"
         "<input type=password name=code autocomplete=off></label>"
         "<button type=submit>Alle Einstellungen löschen</button></form></body></html>");
  web.send(200, "text/html; charset=utf-8", h);
}

static bool argOn(const char* n) { return web.hasArg(n); }

static void webSave() {
  if (!webAuth()) return;
  cfg.callsign = web.arg("call"); cfg.callsign.trim();
  cfg.unit = web.arg("unit").toInt();
  cfg.tz = web.arg("tz"); cfg.tz.trim();
  String np = web.arg("pass");
  if (argOn("passclear")) cfg.menuPass = "";
  else if (np.length()) cfg.menuPass = np.substring(0, 32);

  String oldS[CFG_MAX_WIFI], oldP[CFG_MAX_WIFI];
  int oldN = cfg.wifiCount;
  for (int i = 0; i < CFG_MAX_WIFI; i++) { oldS[i] = cfg.wifiSsid[i]; oldP[i] = cfg.wifiPass[i]; }
  int n = 0;
  for (int i = 0; i < CFG_MAX_WIFI; i++) {
    String s = web.arg("ws" + String(i)); s.trim();
    if (!s.length()) continue;
    String p = web.arg("wp" + String(i));
    if (!p.length()) { for (int k = 0; k < oldN; k++) if (oldS[k] == s) p = oldP[k]; }   // Passwort unveraendert
    cfg.wifiSsid[n] = s; cfg.wifiPass[n] = p; n++;
  }
  cfg.wifiCount = n;

  cfg.pOverview = argOn("pOv"); cfg.pForecast = argOn("pFc"); cfg.pModes = argOn("pMo");
  cfg.pLive = argOn("pLi");     cfg.pDetails = argOn("pDe");
  cfg.showSsb = argOn("ssb");   cfg.showCw = argOn("cw");
  cfg.ssbFirst = argOn("ssbf"); cfg.pSun = argOn("pSu");
  cfg.autoMs = web.arg("automs").toInt() * 1000;
  cfg.sunLat = web.arg("lat").toFloat(); cfg.sunLon = web.arg("lon").toFloat();
  cfg.brightness = web.arg("br").toInt(); cfg.brightnessNight = web.arg("brn").toInt();
  cfg.ldrThreshold = web.arg("ldrt").toInt(); cfg.ldrHyst = web.arg("ldrh").toInt();
  cfg.ldrNightHigh = argOn("ldrn");
  cfg.liveFields = web.arg("lf"); cfg.liveMinutes = web.arg("lmin").toInt();
  for (int i = 0; i < 9; i++) {
    cfg.liveFair[i] = max(0L, web.arg("lfa" + String(i)).toInt());
    cfg.liveGood[i] = max(1L, web.arg("lgo" + String(i)).toInt());
  }
  cfgSave();
  web.sendHeader("Location", "/saved");   // Post-Redirect-Get: Reload sendet das Formular nicht nochmals
  web.send(303, "text/plain", "");
  restartAt = millis() + 3000;
}

static void webReset() {
  if (!webAuth()) return;
  String need = cfg.menuPass.length() ? cfg.menuPass : String("000000");
  if (web.arg("code") != need) {
    web.send(403, "text/html; charset=utf-8",
             "<!DOCTYPE html><html lang=de><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
             "<body style='font-family:sans-serif;background:#111;color:#eee;padding:20px'><h2>Falsche Bestätigung</h2>"
             "<p>Nichts wurde gelöscht.</p><p><a style='color:#fc0' href='/'>Zurück zum Menü</a></p></body></html>");
    return;
  }
  cfgFactoryReset();
  web.sendHeader("Location", "/saved");
  web.send(303, "text/plain", "");
  restartAt = millis() + 3000;
}

// Neutrale Seite nach Speichern oder Werkseinstellung (GET, ohne Passwort, ohne Einstellungen)
static void webSaved() {
  web.send(200, "text/html; charset=utf-8",
           "<!DOCTYPE html><html lang=de><head><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'>"
           "<meta http-equiv=refresh content='15;url=/'></head>"
           "<body style='font-family:sans-serif;background:#111;color:#eee;padding:20px'><h2>Gespeichert</h2>"
           "<p>Das Gerät startet neu. Diese Seite lädt in 15 Sekunden das Menü.</p>"
           "<p><a style='color:#fc0' href='/'>Zum Menü</a></p></body></html>");
}

static void webInit() {
  web.on("/saved", HTTP_GET, webSaved);
  web.on("/", HTTP_GET, webRoot);
  web.on("/save", HTTP_POST, webSave);
  web.on("/reset", HTTP_POST, webReset);
  web.onNotFound([]() { web.sendHeader("Location", "/"); web.send(302, "text/plain", ""); });
  web.begin();
}

static void webLoop() {
  web.handleClient();
  if (restartAt && (int32_t)(millis() - restartAt) >= 0) ESP.restart();
  if (!mdnsStarted && WiFi.status() == WL_CONNECTED) {
    if (MDNS.begin(cfgHostname().c_str())) {
      MDNS.addService("http", "tcp", 80);
      mdnsStarted = true;
      Serial.printf("mDNS: http://%s.local\n", cfgHostname().c_str());
    }
  }
}

// Notfall-WLAN
static void apStart() {
  WiFi.mode(WIFI_AP_STA);
  const char* pw = (cfg.menuPass.length() >= 8) ? cfg.menuPass.c_str() : nullptr;
  WiFi.softAP("Bandampel-Setup", pw);
  apActive = true;
  Serial.printf("Notfall-WLAN Bandampel-Setup%s, Menue: http://%s\n", pw ? " (mit Menue-Passwort)" : " (offen)", WiFi.softAPIP().toString().c_str());
}
static void apStop() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apActive = false;
  Serial.println("Notfall-WLAN beendet");
}
