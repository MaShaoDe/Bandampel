# Status Bandampel

Höchstens 30 Zeilen. Immer überschreiben, nicht anhängen. Erledigtes streichen.

## Stand
- Repo steht (18.11.2026): README, Lizenzen, Ordner firmware, hardware, docs. Hilfstool `firmware/tools/cyd_id` vorhanden.
- Phase 1: Board ESP32-2432S028 (CYD, 1x USB-C und 1x Micro-USB). Das Werksprogramm (lvgl-demo) meldet ILI9341, Chip Revision 3, 4MB Flash, Touch XPT2046 (GPIO 36 und 39). Display ist damit ILI9341, Auflösung 320x240 quer.
- Firmware V1 geschrieben (PR offen, ungetestet): LovyanGFX, WLAN-Liste in secrets.h, HamQSL, 3 Seiten (Übersicht, Bänder, Details), Umschalten per Touch oder BOOT-Taste. Anleitung: docs/Firmware_V1.md.
- Optik V2 (PR 5, ungetestet): dunkle Karten mit Farbrahmen, 9 Bänder (160m bis 10m) als schmale Balken ohne Text, Tag/Nacht als Symbole, Rufzeichen optional in secrets.h. 160m ist eine Schätzung aus 80m-40m Nacht und K-Index (Vorschlag, offen).
- Nachtmodus (PR 6, ungetestet): Lichtsensor GPIO 34, Schwelle in secrets.h, ohne Schwelle nur Messwerte. Polarität des Sensors noch nicht gemessen (offen).
- Betriebsart-Seite (PR offen, ungetestet): CW und SSB pro Band, Faustregel CW eine Stufe besser als SSB, Vorschlag, offen.
- Port am MacBook: `/dev/cu.wchusbserial110`. Werkzeug: arduino-cli, FQBN `esp32:esp32:esp32`.

## Als Nächstes
- Marcel: V1 auf dem CYD testen (Farben, Drehung, Touch, Feed). Rückmeldung mit Seriellem Log.
- Danach Drehgeber (GPIO 22, 27, 35), Nachtmodus per Lichtsensor (GPIO 34), RGB-LED als Ampel. Schwellen für die Farben sind Vorschlag.
- Phase 2: Trägerplatine (KiCad), Phase 3: Gehäuse (OpenSCAD), erst wenn Phase 1 läuft.

## Entscheide (Marcel Sauder)
- 18.11.2026: Repo heisst Bandampel. Doku Deutsch zuerst, kurze englische Zusammenfassung. Firmware GPL-3.0-only.
- 18.11.2026: Firmware zuerst ohne Websuche aus bekanntem HamQSL-Format schreiben (offen: Format am Gerät prüfen).

## Stolpersteine
- Das Display-ID-Tool liest über MISO (GPIO 12). Das Werksprogramm setzt MISO auf -1, Auslesen kann daher Nullen liefern. Display ist bereits als ILI9341 bekannt.
- Das Hochladen überschreibt das Werksprogramm des Boards.
- Claude kann hier nicht kompilieren oder testen, nur Marcel am Gerät.
