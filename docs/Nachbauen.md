<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright (c) 2026 FerienBSD aka Marcel Sauder -->
# Nachbauen: Bandampel Firmware auf dem CYD

Du brauchst kein Löten und keine Vorkenntnisse. Ein fertiges Board, ein USB-Kabel und etwa 30 Minuten reichen. Die Anleitung gilt für Windows und macOS. Es gibt 2 Wege zum Aufspielen: mit dem Programm «Arduino IDE» (mit Maus) oder mit `arduino-cli` (Terminal). Nimm einen von beiden.

## 1. Teile

- Board **ESP32-2432S028** (der Name steht hinten auf der Platine, manchmal mit «R» am Ende). Im Handel heisst es «CYD» oder «Cheap Yellow Display». Es hat ein 2,8-Zoll-Display, 240 x 320 Pixel.
- USB-Kabel mit Datenleitungen. Reine Ladekabel erkennt der Computer nicht.
- Ein WLAN mit 2,4 GHz (5 GHz kann der ESP32 nicht). Beim iPhone-Hotspot «Kompatibilität maximieren» einschalten.

Wenn der Computer das Board nicht als Anschluss (Port) zeigt, installiere den Treiber für den Chip CH340 (Windows) und probiere ein anderes Kabel.

## 2. Programm holen

Auf GitHub «Code», dann «Download ZIP» und entpacken. Oder mit `git clone https://github.com/MaShaoDe/Bandampel`. Der Ordner mit dem Programm heisst `firmware/bandampel`.

## 3. Datei secrets.h anlegen (nötig)

Ohne diese Datei lässt sich das Programm nicht kompilieren.

1. Im Ordner `firmware/bandampel` die Datei `secrets.h.example` kopieren.
2. Die Kopie `secrets.h` nennen.
3. Mit einem Texteditor öffnen und bei `WIFI_LIST` den Namen und das Passwort deines WLAN eintragen.

Alles andere in der Datei ist optional (Rufzeichen, Farben, Helligkeit, Seiten). Das WLAN lässt sich später auch im Web-Menü ändern. Werte aus dem Web-Menü haben Vorrang vor `secrets.h`. Die BOOT-Taste 10 Sekunden halten stellt wieder die Werte aus `secrets.h` her.

## 4a. Aufspielen mit der Arduino IDE (mit Maus)

1. Arduino IDE 2 von arduino.cc installieren.
2. Menü «Datei», «Einstellungen», bei «Additional boards manager URLs» eintragen: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Links «Boards-Manager», nach «esp32» suchen, «esp32 by Espressif Systems» installieren (getestet mit 3.3.12).
4. Links «Bibliotheksverwaltung», nach «LovyanGFX» suchen und installieren (getestet mit 1.2.19).
5. `firmware/bandampel/bandampel.ino` öffnen.
6. Menü «Werkzeuge» (Tools):
   - Board: **ESP32 Dev Module**
   - Partition Scheme: **Huge APP (3MB No OTA/1MB SPIFFS)** (wichtig: Mit der Standard-Aufteilung ist das Programm zu gross und das Kompilieren bricht ab)
   - Port: der Anschluss des Boards (Windows: «COM3» oder ähnlich, macOS: «/dev/cu.wchusbserial...» oder «usbserial»)
7. Pfeil «Hochladen» (rechts oben) drücken. Beim ersten Mal dauert es mehrere Minuten.

## 4b. Aufspielen mit arduino-cli (Terminal)

`arduino-cli` installieren: macOS `brew install arduino-cli`, Windows Download unter docs.arduino.cc/arduino-cli/installation. Dann im Terminal (Windows: PowerShell):

```
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install LovyanGFX
arduino-cli board list
```

`board list` zeigt den Port des Boards (macOS `/dev/cu.wchusbserial110` oder ähnlich, Windows `COM3` oder ähnlich). Dann im Repository-Ordner:

```
cd firmware/bandampel
arduino-cli compile && arduino-cli upload -p DEIN_PORT
```

Die Datei `sketch.yaml` im Ordner stellt Board und die Flash-Aufteilung «Huge APP» schon richtig ein. Gibst du `--fqbn` selbst an, hänge `:PartitionScheme=huge_app` an. Das `&&` verhindert, dass nach einem Kompilierfehler die alte Version hochgeladen wird.

## 4c. Fertige Firmware (.bin) aufspielen (ohne Quellcode, ungetestet)

Der einfachste Weg, wenn du nichts kompilieren willst: Im Bereich «Releases» auf GitHub liegt (sobald sie dort hochgeladen wurde) eine Datei `bandampel-<Version>.merged.bin`. Sie enthält Bootloader, Flash-Aufteilung und Programm in einer Datei und ist ohne WLAN-Daten und ohne Rufzeichen gebaut. Diese Anleitung ist noch **ungetestet**.

1. Werkzeug `esptool` installieren (braucht Python): `pip install esptool`
2. Den Anschluss (Port) suchen: macOS `ls /dev/cu.*`, Windows im Geräte-Manager (zum Beispiel `COM5`).
3. Aufspielen (Dateiname und Port anpassen):

```
esptool --chip esp32 --port DEIN_PORT --baud 921600 write-flash 0x0 bandampel-1.19.2.merged.bin
```

Bei älteren esptool-Versionen (4.x) heisst der Befehl `esptool.py` und `write_flash`. Gespeicherte Einstellungen gehen dabei vermutlich verloren. Danach weiter bei Kapitel 5: Rufzeichen und WLAN trägst du im Web-Menü ein. Findet das Gerät kein WLAN, öffnet es nach 60 Sekunden «Bandampel-Setup».

## 5. Erster Start

1. Nach dem Hochladen startet das Gerät von selbst. Das Display zeigt «WLAN suchen».
2. Wenn dein WLAN stimmt, zeigt die Fusszeile nach einigen Sekunden die IP-Adresse und die Seiten füllen sich mit Daten.
3. Das Web-Menü erreichst du im gleichen WLAN unter `http://bandampel-001.local`. Geht das nicht (manche Android-Geräte), nimm die IP-Adresse von der Seite «Info» auf dem Display, zum Beispiel `http://192.168.1.50`.
4. Findet das Gerät 60 Sekunden lang kein bekanntes WLAN, öffnet es ein eigenes WLAN namens **Bandampel-Setup**. Mit dem Handy verbinden, im Browser `http://192.168.4.1` öffnen, im Reiter «WLAN» Name und Passwort eintragen und «Speichern und neu starten» drücken. Dafür brauchst du `secrets.h` nicht zu ändern.

Gibt es einen Farbstich (zum Beispiel Rot und Blau vertauscht), steht die Abhilfe in `secrets.h.example` bei «Anzeige-Farben».

## 6. Weiter

Bedienung, Seiten und Einstellungen: [Benutzerhandbuch](Benutzerhandbuch.md). Technik: [Firmware_V1.md](Firmware_V1.md).
