# Status Bandampel

Höchstens 30 Zeilen. Immer überschreiben, nicht anhängen. Erledigtes streichen.

## Stand
- Bandampel Firmware 1.18.20 im Repo. Auf dem CYD (ESP32-2432S028, ILI9341, arduino-cli, FQBN in `sketch.yaml`) zuletzt bestätigt: 1.18.5. Der Port wechselt beim Umstecken (zuletzt `/dev/cu.wchusbserial210`). Stand 19.11.2026.
- Phase 1 (Firmware auf dem CYD) ist getestet und eingefroren: nur noch Fehlerbehebungen (1.18.x), keine neuen Funktionen (Vorschlag, Marcel Sauder einverstanden). Farbtest abgeschlossen, ILI9341 bleibt.
- Ungetestet: voller Reset mit BOOT-Taste 10 s, Bestätigung der Werkseinstellung im Menü, Pause zwischen WLAN-Versuchen. Offen: Das Menü wird nur zwischen Datenabrufen bedient (bis 20 s). Einstufungen sind Faustregeln (Vorschlag), keine Prognose.
- Bilder: 8 Display-Seiten und 7 Web-Reiter (8 Bilder, Funk hat 2) liegen in docs/bilder/ (Display noch 1.18.5, Web 1.18.2, nach dem Flashen neu fotografieren). Es fehlen 00_gesamt, 01_kopf_fuss, 03_sonne_tag, 17_boot_countdown.

## Akku und Strom (alles Vorschlag, noch nichts gebaut)
- Kette: Netzteil 5 V 1 A (6,3 mm Buchse, Polarität messen, Schutzdiode) zu TP4056 mit Schutz (USB-C Board) zu Akku XMY 103665 (3,7 V, 3000 mAh, 11,1 Wh) zu Aufwärtswandler MT3608 auf 5,0 V zu CYD, Anschluss P1 VIN und GND.
- P1 VIN misst bei USB Betrieb 4,8 V (hinter der Diode D1). USB und Akku nicht gleichzeitig anschliessen, solange das nicht geklärt ist.
- Ladestrom des TP4056 auf etwa 0,5 A senken (Rprog etwa 2,4 kOhm), damit das Netzteil Reserve hat. Der TP4056 hat keinen echten Power Path: Läuft der CYD beim Laden, bleibt der Akku auf 4,2 V (kein sauberes Ladeende). Alternative: IP5306 Modul (offen).
- Nicht geeignet: XL6009 und XL6019 (Eingang unter 5 V unsicher), HW-070 (PWM Regler, hebt nicht an), PAM8403 (Audio), CN3791 (Solar, Brücke auf 12 V), LX-LJJC (Mehrzellen Lader).
- INA219 (I2C 0x40) vorhanden: nur als Messhilfe. Fest eingebaut erst bei Bandampel Neo (offen). Ladestand über IO35 und Spannungsteiler ist eine neue Funktion (offen).
- Als Nächstes: MT3608 wird bestellt (Marcel, 19.11.2026), CYD Strom am Labornetzteil messen (5,0 V an P1, Helligkeit 100 und 10 Prozent), Laufzeit rechnen, dann 3D Gehäuse mit 6,3 mm Buchse.

## Entscheide (Marcel Sauder)
- 18.11.2026: Repo heisst Bandampel. Doku Deutsch zuerst, kurze englische Zusammenfassung. Firmware GPL-3.0-only.
- 18.11.2026: Nur privat, zum Materialpreis, Bastlerprojekt (siehe README). Keine Personendaten im Repository.
- 18.11.2026: Die Software heisst «Bandampel Firmware» mit Versionsnummer. Hardware bekommt eigene Namen (zum Beispiel Bandampel Neo).

## Stolpersteine
- `arduino-cli upload` schreibt bei einem Fehler im Kompilieren die alte Version. Immer mit `&&` verbinden.
- Claude kann hier nicht kompilieren oder testen, nur Marcel am Gerät.
- Regeln für Claude (Git, Autor, Antwortstil) stehen im Projekt, nicht im Repo (öffentlich). Es gibt keine CLAUDE.md. Zu Beginn eines Chats im Projekt lesen.
- Marcel legt die Releases an (Skript bis v1.18.6, ab v1.18.7 von Hand mit `gh release create`, offen).
