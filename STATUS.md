# Status Bandampel

Höchstens 30 Zeilen. Immer überschreiben, nicht anhängen. Erledigtes streichen.

## Stand
- Läuft auf dem CYD (ESP32-2432S028, ILI9341, Port `/dev/cu.wchusbserial110`, arduino-cli, FQBN `esp32:esp32:esp32`). Stand 18.11.2026.
- Läuft: Übersicht, Prognose Bänder, Prognose Betriebsart (SSB oben, CW unten), Details. Auto-Wechsel alle 10 s, langer Druck schaltet auf Halten. Kopfzeile: Rufzeichen, Zürcher Zeit und UTC. WLAN-Liste und Rufzeichen in secrets.h.
- Neu (PR offen, ungetestet): Live-Seite mit gemessener WSPR-Aktivität aller 9 Bänder inkl. 160m (wspr.live, Abruf alle 15 min, Region JN und JO). Prognose-Seiten ohne 160m. Alle Seiten und SSB/CW einzeln in secrets.h schaltbar.
- Abfrage geprüft: `db1.wspr.live/?query=` liefert Bandcodes (1=160m, 3=80m, 7=40m, 10=30m, 14=20m, 18=17m, 21=15m, 24=12m, 28=10m). Der Regionsfilter (JN, JO) ist noch nicht verifiziert, Prüfung am Gerät über den seriellen Monitor.
- Farbschwellen der Live-Seite sind geschätzt (Vorschlag), nach Beobachtung einstellen. CW und SSB sind eine Faustregel (Vorschlag, offen).
- Nachtmodus: Sensor GPIO 34, dunkel = grosser Wert, Schwelle 400 in secrets.h. Test im Dunkeln offen.

## Als Nächstes
- Marcel: Live-Seite testen (serielle Ausgabe `wspr.live`), Haus-WLAN freigeben (MAC `a4:f0:0f:5b:e9:9c`), Nachtmodus testen.
- Offen: Schwellen der Live-Seite einstellen. Sonnenauf- und -untergang per Locator JN47. Einstellungsseite für WLAN per Browser. Farben Türkis statt Grün und Rosa statt Rot (rgb_order oder invert in lgfx_cyd.h).
- Phase 2: Drehgeber (GPIO 22, 27, 35) und Trägerplatine (KiCad), Phase 3: Gehäuse (OpenSCAD).

## Entscheide (Marcel Sauder)
- 18.11.2026: Repo heisst Bandampel. Doku Deutsch zuerst, kurze englische Zusammenfassung. Firmware GPL-3.0-only.
- 18.11.2026: SSB hat Priorität (oben), konfigurierbar. Prognose und gemessene Daten auf getrennten Seiten, 160m nur live. Alle Seiten in secrets.h schaltbar.

## Stolpersteine
- `arduino-cli upload` schreibt bei einem Fehler im Kompilieren die alte Version. Immer mit `&&` verbinden.
- Das Hochladen überschreibt das Werksprogramm des Boards.
- Claude kann hier nicht kompilieren oder testen, nur Marcel am Gerät.
