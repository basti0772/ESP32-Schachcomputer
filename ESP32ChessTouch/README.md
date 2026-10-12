# ESP32 Chess Touch

Eigenstaendiger Schachcomputer fuer ESP32-WROOM-32 und ILI9341 mit resistivem
SPI-Touch (XPT2046-kompatibel). Touch und serielles Terminal sind gleichzeitig
verfuegbar. Die Engine entspricht der bisherigen Version, mit ca. 1,8 Sekunden
Bedenkzeit pro Zug.

## Arduino IDE

1. Den gesamten Ordner ESP32ChessTouch entpacken/kopieren.
2. ESP32ChessTouch.ino in der Arduino IDE oeffnen.
3. Boardpaket esp32 von Espressif Systems installieren (getestet: 3.3.12).
4. Bibliothek TFT_eSPI von Bodmer installieren (getestet: 2.5.43).
5. Die mitgelieferte config/User_Setup.h als User_Setup.h im TFT_eSPI-
   Bibliotheksordner verwenden. Vorhandene andere Konfiguration vorher sichern.
   Diese Bibliothekskonfiguration gilt auch fuer andere TFT_eSPI-Projekte.
   Im User_Setup_Select.h muss die Standardzeile #include <User_Setup.h>
   aktiviert sein. Eine Konfiguration nur im Sketch reicht bei TFT_eSPI nicht.
6. Board ESP32 Dev Module und den angeschlossenen USB-Port waehlen, hochladen.

Preferences und SPI sind Bestandteile des ESP32-Boardpakets.

## Verkabelung

| Display/Touch | ESP32 GPIO |
|---|---:|
| SPI MISO (auch Touch DOUT) | 19 |
| SPI MOSI (auch Touch DIN) | 23 |
| SPI SCLK (auch Touch CLK) | 18 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT RST | 4 |
| Touch CS | 32 |

Gemeinsame Masse und passende Versorgung wie beim bestehenden Aufbau.
Touch IRQ wird nicht verwendet. Rotation 3: 90 Grad nach links gegenueber
bisherigem Hochformat; Aufloesung 320 x 240.

## Erste Inbetriebnahme

Beim ersten Start vier gelbe Kreuze nacheinander genau in der Mitte antippen.
Jeden Punkt kurz halten (mindestens ca. 150 ms), dann den Finger abheben.
Der vierte Punkt prueft die Genauigkeit. Bei zu grosser Abweichung beginnt die
Kalibrierung erneut. Ein Kunststoffstift ist fuer den resistiven Touch hilfreich.
Die Kalibrierung wird im ESP32-NVS gespeichert und beim naechsten Start geladen.
Mit der Schaltflaeche Kalibrieren kann sie jederzeit wiederholt werden.

## Spielen

- Eigene Figur antippen: Auswahl und legale Zielfelder erscheinen tuerkis.
- Zielfeld antippen: Zug wird ausgefuehrt, die Engine antwortet automatisch.
- Dieselbe Figur erneut antippen: Auswahl aufheben.
- Andere eigene Figur antippen: Auswahl wechseln.
- Umwandlung: rechts Dame, Turm, Laeufer oder Springer waehlen.
- Neu Weiss / Neu Schwarz: zweimal antippen, um eine neue Partie zu starten.
  Die Bezeichnung meint die Farbe des Menschen; bei Neu Schwarz beginnt die Engine.
- Weiss steht auf dem Brett unten. Die Partie selbst wird bei Neustart nicht
  gespeichert. Nur die Touch-Kalibrierung bleibt erhalten.
- Waehrend die Engine rechnet, werden keine Touch-Zuege verarbeitet.

## Serielles Terminal

115200 Baud, 8N1, Zeilenende LF oder CRLF. Beispiele:

- new / new white / new black: neue Partie
- e2e4: Zug; e7e8q: Umwandlung (q/r/b/n)
- board / legal / help: Stellung, legale Zuege, Hilfe
- go: Engine spielt die Seite am Zug
- auto off / auto on: automatische Antworten aus/ein
- resign: Seite am Zug gibt auf
- calibrate: Touch-Kalibrierung starten
- calibrate cancel: Kalibrierung abbrechen; Terminal bleibt bedienbar
- touchinfo: Kalibrierstatus und Touch-Druck lesen
- tap X Y: Diagnose; simuliert einen Bildschirmtipp in Pixelkoordinaten
- selftest / perft 4: Zugerzeugung pruefen

Terminalbefehle funktionieren auch waehrend der Kalibrierung; die Anzeige bleibt
bis Abschluss oder Abbruch auf dem Kalibrierbildschirm. Die Kalibrierung benoetigt
keinen PC. Nach Abbruch ohne gespeicherte Werte startet ein echter Tipp sie erneut.

Unter Windows optional Play-Chess.ps1 fuer ein interaktives Terminal starten.
Find-ESP32Port.ps1 erkennt den COM-Port automatisch.

## Arduino CLI

Im Projektordner:

    arduino-cli compile --fqbn esp32:esp32:esp32 .
    powershell -ExecutionPolicy Bypass -File .\Build-And-Flash.ps1

Build-And-Flash.ps1 kompiliert und laedt auf den automatisch ermittelten Port.

Die Engine hat noch keine gemessene Elo-Zahl. Sie unterstuetzt Rochade, en passant,
alle Umwandlungen, Schachmatt und Patt. Dreifache Wiederholung und 50-Zuege-Regel
beenden die Partie automatisch ohne Remisantrag.
