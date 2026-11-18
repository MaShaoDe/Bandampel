# Status Bandampel

Höchstens 30 Zeilen. Immer überschreiben, nicht anhängen. Erledigtes streichen.

## Stand
- Läuft auf dem CYD (ESP32-2432S028, ILI9341, Port `/dev/cu.wchusbserial110`, arduino-cli, FQBN `esp32:esp32:esp32:PartitionScheme=huge_app`, steht in `sketch.yaml`). Stand 18.11.2026.
- Läuft (am Gerät bestätigt): Seiten Übersicht, Sonne (Bogen, Zeiten, Grayline), Bänder, Betriebsart, Live (WSPR, inkl. 160m), Details (4 Farbkarten), Info. Auto-Wechsel, Halten per langem Druck, WLAN-Symbol und Datenalter in der Fusszeile.
- Läuft: Web-Menü mit 6 Reitern (bandampel-001.local), Einstellungen im Flash (Vorrang vor secrets.h), Notfall-WLAN Bandampel-Setup nach 60 s ohne Netz, WLAN-Scan, Helligkeit in Prozent (stufenlos nach Lichtsensor).
- Farbtest abgeschlossen: ILI9341 mit Standardwerten bleibt, ST7789 weiss, INVERT 1 negativ, RGB_ORDER 1 vertauscht Rot und Blau.
- Ungetestet: Bestätigung der Werkseinstellung im Menü, Pause zwischen WLAN-Versuchen.
- Getestet: BOOT-Taste 10 s, Countdown ab 5 s und Abbruch durch Loslassen funktionieren. Der volle Reset bei 10 s ist noch nicht ausprobiert. Die andere Taste (RESET) startet das Gerät nur neu.
- Neu (ungetestet): Seitenname in der Kopfzeile (Ampel, Baender, Modus, Live, Details, Sonne, Info, Lizenz), gleiche Namen im Web-Menü. Rückfrage vor dem Speichern im Web-Menü. Version 1.18.2.
- Läuft: Seite Über (Urheber, Lizenzen, Datenquellen, QR-Code zu GitHub) auf dem Display und als Reiter im Web-Menü, Version 1.0.0 fest im Code.
- Einstufungen (Live-Schwellen, Details-Karten, CW und SSB, Grayline) sind Faustregeln (Vorschlag), keine Prognose.

## Als Nächstes
- Live-Schwellen kalibrieren: Fotos der Live-Seite morgens, abends, nachts (offen, abwarten).
- Phase 2: Drehgeber (GPIO 22, 27, 35) und Trägerplatine (KiCad), Phase 3: Gehäuse (OpenSCAD).

## Entscheide (Marcel Sauder)
- 18.11.2026: Repo heisst Bandampel. Doku Deutsch zuerst, kurze englische Zusammenfassung. Firmware GPL-3.0-only.
- 18.11.2026: SSB oben, konfigurierbar. Prognose und gemessene Daten getrennt, 160m nur live. Alle Seiten schaltbar.
- 18.11.2026: Nur privat, zum Materialpreis, Bastlerprojekt (siehe README). Kontakt über GitHub und Rufzeichen, keine Personendaten im Repository.

## Stolpersteine
- `arduino-cli upload` schreibt bei einem Fehler im Kompilieren die alte Version. Immer mit `&&` verbinden.
- Claude kann hier nicht kompilieren oder testen, nur Marcel am Gerät.
- Neu (PR offen): Versionsbaum 1.0.0 bis 1.18.2 in CHANGELOG.md, Tags und Releases v1.0.0 bis v1.17.2: Marcel führt `tools/releases_anlegen.sh` aus (offen). Handbuch liegt in docs/Benutzerhandbuch.md, Fotos fehlen noch (docs/bilder/).
