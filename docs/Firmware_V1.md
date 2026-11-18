# Firmware V1 (CYD)

Status: ungetestet, am Gerät zu prüfen.

## Einrichten (einmalig)

```bash
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install LovyanGFX
cd firmware/bandampel
cp secrets.h.example secrets.h   # WLAN-Daten eintragen
```

## Bauen und hochladen

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/bandampel
arduino-cli upload -p /dev/cu.wchusbserial110 --fqbn esp32:esp32:esp32 firmware/bandampel
arduino-cli monitor -p /dev/cu.wchusbserial110 -c baudrate=115200
```

## Bedienung

Seite wechseln: Display antippen oder BOOT-Taste. Seiten: Übersicht (SFI, K, A, Sunspots), Bänder (Tag und Nacht), Betriebsart (CW und SSB), Details.

## Anpassen

- Zeit: Hauptuhr in Zürcher Zeit (MEZ/MESZ, automatische Sommerzeit), daneben klein die UTC-Zeit mit Z. Andere Zeitzone: `TZ_ZURICH` in `bandampel.ino` ändern.
- Bild gedreht: `ROTATION` in `bandampel.ino` von 1 auf 3.
- Farben negativ oder Rot und Blau vertauscht: `invert` oder `rgb_order` in `lgfx_cyd.h`.
- Mehrere WLAN-Netze (Heimnetz, Hotspot): Liste in `secrets.h`.

## Quelle

Bandprognose: HamQSL (N0NBH), Abruf alle 30 Minuten.

## Nachtmodus einstellen

1. Firmware hochladen, Monitor öffnen: Alle 5 Sekunden erscheint `LDR roh: <Wert>`.
2. Wert bei Tageslicht notieren, dann den Lichtsensor (kleiner Fotowiderstand neben dem Display) mit dem Finger abdecken und den Wert im Dunkeln notieren.
3. In `secrets.h` die Mitte als `LDR_THRESHOLD` eintragen. Wird der Wert im Dunkeln kleiner, zusätzlich `#define LDR_NIGHT_IS_HIGH false`.
4. Neu hochladen. Im Dunkeln wird die Helligkeit auf `BRIGHTNESS_NIGHT` gesenkt.

## Betriebsart (CW und SSB)

HamQSL liefert keine Prognose pro Betriebsart. Die Seite ist eine Faustregel (Vorschlag): CW und Digital (FT8) kommen mit ca. 10 bis 15 dB weniger Signal aus als SSB und werden deshalb eine Stufe besser bewertet. Bei K ab 5 (gestört) werden beide eine Stufe schlechter bewertet. 160m ist wie auf der Bänderseite eine Schätzung (hohl).

## Rufzeichen

In `secrets.h`: `#define CALLSIGN "HE9QCU"`. Es steht dann in der Kopfzeile aller Seiten. Die Datei bleibt privat.

## Seitenwechsel: Auto oder Halten

- Kurz tippen oder BOOT-Taste kurz drücken: nächste Seite.
- Lang drücken (ca. 0,8 s): zwischen **AUTO** (Seiten wechseln alle 10 Sekunden) und **HALT** (bleibt auf der Seite) umschalten. Der Zustand steht unten im Display und wird im Gerät gespeichert.
- Intervall ändern: in `secrets.h` `#define AUTO_PAGE_MS 15000` (in Millisekunden).

## Reihenfolge SSB und CW

Standard: SSB oben, CW unten. Umkehren in `secrets.h` mit `#define SSB_FIRST 0`.
