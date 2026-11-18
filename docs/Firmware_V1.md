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
- Farbstich (z.B. Grün als Türkis, Rot als Rosa): In `secrets.h` zuerst `#define PANEL_ST7789 1` probieren (CYD mit 2 USB-Anschlüssen hat oft einen ST7789). Sind die Farben danach negativ, `#define PANEL_INVERT 0` setzen. Sind Rot und Blau vertauscht, `#define PANEL_RGB_ORDER 1`. Neu kompilieren und hochladen.
- Nachtmodus: Empfindlichkeit über `LDR_THRESHOLD` (Schwelle) und `LDR_HYST` (Toleranz, Standard 150) in `secrets.h`.
- Mehrere WLAN-Netze (Heimnetz, Hotspot): Liste in `secrets.h`.

## Quelle

Bandprognose: HamQSL (N0NBH), Abruf alle 30 Minuten.

## Nachtmodus einstellen

1. Firmware hochladen, Monitor öffnen: Alle 5 Sekunden erscheint `LDR roh: <Wert>`.
2. Wert bei Tageslicht notieren, dann den Lichtsensor (kleiner Fotowiderstand neben dem Display) mit dem Finger abdecken und den Wert im Dunkeln notieren.
3. In `secrets.h` die Mitte als `LDR_THRESHOLD` eintragen. Wird der Wert im Dunkeln kleiner, zusätzlich `#define LDR_NIGHT_IS_HIGH false`.
4. Neu hochladen. Im Dunkeln wird die Helligkeit auf `BRIGHTNESS_NIGHT` gesenkt.

## Betriebsart (CW und SSB)

HamQSL liefert keine Prognose pro Betriebsart. Die Seite ist eine Faustregel (Vorschlag): CW und Digital (FT8) kommen mit ca. 10 bis 15 dB weniger Signal aus als SSB und werden deshalb eine Stufe besser bewertet. Bei K ab 5 (gestört) werden beide eine Stufe schlechter bewertet.

## Rufzeichen

In `secrets.h`: `#define CALLSIGN "HE9QCU"`. Es steht dann in der Kopfzeile aller Seiten. Die Datei bleibt privat.

## Seitenwechsel: Auto oder Halten

- Kurz tippen oder BOOT-Taste kurz drücken: nächste Seite.
- Lang drücken (ca. 0,8 s): zwischen **AUTO** (Seiten wechseln alle 10 Sekunden) und **HALT** (bleibt auf der Seite) umschalten. Der Zustand steht unten im Display und wird im Gerät gespeichert.
- Intervall ändern: in `secrets.h` `#define AUTO_PAGE_MS 15000` (in Millisekunden).

## Reihenfolge SSB und CW

Standard: SSB oben, CW unten. Umkehren in `secrets.h` mit `#define SSB_FIRST 0`.

## Seiten ein- und ausschalten

Alle Seiten sind einzeln schaltbar, in `secrets.h` (1 = an, 0 = aus, Standard alle an):

```cpp
#define PAGE_OVERVIEW 1   // Übersicht
#define PAGE_FORECAST 1   // Prognose Bänder (80m bis 10m)
#define PAGE_MODES 1      // Prognose Betriebsart
#define PAGE_LIVE 1       // Live-Aktivität (inkl. 160m)
#define PAGE_DETAILS 1    // Details
```

Auf der Seite Betriebsart lassen sich SSB und CW einzeln schalten: `#define SHOW_SSB 0` oder `#define SHOW_CW 0`. Ist nur eine Betriebsart an, werden die Balken grösser. Sind alle Seiten aus, bleibt die Übersicht. Ist `PAGE_LIVE` aus, wird wspr.live auch nicht abgefragt.

## Live-Aktivität (WSPR)

- 160m liefert HamQSL nicht. Die Live-Seite zeigt deshalb **gemessene Aktivität** aller 9 Bänder (160m bis 10m): Anzahl WSPR-Spots der letzten 30 Minuten, bei denen Sender oder Empfänger in der Region liegt (Standard Locator-Felder JN und JO).
- Quelle: wspr.live (Abfrage alle 15 Minuten, eine kleine Abfrage für alle Bänder). Der Dienst wird von Freiwilligen betrieben, bitte nicht öfter abfragen.
- Region und Zeitfenster: `#define LIVE_FIELDS "'JN','JO'"` und `#define LIVE_MINUTES 30`.
- Es ist Aktivität, keine Prognose, und sagt nichts über Betriebsart. Die Farbschwellen (rot, gelb, grün) pro Band sind ein Vorschlag und müssen im Betrieb eingestellt werden (`LIVE_FAIR` und `LIVE_GOOD` in `bandampel.ino`). Die gemessenen Zahlen stehen im seriellen Monitor (`wspr.live (...)`).

## Sonnenaufgang und Sonnenuntergang

Die Seite «Sonne» (nach der Übersicht) zeigt einen Sonnenbogen mit der aktuellen Position (nachts den Mond unter dem Horizont), Aufgang, Untergang, Tageslänge und einen Countdown zum nächsten Ereignis. In den 30 Minuten vor und nach Auf- und Untergang steht «GRAYLINE AKTIV». In dieser Dämmerungszone sind die tiefen Bänder (160, 80, 40 m) oft besonders gut. Das ist eine Faustregel, keine Prognose. Auf der Seite «Bänder» ist die Zeile Tag (Sonne) oder Nacht (Mond) hell, wenn sie gerade gilt. Die Zeiten werden im Gerät berechnet (Genauigkeit etwa 1 bis 2 Minuten), in Ortszeit (Zürich, mit Sommerzeit). Standort mit `SUN_LAT` und `SUN_LON` (Voreinstellung Frauenfeld), Seite ausblenden mit `#define PAGE_SUN 0` oder im Web-Menü. Vor der Zeitsynchronisation steht «Warte auf Uhrzeit».

## Einstellungen im Flash

Alle Einstellungen laufen jetzt über eine Laufzeit-Konfiguration (`settings.h`). Die Werte in `secrets.h` sind die Vorgabe. Was später im Web-Menü gespeichert wird, liegt im Flash (Bereich «cfg») und hat Vorrang. Das Web-Menü selbst folgt in einem weiteren Schritt, bis dahin ändert sich im Verhalten nichts.

Neu: Geräte-Nummer `UNIT_NUMBER` (1 bis 999, Standard 1). Der Gerätename im Netz ist `bandampel-001`, damit mehrere Geräte parallel laufen können. Ausserdem `MENU_PASSWORD` (leer = kein Passwort) und `TZ_DEFAULT`.

## Web-Menü

Das Gerät hat einen Web-Konfigurator. Er läuft im selben WLAN wie das Gerät, ohne Zusatzgeräte.

1. Im Browser `http://bandampel-001.local` öffnen (die Nummer ist `UNIT_NUMBER`). Geht das nicht, die IP-Adresse aus der Fusszeile des Geräts verwenden.
2. Einstellungen ändern und «Speichern und neu starten» drücken. Das Gerät startet danach neu.
3. «Werkseinstellung» löscht alle gespeicherten Werte. Danach gelten wieder die Vorgaben aus `secrets.h`.

Passwort: Ist ein Menü-Passwort gesetzt, fragt der Browser danach (Benutzer `admin`). Leer heisst kein Passwort. Die Übertragung ist unverschlüsselt (HTTP), das Passwort schützt also nur vor zufälligem Zugriff im Heimnetz.

Notfall-WLAN: Findet das Gerät 60 Sekunden lang kein bekanntes WLAN, öffnet es das WLAN «Bandampel-Setup» (Fusszeile: «AP Bandampel-Setup»). Dort unter `http://192.168.4.1` das Menü öffnen und die WLAN-Daten korrigieren. Ist ein Menü-Passwort mit mindestens 8 Zeichen gesetzt, gilt es auch als WLAN-Passwort, sonst ist das Notfall-WLAN offen. Findet das Gerät wieder ein bekanntes Netz, schliesst sich das Notfall-WLAN von selbst. Im Notfall-Modus sucht das Gerät nur alle 5 Minuten nach dem bekannten Netz.

Live-Schwellen: Auf der Menüseite steht neben jedem Band die aktuelle Zahl der WSPR-Spots. Damit lassen sich die Schwellen «mittel» und «gut» nach Foto oder Gefühl einstellen.

