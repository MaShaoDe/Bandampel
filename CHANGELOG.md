<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright (c) 2026 FerienBSD aka Marcel Sauder -->
# Changelog

Alle Versionen von Bandampel (CYD-Ausführung), neueste zuerst. Die Zeiten sind UTC.

Zählung: Eine neue Funktion erhöht die mittlere Zahl (1.5.0 zu 1.6.0). Eine Fehlerbehebung, eine kleine Anpassung oder reine Doku erhöht die letzte Zahl (1.5.0 zu 1.5.1). Jeder gemergte Pull Request bekommt eine Version, ausser dem Projektgerüst (PR 1: CLAUDE.md und STATUS.md). Die Version steht in `bandampel.ino` (`FW_VERSION`) und wird auf der Seite «Über» und im Web-Menü angezeigt. Alle Versionen entstanden am 18.11.2026 und sind nachträglich in der Reihenfolge der Merges nummeriert. Tag und Release `v1.x.y` liegen auf dem jeweiligen Merge-Commit. Marcel Sauder legt sie mit `tools/releases_anlegen.sh` an (Claude darf in der Sitzung keine Releases anlegen).

| Version | Gemergt | PR | Inhalt |
|---|---|---|---|
| 1.18.9 | 2026-11-18 | dieser PR | STATUS: Akku und Strom (Vorschlag), Bilder, Port |
| 1.18.8 | 2026-11-18 | [#38](https://github.com/MaShaoDe/Bandampel/pull/38) | Bilder der 8 Display-Seiten und 7 Web-Reiter im Handbuch (docs/bilder/) |
| 1.18.7 | 2026-11-18 | [#37](https://github.com/MaShaoDe/Bandampel/pull/37) | Nachbauanleitung (docs/Nachbauen.md), Release v1.18.6 im Skript |
| 1.18.6 | 2026-11-18 | [#36](https://github.com/MaShaoDe/Bandampel/pull/36) | Release-Skript bis v1.18.5, STATUS: Firmware eingefroren |
| 1.18.5 | 2026-11-18 | [#35](https://github.com/MaShaoDe/Bandampel/pull/35) | Seite Info zeigt die Firmware-Version, 7 Zeilen etwas kleiner |
| 1.18.4 | 2026-11-18 | [#34](https://github.com/MaShaoDe/Bandampel/pull/34) | STATUS: Name «Bandampel Firmware» |
| 1.18.3 | 2026-11-18 | [#33](https://github.com/MaShaoDe/Bandampel/pull/33) | STATUS aufgeräumt, Release-Skript bis v1.18.2 |
| 1.18.2 | 2026-11-18 | [#32](https://github.com/MaShaoDe/Bandampel/pull/32) | Seitenname in der Kopfzeile auch bei Baender und Details (mehr Platz) |
| 1.18.1 | 2026-11-18 | [#31](https://github.com/MaShaoDe/Bandampel/pull/31) | Web-Menü: Rückfrage vor «Speichern und neu starten» |
| 1.18.0 | 2026-11-18 | [#30](https://github.com/MaShaoDe/Bandampel/pull/30) | Seitenname in der Kopfzeile, klarere Namen der Seiten im Web-Menü |
| 1.17.3 | 2026-11-18 | [#29](https://github.com/MaShaoDe/Bandampel/pull/29) | Changelog, Versionsnummer im Programm, Regel zur Versionszählung |
| 1.17.2 | 2026-11-18 18:55 | [#28](https://github.com/MaShaoDe/Bandampel/pull/28) | Benutzerhandbuch |
| 1.17.1 | 2026-11-18 18:51 | [#27](https://github.com/MaShaoDe/Bandampel/pull/27) | STATUS: BOOT-Taste getestet |
| 1.17.0 | 2026-11-18 18:45 | [#22](https://github.com/MaShaoDe/Bandampel/pull/22) | BOOT-Taste 10 Sekunden: Werkseinstellung |
| 1.16.0 | 2026-11-18 18:43 | [#25](https://github.com/MaShaoDe/Bandampel/pull/25) | Seite Über mit Urheber, Lizenzen und QR-Code |
| 1.15.2 | 2026-11-18 18:39 | [#26](https://github.com/MaShaoDe/Bandampel/pull/26) | README: Platinen und Bausätze |
| 1.15.1 | 2026-11-18 18:09 | [#24](https://github.com/MaShaoDe/Bandampel/pull/24) | sketch.yaml mit Flash-Aufteilung Huge APP |
| 1.15.0 | 2026-11-18 18:06 | [#23](https://github.com/MaShaoDe/Bandampel/pull/23) | Web-Menü mit Reitern, Handy-Layout, WLAN-Scan |
| 1.14.0 | 2026-11-18 17:56 | [#21](https://github.com/MaShaoDe/Bandampel/pull/21) | Seite Info, WLAN-Symbol in der Fusszeile, Datenalter |
| 1.13.1 | 2026-11-18 17:19 | [#20](https://github.com/MaShaoDe/Bandampel/pull/20) | WLAN-Pause einstellbar |
| 1.13.0 | 2026-11-18 16:10 | [#19](https://github.com/MaShaoDe/Bandampel/pull/19) | Helligkeit in Prozent, stufenlos nach Lichtsensor; Details als Farbkarten |
| 1.12.0 | 2026-11-18 16:08 | [#18](https://github.com/MaShaoDe/Bandampel/pull/18) | Werkseinstellung mit Bestätigung |
| 1.11.1 | 2026-11-18 16:03 | [#17](https://github.com/MaShaoDe/Bandampel/pull/17) | Web-Menü: neutrale Seite nach Speichern |
| 1.11.0 | 2026-11-18 15:57 | [#16](https://github.com/MaShaoDe/Bandampel/pull/16) | Seite Sonne mit Bogen, Countdown und Grayline |
| 1.10.0 | 2026-11-18 15:44 | [#15](https://github.com/MaShaoDe/Bandampel/pull/15) | Web-Menü, mDNS (bandampel-001.local), Notfall-WLAN |
| 1.9.0 | 2026-11-18 15:39 | [#14](https://github.com/MaShaoDe/Bandampel/pull/14) | Einstellungen im Flash, Gerätenummer 001 bis 999 |
| 1.8.0 | 2026-11-18 15:33 | [#13](https://github.com/MaShaoDe/Bandampel/pull/13) | Sonnenaufgang und Sonnenuntergang |
| 1.7.1 | 2026-11-18 14:41 | [#12](https://github.com/MaShaoDe/Bandampel/pull/12) | Option für Anzeigechip und Farbreihenfolge |
| 1.7.0 | 2026-11-18 14:26 | [#11](https://github.com/MaShaoDe/Bandampel/pull/11) | Live-Seite (WSPR, inkl. 160 m), Seiten und Betriebsarten schaltbar |
| 1.6.0 | 2026-11-18 14:14 | [#10](https://github.com/MaShaoDe/Bandampel/pull/10) | SSB oben, einstellbar |
| 1.5.0 | 2026-11-18 14:07 | [#9](https://github.com/MaShaoDe/Bandampel/pull/9) | Seitenwechsel: Auto und Halten |
| 1.4.0 | 2026-11-18 13:51 | [#8](https://github.com/MaShaoDe/Bandampel/pull/8) | Seite Betriebsart (SSB, CW) |
| 1.3.0 | 2026-11-18 13:46 | [#7](https://github.com/MaShaoDe/Bandampel/pull/7) | Zürcher Zeit mit UTC daneben |
| 1.2.0 | 2026-11-18 13:44 | [#5](https://github.com/MaShaoDe/Bandampel/pull/5) | Optik: filigrane Karten, 9 Bänder inkl. 160 m, Rufzeichen |
| 1.1.0 | 2026-11-18 13:43 | [#6](https://github.com/MaShaoDe/Bandampel/pull/6) | Nachtmodus per Lichtsensor |
| 1.0.2 | 2026-11-18 13:37 | [#4](https://github.com/MaShaoDe/Bandampel/pull/4) | WLAN-Diagnose und längeres Zeitlimit |
| 1.0.1 | 2026-11-18 13:09 | [#3](https://github.com/MaShaoDe/Bandampel/pull/3) | Fix: Tasterpin umbenannt (Konflikt mit dem ESP32-Kern) |
| 1.0.0 | 2026-11-18 13:00 | [#2](https://github.com/MaShaoDe/Bandampel/pull/2) | Firmware V1: WLAN, HamQSL-Daten, Übersicht, Bänder, Details |
