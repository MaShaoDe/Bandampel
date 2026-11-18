# Status Bandampel

Höchstens 30 Zeilen. Immer überschreiben, nicht anhängen. Erledigtes streichen.

## Stand
- Läuft auf dem CYD (ESP32-2432S028, ILI9341, Port `/dev/cu.wchusbserial110`, arduino-cli, FQBN `esp32:esp32:esp32`). Stand 18.11.2026.
- 4 Seiten: Übersicht, Bänder (160m bis 10m, Tag und Nacht), Betriebsart (SSB und CW), Details. Auto-Wechsel alle 10 s, langer Druck schaltet auf Halten. Kopfzeile: Rufzeichen, Zürcher Zeit und UTC. WLAN-Liste (Hotspot, Haus) und Rufzeichen in secrets.h.
- 160m ist eine Schätzung (hohl). CW und SSB sind eine Faustregel (CW eine Stufe besser, bei K ab 5 eine Stufe schlechter). Beides Vorschlag, offen.
- Nachtmodus: Sensor GPIO 34, dunkel = grosser Wert (hell 0 bis 174, abgedeckt ab 700), Schwelle 400 in secrets.h. Test im Dunkeln offen.
- SSB oben, CW unten (Standard), per `SSB_FIRST` in secrets.h umkehrbar (PR offen, ungetestet).

## Als Nächstes
- Marcel: Haus-WLAN freigeben (MAC `a4:f0:0f:5b:e9:9c`), Nachtmodus testen.
- Offen: Sonnenauf- und -untergang per Locator (aktuelle Zeile Tag oder Nacht hervorheben). Einstellungsseite für WLAN per Browser. Farben Türkis statt Grün und Rosa statt Rot (rgb_order oder invert in lgfx_cyd.h).
- Phase 2: Drehgeber (GPIO 22, 27, 35) und Trägerplatine (KiCad), Phase 3: Gehäuse (OpenSCAD).

## Entscheide (Marcel Sauder)
- 18.11.2026: Repo heisst Bandampel. Doku Deutsch zuerst, kurze englische Zusammenfassung. Firmware GPL-3.0-only.
- 18.11.2026: SSB hat Priorität (oben), konfigurierbar in secrets.h.

## Stolpersteine
- `arduino-cli upload` schreibt bei einem Fehler im Kompilieren die alte Version. Immer mit `&&` verbinden.
- Das Hochladen überschreibt das Werksprogramm des Boards.
- Claude kann hier nicht kompilieren oder testen, nur Marcel am Gerät.
