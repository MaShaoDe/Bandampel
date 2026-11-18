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

Seite wechseln: Display antippen oder BOOT-Taste. Seiten: Übersicht (SFI, K, A, Sunspots), Bänder (Tag und Nacht), Details.

## Anpassen

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
