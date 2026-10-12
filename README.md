# ESP32-Schachcomputer

Eigenständiger Schachcomputer für einen klassischen **ESP32-WROOM-32** mit
**ILI9341-Display und resistivem SPI-Touch (XPT2046-kompatibel)**.
Bedienung wahlweise per Touch oder seriell mit 115200 Baud.

## Funktionen

- Schachengine mit Alpha-Beta-Suche, maximal Tiefe 4 und etwa 1,8 Sekunden pro Zug
- Legale Züge, Rochade, en passant, vier Bauernumwandlungen, Matt und Patt
- Touch: Startfigur und Zielfeld wählen; legale Ziele werden markiert
- Neue Partie als Weiß oder Schwarz, Auswahl der Umwandlungsfigur
- Vierpunkt-Touch-Kalibrierung mit Speicherung im NVS
- Querformat 320 × 240, um 90° nach links gedreht
- Serielle Befehle und Diagnosefunktionen auch während der Kalibrierung

Eine gemessene Elo-Zahl liegt noch nicht vor. Die Partie wird bei Neustart
zurückgesetzt; die Touch-Kalibrierung bleibt gespeichert. Wiederholung und
50-Züge-Regel werden automatisch als Remis gewertet.

## Schnellstart: fertige Firmware flashen

Repository über **Code → Download ZIP** herunterladen und vollständig entpacken.
Auf Windows benötigt der Flasher PowerShell 5.1 oder neuer sowie **esptool 5.x**
(getestet 5.3.1), etwa aus dem ESP32-Boardpaket der Arduino IDE.

Im Repository-Ordner:

```powershell
powershell -ExecutionPolicy Bypass -File .\ESP32ChessTouch-Flash\Flash-ESP32.ps1 -CheckOnly
powershell -ExecutionPolicy Bypass -File .\ESP32ChessTouch-Flash\Flash-ESP32.ps1
```

Der Port wird automatisch erkannt. Optional `-Port COM3` oder
`-EsptoolPath "C:\Tools\esptool.exe"` angeben. Bei Übertragungsproblemen hilft
`-Baud 115200`. Andere serielle Programme vor dem Flashen schließen.

Das mitgelieferte Anwendungs-Binary wurde am 12.10.2026 aus den unveränderten
Touch-Schachquellen neu gebaut, mit neutralisierten Compiler-Diagnosepfaden.
Bootloader und Partitionen stammen vom zuvor erfolgreich geflashten Stand.
Für das neu gebaute Binary wurde kein erneuter Hardwaretest ausgeführt. Vor dem Upload werden SHA256-Prüfsummen geprüft.
Die NVS-Kalibrierung des bisherigen Partitionslayouts wird nicht gelöscht.

[Ausführliche Flash-Anleitung](ESP32ChessTouch-Flash/README.md)

## Arduino-Projekt selbst bauen

`ESP32ChessTouch/ESP32ChessTouch.ino` öffnen. Benötigt werden:

- ESP32-Boardpaket von Espressif Systems **3.3.12**
- TFT_eSPI von Bodmer **2.5.43**
- Board: **ESP32 Dev Module**, FQBN `esp32:esp32:esp32`

Die Datei `ESP32ChessTouch/config/User_Setup.h` in der TFT_eSPI-Bibliothek als
`User_Setup.h` verwenden; eine vorhandene Konfiguration vorher sichern.
Die Einstellung gilt bibliotheksweit. Details stehen in der Projektanleitung.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 .\ESP32ChessTouch
powershell -ExecutionPolicy Bypass -File .\ESP32ChessTouch\Build-And-Flash.ps1
```

[Verkabelung, Touch-Kalibrierung und Bedienung](ESP32ChessTouch/README.md)

## Terminal

```powershell
powershell -ExecutionPolicy Bypass -File .\ESP32ChessTouch\Play-Chess.ps1
```

Beispiele: `new`, `new black`, `e2e4`, `e7e8q`, `board`, `legal`, `help`.
Im seriellen Monitor: 115200 Baud, 8N1, LF oder CRLF.

## Tests und Stand der Prüfung

Die Firmware wurde auf dem ESP32 kompiliert, geflasht und seriell geprüft.
Vier Referenz-Perft-Tests, die Touch-Bedienlogik über simulierte Koordinaten,
alle vier Umwandlungen und automatische Engine-Antworten bestanden.
Eine vollständige Partie gegen einen einfachen PC-Testgegner endete nach
29 Zügen mit einem Sieg des ESP32; das ist keine Elo-Messung.
Die gespeicherte Touch-Kalibrierung wurde nach erneutem Flashen als aktiv gemeldet.
Eine visuelle Prüfung des Displays ist im Testprotokoll nicht belegt.

- [Testbeschreibung](ESP32ChessTouch/TESTS.md)
- [Testpartie](test-results/test-game.pgn)
- [Touch-Protokoll](test-results/touch-test-results.json)

Optionale PC-Tests benötigen Python und lokal installierte Testabhängigkeiten:

```powershell
python -m pip install --target .test-deps python-chess pyserial
python .\Serial-Chess.py board selftest
python .\Test-TouchChess.py
```

Die Hardwaretests starten neue Partien und verändern den Spielzustand auf dem
angeschlossenen ESP32. Die eigentliche Firmware benötigt kein Python.

## Ordner

- `ESP32ChessTouch/`: vollständiges Arduino-Projekt und Windows-Hilfsskripte
- `ESP32ChessTouch-Flash/`: fertige Firmware, Prüfsummen und Flash-Skript
- `test-results/`: vorhandene Testergebnisse und Beispielpartie

Fremdbibliotheken werden über Arduino beziehungsweise pip installiert und sind
nicht als lokale Entwicklungsumgebung mitkopiert. Siehe [Drittanbieterhinweise](THIRD_PARTY.md).

