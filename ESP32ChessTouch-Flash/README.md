# Letzten ESP32-Schachstand ohne Kompilieren flashen

Dieses Paket enthaelt ESP32 Chess Touch 2.0 mit Touch-Steuerung und 90 Grad
nach links gedrehter Anzeige. Die Anwendung wurde am 12.10.2026 aus den
unveraenderten Schachquellen mit neutralisierten Build-Pfaden neu kompiliert.
Bootloader und Partitionen stammen vom zuvor erfolgreich geflashten Stand.
Das neue Anwendungs-Binary wurde nicht erneut auf Hardware getestet.
Es wird NICHT neu kompiliert. Spaetere Quellcodeaenderungen sind nicht automatisch
in diesen Binaerdateien enthalten.

## Starten

Das gesamte ZIP entpacken. Im entpackten Ordner PowerShell oeffnen und ausfuehren:

    powershell -ExecutionPolicy Bypass -File .\Flash-ESP32.ps1

Der COM-Port wird automatisch erkannt. Optional explizit:

    powershell -ExecutionPolicy Bypass -File .\Flash-ESP32.ps1 -Port COM3

Nur Dateien, Flasher und Porterkennung pruefen (ohne Flashen):

    powershell -ExecutionPolicy Bypass -File .\Flash-ESP32.ps1 -CheckOnly

Bei Uebertragungsproblemen langsamer flashen:

    powershell -ExecutionPolicy Bypass -File .\Flash-ESP32.ps1 -Baud 115200

Bei 'Failed to connect' BOOT am ESP32 waehrend des Verbindungsaufbaus gedrueckt
halten und nach Beginn des Schreibens loslassen. Vorher serielle Monitore und
andere Programme schliessen, die auf den COM-Port zugreifen.

## Voraussetzungen

- Windows PowerShell 5.1 oder neuer
- Klassischer ESP32-WROOM-32 (kein ESP32-S3/C3), 4 MB Flash, bisherige Verkabelung
- USB-Datenkabel und USB-Seriell-Treiber
- esptool 5.x (getestet mit 5.3.1)

Ein mit dem Arduino-ESP32-Boardpaket installierter Flasher wird automatisch gesucht. Die Arduino IDE und Arduino CLI muessen zum
Flashen nicht gestartet werden; TFT_eSPI oder Sketch-Quellen werden nicht benoetigt.
Auf einem anderen PC das ESP32-Boardpaket 3.3.12 in der Arduino IDE installieren
oder einen vorhandenen esptool-5.x-Flasher direkt angeben:

    powershell -ExecutionPolicy Bypass -File .\Flash-ESP32.ps1 -EsptoolPath "C:\Tools\esptool.exe"

Das Paket enthaelt keinen Flasher und laedt nichts aus dem Internet herunter.

## Inhalt und Flash-Adressen

- firmware/bootloader.bin: 0x1000
- firmware/partitions.bin: 0x8000
- firmware/boot_app0.bin: 0xe000
- firmware/firmware.bin: 0x10000
- firmware/manifest.json: SHA256-Pruefsummen und Herkunft

Es werden nur diese Bereiche geschrieben; kein kompletter Chip-Erase.
Die Touch-Kalibrierung im NVS-Bereich des bisherigen Partitionslayouts bleibt
somit erhalten. Ein zusammengefuehrtes 4-MB-Abbild wird absichtlich nicht benutzt,
da dessen leere Bereiche auch gespeicherte Einstellungen ueberschreiben koennten.

Pruefsummen werden vor dem Schreiben kontrolliert. esptool prueft die geschriebenen
Daten und startet den ESP32 anschliessend neu. Der erste Start auf einem bislang
unkalibrierten Geraet zeigt die vier Touch-Kalibrierpunkte.

## Falls die automatische Flasher-Suche scheitert

Optional eine Datei esptool-path.txt mit dem vollstaendigen lokalen esptool.exe-Pfad anlegen. Alternativ esptool.exe neben Flash-ESP32.ps1 bzw. unter tools ablegen
oder -EsptoolPath verwenden. Nicht vorhandene Pfade werden bei der Suche uebersprungen.
Eine gefundene Datei muss weiterhin ein funktionsfaehiger esptool-5.x-Flasher sein.
Bei erfolgloser Suche gibt das Skript die geprueften Verzeichnisse aus.



