// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 FerienBSD aka Marcel Sauder
// Web-Menue (Konfigurator) und Notfall-WLAN. Benutzt cfg aus settings.h.
// Erreichbar unter http://bandampel-001.local oder ueber die IP in der Fusszeile.
// Ohne bekanntes WLAN oeffnet das Geraet nach 60 Sekunden das WLAN "Bandampel-Setup" (http://192.168.4.1).
#pragma once
#include <WebServer.h>
#include <ESPmDNS.h>

static WebServer web(80);
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

static String fSel(const String& label, const String& name, int cur, const char* opts[], int n) {
  String s = "<label>" + label + "<select name=" + name + ">";
  for (int i = 0; i < n; i++) s += String("<option value=") + i + (cur == i ? " selected" : "") + ">" + opts[i] + "</option>";
  return s + "</select></label>";
}

static String statusRow(const char* k, const String& v) {
  return String("<div class=kv><span>") + k + "</span><b>" + he(v) + "</b></div>";
}

static void webRoot() {
  if (!webAuth()) return;
  bool ok = WiFi.status() == WL_CONNECTED;
  String h;
  h.reserve(14000);
  h += F("<!DOCTYPE html><html lang=de><head><meta charset=utf-8>"
         "<meta name=viewport content='width=device-width,initial-scale=1'><title>Bandampel</title><style>"
         "*{box-sizing:border-box}body{margin:0;font:16px/1.4 system-ui,sans-serif;background:#0b0f18;color:#e8ecf4;padding-bottom:84px}"
         "header{padding:12px 14px 4px}h1{font-size:20px;margin:0}header small{color:#8a94a8}"
         "nav{position:sticky;top:0;z-index:5;display:flex;flex-wrap:wrap;gap:6px;padding:8px 10px;background:#0b0f18;border-bottom:1px solid #243049}"
         "nav button{flex:1 1 auto;min-height:44px;padding:8px 12px;border:1px solid #243049;border-radius:10px;background:#151d2e;color:#cfd6e6;font-size:15px}"
         "nav button.on{background:#fc0;color:#111;border-color:#fc0;font-weight:600}"
         "main{max-width:720px;margin:0 auto;padding:10px}.tab{display:none}"
         ".card{background:#151d2e;border:1px solid #243049;border-radius:12px;padding:12px 14px;margin:10px 0}"
         ".card h2{font-size:15px;margin:0 0 8px;color:#fc0}"
         ".g{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:8px 12px}"
         "label{display:block;font-size:14px;color:#aab3c5}label.c{display:flex;gap:8px;align-items:center;min-height:36px;color:#e8ecf4}"
         "input[type=text],input[type=password],input[type=number],select{width:100%;min-height:44px;margin-top:3px;padding:8px 10px;font-size:16px;"
         "background:#0b0f18;color:#e8ecf4;border:1px solid #2c3956;border-radius:8px}"
         "input[type=checkbox]{width:22px;height:22px}"
         "button.b{min-height:44px;padding:8px 14px;font-size:15px;border:0;border-radius:10px;background:#2c3956;color:#e8ecf4;margin:6px 6px 0 0}"
         "button.s{background:#fc0;color:#111;font-weight:600}button.d{background:#8a2a2a}"
         ".kv{display:flex;justify-content:space-between;gap:10px;padding:6px 0;border-bottom:1px solid #1f2a40}.kv:last-child{border:0}.kv span{color:#8a94a8}"
         "small{color:#8a94a8;display:block;margin-top:6px}"
         ".bar{position:fixed;left:0;right:0;bottom:0;padding:10px;background:#0b0f18;border-top:1px solid #243049;text-align:center}"
         ".bar button{width:100%;max-width:700px;min-height:48px;font-size:17px}"
         "@media(orientation:landscape) and (max-height:500px){body{padding-bottom:70px}.bar{padding:6px}}"
         "</style></head><body>");
  h += "<header><h1>" + he(cfgHostname()) + " <small>" + he(cfg.callsign) + "</small></h1></header>";
  h += F("<nav><button type=button>Status</button><button type=button>Anzeige</button><button type=button>Helligkeit</button>"
         "<button type=button>WLAN</button><button type=button>Funk</button><button type=button>System</button><button type=button>Über</button></nav>");
  h += "<main><form id=f method=post action=/save onsubmit=\"return confirm('Einstellungen speichern und Gerät neu starten?')\">";

  // 1 Status (nur lesen)
  long up = (long)(millis() / 1000UL);
  String upt = (up >= 86400) ? String(up / 86400) + " d " + String((up % 86400) / 3600) + " h"
             : (up >= 3600  ? String(up / 3600) + " h " + String((up % 3600) / 60) + " min" : String(up / 60) + " min");
  String age = haveData ? dataAgeText() : String("");
  h += "<section class=tab><div class=card><h2>Gerät</h2>";
  h += statusRow("WLAN", ok ? WiFi.SSID() : (apActive ? String("Notfall: Bandampel-Setup") : String("getrennt")));
  h += statusRow("IP-Adresse", ok ? WiFi.localIP().toString() : (apActive ? WiFi.softAPIP().toString() : String("--")));
  h += statusRow("Signal", ok ? String(WiFi.RSSI()) + " dBm" : String("--"));
  h += statusRow("Version", FW_VERSION);
  h += statusRow("Name im Netz", cfgHostname() + ".local");
  h += statusRow("Daten", age.length() ? String("vor ") + age : String("--"));
  h += statusRow("Laufzeit", upt);
  h += statusRow("Seite auf dem Display", String(page + 1) + " von " + String(pageCount));
  h += F("</div><button type=button class=b onclick='location.reload()'>Aktualisieren</button></section>");

  // 2 Anzeige
  h += "<section class=tab><div class=card><h2>Seiten</h2><div class=g>";
  h += fCheck("Ampel (Übersicht)", "pOv", cfg.pOverview) + fCheck("Sonne", "pSu", cfg.pSun) + fCheck("Bänder (Prognose)", "pFc", cfg.pForecast)
     + fCheck("Modus (SSB, CW)", "pMo", cfg.pModes) + fCheck("Live (Aktivität)", "pLi", cfg.pLive) + fCheck("Details", "pDe", cfg.pDetails)
     + fCheck("Info (WLAN, IP)", "pIn", cfg.pInfo) + fCheck("Lizenz (QR-Code)", "pAb", cfg.pAbout);
  h += "</div><small>Der Name steht auf dem Display in der Kopfzeile neben dem Rufzeichen.</small></div><div class=card><h2>Ablauf und Betriebsart</h2><div class=g>";
  h += fInput("Seitenwechsel (Sekunden)", "automs", String(cfg.autoMs / 1000), "number", "min=2 max=600");
  h += "</div>" + fCheck("SSB zeigen", "ssb", cfg.showSsb) + fCheck("CW zeigen", "cw", cfg.showCw) + fCheck("SSB oben, CW unten", "ssbf", cfg.ssbFirst);
  h += "</div></section>";

  // 3 Helligkeit
  const char* modes[3] = { "Fest (Tageswert)", "Tag oder Nacht mit Schwelle", "Stufenlos nach Lichtsensor" };
  h += "<section class=tab><div class=card><h2>Helligkeit</h2>";
  h += fSel("Modus", "bmode", cfg.brightMode, modes, 3);
  h += "<div class=g>" + fInput("Tag (5 bis 100 %)", "bp", String(cfg.brightPct), "number", "min=5 max=100")
     + fInput("Nacht (5 bis 100 %)", "bnp", String(cfg.brightNightPct), "number", "min=5 max=100") + "</div>";
  h += F("<small>Weniger Helligkeit spart Strom und Wärme. Schnellwahl: 100, 75, 50, 25, 10 Prozent.</small></div>");
  h += "<div class=card><h2>Lichtsensor</h2><div class=g>" + fInput("Sensorwert hell", "ldrb", String(cfg.ldrBright), "number")
     + fInput("Sensorwert dunkel", "ldrd", String(cfg.ldrDark), "number") + "</div>";
  h += F("<button type=button class=b onclick=\"fetch('/ldr').then(r=>r.text()).then(v=>document.getElementsByName('ldrb')[0].value=v)\">Aktuell als hell</button>"
         "<button type=button class=b onclick=\"fetch('/ldr').then(r=>r.text()).then(v=>document.getElementsByName('ldrd')[0].value=v)\">Aktuell als dunkel</button>"
         "<small>Kalibrieren: Raum hell, «als hell» drücken. Sensor abdecken, «als dunkel» drücken. Dann speichern.</small></div>");
  h += "<div class=card><h2>Nur für «Tag oder Nacht»</h2><div class=g>" + fInput("Schwelle (-1 = aus)", "ldrt", String(cfg.ldrThreshold), "number")
     + fInput("Toleranz", "ldrh", String(cfg.ldrHyst), "number") + "</div>" + fCheck("Grosser Messwert = dunkel", "ldrn", cfg.ldrNightHigh);
  h += "<small>Aktueller Sensorwert: " + String(ldrRaw) + "</small></div></section>";

  // 4 WLAN
  h += "<section class=tab><div class=card><h2>WLAN (bis 5, Reihenfolge = Vorrang)</h2>";
  for (int i = 0; i < CFG_MAX_WIFI; i++) {
    h += "<div class=g>" + fInput("Name " + String(i + 1), "ws" + String(i), i < cfg.wifiCount ? cfg.wifiSsid[i] : String(""), "text", "list=nets autocomplete=off")
       + fInput("Passwort (leer = unverändert)", "wp" + String(i), "", "password", "autocomplete=new-password") + "</div>";
  }
  h += F("<datalist id=nets></datalist><button type=button class=b onclick=scan()>Netze suchen</button><small id=sc></small>"
         "<small>Nur 2,4 GHz. Beim iPhone-Hotspot «Kompatibilität maximieren» einschalten. Das Suchen kann die Verbindung kurz stören. "
         "Nach der Suche in das Namensfeld tippen und ein Netz wählen. Beim Ändern eines Namens das Passwort neu eingeben.</small></div>");
  h += "<div class=card><h2>Wiederverbindung</h2>" + fInput("Pause zwischen Versuchen (Sekunden, 5 bis 600)", "wretry", String(cfg.wifiRetryS), "number", "min=5 max=600") + "</div></section>";

  // 5 Funk
  h += "<section class=tab><div class=card><h2>Station</h2><div class=g>" + fInput("Rufzeichen (Kopfzeile)", "call", cfg.callsign)
     + fInput("Zeitzone (POSIX)", "tz", cfg.tz) + "</div></div>";
  h += "<div class=card><h2>Standort für die Sonnenzeiten</h2><div class=g>" + fInput("Breitengrad", "lat", String(cfg.sunLat, 3)) + fInput("Längengrad", "lon", String(cfg.sunLon, 3)) + "</div></div>";
  h += "<div class=card><h2>Live-Seite (WSPR)</h2><div class=g>" + fInput("Locator-Felder der Region", "lf", cfg.liveFields)
     + fInput("Zeitfenster (Minuten, 5 bis 120)", "lmin", String(cfg.liveMinutes), "number") + "</div>";
  h += F("<small>Schwellen in Spots im Zeitfenster: ab «mittel» gelb, ab «gut» grün. «Aktuell» ist die gemessene Zahl zur Kalibrierung.</small>");
  for (int i = 0; i < 9; i++) {
    h += "<div class=g style='grid-template-columns:1fr 1fr 1fr'>" + fInput(String(LIVE_LABELS[i]) + " m mittel", "lfa" + String(i), String(cfg.liveFair[i]), "number")
       + fInput("gut", "lgo" + String(i), String(cfg.liveGood[i]), "number")
       + "<label>aktuell<input type=text value=\"" + (haveLive ? String(liveCount[i]) : String("--")) + "\" disabled></label></div>";
  }
  h += "</div></section>";

  // 6 System (Passwort zuletzt)
  h += "<section class=tab><div class=card><h2>Gerät</h2><div class=g>" + fInput("Gerätenummer (1 bis 999)", "unit", String(cfg.unit), "number", "min=1 max=999") + "</div>";
  h += F("<small>Der Name im Netz heisst bandampel-NNN.local. Mit verschiedenen Nummern laufen mehrere Geräte parallel.</small></div>");
  h += "<div class=card><h2>Menü-Passwort</h2>" + fInput("Neues Passwort (leer = unverändert)", "pass", "", "password", "autocomplete=new-password")
     + fCheck("Passwort entfernen", "passclear", false);
  h += F("<small>Benutzername im Browser: admin. Das Passwort gilt auch für das Notfall-WLAN, wenn es mindestens 8 Zeichen hat. Übertragung unverschlüsselt (HTTP).</small></div>");
  h += F("<div class=card><h2>Werkseinstellung</h2>"
         "<label>Bestätigung: Menü-Passwort, wenn keines gesetzt ist 000000<input type=password name=code form=rf autocomplete=off></label>"
         "<button type=submit form=rf class='b d'>Alle Einstellungen löschen</button>"
         "<small>Danach gelten wieder die Werte aus secrets.h. Notausgang am Gerät: BOOT-Taste 10 Sekunden halten.</small></div></section>");

  // 7 Ueber (fest im Code, nicht aenderbar)
  h += F("<section class=tab><div class=card><h2>Bandampel</h2>"
         "<div class=kv><span>Version</span><b>" FW_VERSION "</b></div>"
         "<div class=kv><span>Urheber</span><b>FerienBSD aka Marcel Sauder</b></div>"
         "<div class=kv><span>Rufzeichen</span><b>HE9QCU</b></div>"
         "<div class=kv><span>Projekt</span><b><a style='color:#fc0' href='" FW_URL "' target=_blank>GitHub</a></b></div>"
         "<small>Fehler melden und Ideen einbringen: auf GitHub unter «Issues».</small></div>"
         "<div class=card><h2>Lizenzen</h2>"
         "<div class=kv><span>Firmware</span><b>GPL-3.0-only</b></div>"
         "<div class=kv><span>Hardware</span><b>CERN-OHL-P-2.0</b></div>"
         "<div class=kv><span>Dokumentation</span><b>CC-BY-4.0</b></div>"
         "<small>Copyright (c) 2026 FerienBSD aka Marcel Sauder. Dieses Programm ist freie Software und darf unter den Bedingungen der GPL "
         "weitergegeben und verändert werden. Es gibt keine Gewährleistung. Den Quelltext gibt es auf GitHub. "
         "Der Urhebervermerk muss erhalten bleiben.</small></div>"
         "<div class=card><h2>Datenquellen</h2>"
         "<small>Sonnen- und Bandwerte: HamQSL (hamqsl.com, N0NBH) mit Daten von NOAA. Live-Aktivität: wspr.live (WSPR-Spots). "
         "Vielen Dank an die Betreiber. Die Daten werden höchstens alle 15 bis 30 Minuten abgefragt.</small></div>"
         "<div class=card><h2>Verwendete Software</h2>"
         "<small>LovyanGFX (Anzeige) und das ESP32-Arduino-Paket von Espressif, jeweils unter ihren eigenen Open-Source-Lizenzen. "
         "Die Einstufungen der Bandbedingungen sind Faustregeln und keine Prognose.</small></div></section>");

  h += F("</form><form id=rf method=post action=/reset></form></main>"
         "<div class=bar><button type=submit form=f class='b s'>Speichern und neu starten</button></div>"
         "<script>function T(i){var t=document.querySelectorAll('.tab'),b=document.querySelectorAll('nav button');"
         "for(var k=0;k<t.length;k++){t[k].style.display=k==i?'block':'none';b[k].className=k==i?'on':''}try{history.replaceState(0,'','#'+i)}catch(e){}}"
         "document.querySelectorAll('nav button').forEach(function(b,i){b.onclick=function(){T(i)}});"
         "function scan(){var s=document.getElementById('sc');s.textContent='Suche ...';fetch('/scan').then(function(r){return r.text()}).then(function(x){"
         "var d=document.getElementById('nets');d.innerHTML='';var n=0;x.split('\\n').forEach(function(l){if(!l)return;var p=l.split('\\t');"
         "var o=document.createElement('option');o.value=p[0];o.label=p[0]+' ('+p[1]+' dBm)';d.appendChild(o);n++});"
         "s.textContent=n+' Netze gefunden.'}).catch(function(){s.textContent='Suche fehlgeschlagen'})}"
         "T(parseInt((location.hash||'#0').slice(1))||0)</script></body></html>");
  web.send(200, "text/html; charset=utf-8", h);
}

static void webScan() {
  if (!webAuth()) return;
  int n = WiFi.scanNetworks();
  String out;
  for (int i = 0; i < n; i++) {
    String s = WiFi.SSID(i);
    if (!s.length()) continue;
    int r = WiFi.RSSI(i);
    bool skip = false;
    for (int j = 0; j < n; j++) {
      if (j == i || WiFi.SSID(j) != s) continue;
      if (WiFi.RSSI(j) > r || (WiFi.RSSI(j) == r && j < i)) { skip = true; break; }   // nur das staerkste je Name
    }
    if (skip) continue;
    out += s + "\t" + String(r) + "\n";
  }
  WiFi.scanDelete();
  web.send(200, "text/plain; charset=utf-8", out);
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
  cfg.wifiRetryS = web.arg("wretry").toInt();

  cfg.pOverview = argOn("pOv"); cfg.pForecast = argOn("pFc"); cfg.pModes = argOn("pMo");
  cfg.pLive = argOn("pLi");     cfg.pDetails = argOn("pDe");
  cfg.showSsb = argOn("ssb");   cfg.showCw = argOn("cw");
  cfg.ssbFirst = argOn("ssbf"); cfg.pSun = argOn("pSu"); cfg.pInfo = argOn("pIn"); cfg.pAbout = argOn("pAb");
  cfg.autoMs = web.arg("automs").toInt() * 1000;
  cfg.sunLat = web.arg("lat").toFloat(); cfg.sunLon = web.arg("lon").toFloat();
  cfg.brightMode = web.arg("bmode").toInt();
  cfg.brightPct = web.arg("bp").toInt(); cfg.brightNightPct = web.arg("bnp").toInt();
  cfg.ldrBright = web.arg("ldrb").toInt(); cfg.ldrDark = web.arg("ldrd").toInt();
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
  web.on("/scan", HTTP_GET, webScan);
  web.on("/ldr", HTTP_GET, []() { if (!webAuth()) return; web.send(200, "text/plain", String(ldrRaw)); });
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
