# Status Bandampel

Höchstens 30 Zeilen. Immer überschreiben, nicht anhängen. Erledigtes streichen.

## Stand
- Läuft auf dem CYD (ESP32-2432S028, ILI9341, Port `/dev/cu.wchusbserial110`, arduino-cli, FQBN `esp32:esp32:esp32:PartitionScheme=huge_app`, steht in `sketch.yaml`). Stand 18.11.2026.
- Läuft: Übersicht, Prognose Bänder, Prognose Betriebsart (SSB oben, CW unten), Details. Auto-Wechsel alle 10 s, langer Druck schaltet auf Halten. Kopfzeile: Rufzeichen, Zürcher Zeit und UTC. WLAN-Liste und Rufzeichen in secrets.h.
- Läuft und getestet: Live-Seite mit gemessener WSPR-Aktivität aller 9 Bänder inkl. 160m (wspr.live, Abruf alle 15 min, Region JN und JO). Prognose-Seiten ohne 160m. Alle Seiten und SSB/CW einzeln in secrets.h schaltbar.
- Abfrage geprüft: `db1.wspr.live/?query=` liefert Bandcodes (1=160m, 3=80m, 7=40m, 10=30m, 14=20m, 18=17m, 21=15m, 24=12m, 28=10m). Der Regionsfilter (JN, JO) funktioniert (Zahlen 30 bis 58 Prozent der Weltzahlen).
- Farbschwellen der Live-Seite sind geschätzt (Vorschlag), nach Beobachtung einstellen. CW und SSB sind eine Faustregel (Vorschlag, offen).
- Nachtmodus: Sensor GPIO 34, dunkel = grosser Wert, Schwelle 400 in secrets.h. Getestet, funktioniert. Empfindlichkeit über LDR_THRESHOLD und LDR_HYST in secrets.h.

## Als Nächstes
- Phase 2: Drehgeber (GPIO 22, 27, 35) und Trägerplatine (KiCad), Phase 3: Gehäuse (OpenSCAD).

## Entscheide (Marcel Sauder)
- 18.11.2026: Repo heisst Bandampel. Doku Deutsch zuerst, kurze englische Zusammenfassung. Firmware GPL-3.0-only.
- 18.11.2026: SSB hat Priorität (oben), konfigurierbar. Prognose und gemessene Daten auf getrennten Seiten, 160m nur live. Alle Seiten in secrets.h schaltbar.

## Stolpersteine
- `arduino-cli upload` schreibt bei einem Fehler im Kompilieren die alte Version. Immer mit `&&` verbinden.
- Das Hochladen überschreibt das Werksprogramm des Boards.
- Claude kann hier nicht kompilieren oder testen, nur Marcel am Gerät.
- Neu (PR offen, ungetestet): Seite Sonne nach der Übersicht (Bogen, Zeiten, Countdown, Grayline), im Menü schaltbar. Web-Menü läuft (getestet).
- Gemergt, ungetestet: Web-Menü nach Speichern auf neutraler Seite (kein doppeltes Senden bei Reload).
- Gemergt, ungetestet: Werkseinstellung verlangt Bestätigung (Menü-Passwort, sonst 000000).
- Neu (PR offen, ungetestet): Helligkeit in Prozent, 3 Modi inkl. stufenlos nach Lichtsensor, Kalibrierknöpfe im Menü (Vorgabe Tag 60, Nacht 10). Details-Seite als 4 Karten mit Farbrahmen (Einstufung ist Vorschlag).
- Neu (PR offen, ungetestet): Seite Info (WLAN, IP, Signal, Name, Datenalter, Laufzeit), WLAN-Symbol in der Fusszeile, Datenalter statt Zeitstempel.
- Neu (PR offen, ungetestet): Web-Menü mit 6 Reitern, Handy-Layout, WLAN-Scan. Offen: Live-Schwellen kalibrieren, Phase 2 (Platine).
