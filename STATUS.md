# Status Bandampel

Höchstens 30 Zeilen. Immer überschreiben, nicht anhängen. Erledigtes streichen.

## Stand
- Bandampel Firmware 1.18.4 läuft auf dem CYD (ESP32-2432S028, ILI9341, Port `/dev/cu.wchusbserial110`, arduino-cli, FQBN `esp32:esp32:esp32:PartitionScheme=huge_app`, steht in `sketch.yaml`). Stand 18.11.2026.
- Läuft (am Gerät bestätigt): 8 Seiten mit Namen in der Kopfzeile (Ampel, Sonne, Baender, Modus, Live, Details, Info, Lizenz), alle im Web-Menü abwählbar, gleiche Reihenfolge wie am Gerät. Auto-Wechsel, Halten per langem Druck, WLAN-Symbol, Datenalter.
- Läuft: Web-Menü mit 7 Reitern (bandampel-001.local), Rückfrage vor «Speichern und neu starten», Einstellungen im Flash (Vorrang vor secrets.h), Notfall-WLAN Bandampel-Setup nach 60 s ohne Netz, WLAN-Scan, Helligkeit in Prozent (stufenlos nach Lichtsensor).
- Farbtest abgeschlossen: ILI9341 mit Standardwerten bleibt.
- BOOT-Taste 10 s: Countdown und Abbruch getestet, voller Reset noch nicht ausprobiert. Ungetestet: Bestätigung der Werkseinstellung im Menü, Pause zwischen WLAN-Versuchen.
- Einstufungen (Live-Schwellen, Details-Karten, CW und SSB, Grayline) sind Faustregeln (Vorschlag), keine Prognose.
- Beobachtung (offen): Das Menü wird nur zwischen Datenabrufen bedient (bis 20 s Wartezeit). Nach «Speichern» etwa 30 s warten. Umbau erst auf Wunsch.

## Als Nächstes
- Phase 1 (Firmware auf dem CYD) gilt als fertig, Einfrieren nach dem Test der Kopfzeile (Vorschlag).
- Neuer Chat: Spannung, Strom, Akku und Stabilisierung. Akku vorhanden: XMY 103665, 3,7 V, 3000 mAh, 11,1 Wh, mit Schutzschaltung. Danach 3D-gedrucktes Gehäuse. Weiter mit eigenem Board «Bandampel Neo» (offen, Repo-Form offen).
- Live-Schwellen kalibrieren: Fotos der Live-Seite morgens, abends, nachts (offen, abwarten).

## Entscheide (Marcel Sauder)
- 18.11.2026: Repo heisst Bandampel. Doku Deutsch zuerst, kurze englische Zusammenfassung. Firmware GPL-3.0-only.
- 18.11.2026: Nur privat, zum Materialpreis, Bastlerprojekt (siehe README). Keine Personendaten im Repository.

- 18.11.2026: Die Software heisst «Bandampel Firmware» mit Versionsnummer (zum Beispiel Bandampel Firmware 1.18). Hardware bekommt eigene Namen (zum Beispiel Bandampel Neo).

## Stolpersteine
- `arduino-cli upload` schreibt bei einem Fehler im Kompilieren die alte Version. Immer mit `&&` verbinden.
- Claude kann hier nicht kompilieren oder testen, nur Marcel am Gerät.
- Marcel führt `sh tools/releases_anlegen.sh` aus (Releases v1.0.0 bis v1.18.2, offen). Handbuch: docs/Benutzerhandbuch.md, Fotos fehlen noch (docs/bilder/).
