# Bandampel

*HF-Ausbreitung auf einen Blick: Welches Band läuft gerade, und wofür?*

Ein kleines ESP32-Gerät mit 2,8-Zoll-Farbdisplay, das Solardaten und Bandbedingungen für den Amateurfunk als Farbampel darstellt: 80m bis 10m, Tag und Nacht, grün, gelb, rot. Dazu SFI, K-Index, A-Index, Sunspots und weitere Werte. Das Gerät holt die Daten selbst per WLAN (Heimnetz oder Handy-Hotspot).

## Status

| Phase | Inhalt | Stand |
|---|---|---|
| 1 | Firmware für das fertige Board ESP32-2432S028 ("Cheap Yellow Display", CYD) | fertig, eingefroren (1.18.x) |
| 2 | Eigene Trägerplatine (Drehgeber, Entlastung der Stecker) | geplant |
| 3 | 3D-gedrucktes Gehäuse als Komplettlösung | geplant |

## Geplante Funktionen

- Übersicht mit Farbkacheln für SFI, K, A und Sunspots
- Bandbalken 80m bis 10m, Tag und Nacht
- Detailseite: X-Ray, Sonnenwind, Noise, Geomagnetik
- Einstellungen: Helligkeit, WLAN, Rufzeichen, Locator
- Nachtmodus per Lichtsensor, RGB-LED als Ampel
- Bedienung per Touch und (später) Drehgeber

## Hardware

Erste Phase: ESP32-2432S028 (ESP32, 2,8 Zoll, 240x320, resistiver Touch). Die Display-Variante (ILI9341 oder ST7789) ermittelt das Tool in [firmware/tools/cyd_id](firmware/tools/cyd_id).

## Datenquellen

Die Bandprognose stammt von HamQSL (N0NBH), Rohdaten ergänzend von NOAA SWPC. Bitte die Quellen nicht öfter als nötig abfragen (alle 30 Minuten oder seltener reicht, HamQSL aktualisiert ca. alle 3 Stunden). Die Daten sind für nicht-kommerzielle Nutzung gedacht. 160m liefert HamQSL nicht, deshalb steht 160m nur auf der Live-Seite. Die Live-Aktivität stammt aus gemessenen WSPR-Spots von wspr.live (Abruf alle 15 Minuten, bitte nicht öfter).

## Nachbauen

Mit einem fertigen CYD-Board (ESP32-2432S028), einem USB-Kabel und etwa 30 Minuten: [docs/Nachbauen.md](docs/Nachbauen.md). Beschrieben sind beide Wege, Arduino IDE (mit Maus) und arduino-cli, für Windows und macOS, samt Flash-Aufteilung «Huge APP» und erstem Start.

## Benutzerhandbuch

Funktionsumfang, Bedienung und Einstellungen: [docs/Benutzerhandbuch.md](docs/Benutzerhandbuch.md). Technik zum Selbstbau: [docs/Firmware_V1.md](docs/Firmware_V1.md).

## Platinen und Bausätze

Bastlerprojekt. Platinen und Bausätze gibt es auf Anfrage zum Materialpreis, solange Vorrat, ohne Gewährleistung und ohne CE-Prüfung. Betrieb auf eigene Verantwortung. Anfragen über einen «Issue» auf GitHub oder an HE9QCU. Gemeinsame Lötworkshops (zum Beispiel im Funkclub oder Fab Lab) sind möglich, das Material wird zum Selbstkostenpreis weitergegeben. Es gibt keinen Verkauf mit Gewinn. Die Datenquellen (HamQSL, NOAA, wspr.live) sind für nicht-kommerzielle Nutzung gedacht.

## Lizenzen

Firmware GPL-3.0-only, Hardware CERN-OHL-P-2.0, Dokumentation CC-BY-4.0. Details in [LICENSE.md](LICENSE.md).

## English summary

Bandampel is a small ESP32 device with a 2.8 inch colour display that shows HF propagation for ham radio as a traffic light: solar data and band conditions from 80m to 10m, day and night. It fetches the data over Wi-Fi (home network or phone hotspot). Phase 1 targets the ESP32-2432S028 "Cheap Yellow Display", phase 2 a custom carrier board, phase 3 a 3D-printed case. Firmware is GPL-3.0-only, hardware CERN-OHL-P-2.0, documentation CC-BY-4.0. Band forecasts come from HamQSL, please poll sparingly. Hobby project: boards and kits are available on request at material cost while stocks last, without warranty and without CE conformity assessment, use at your own risk. Contact via a GitHub issue or call sign HE9QCU. The data sources are intended for non-commercial use.

Build guide (German): docs/Nachbauen.md.

Copyright (c) 2026 FerienBSD aka Marcel Sauder
