# Drittanbieter

Das Projekt verwendet folgende separat erhältliche Komponenten. Ihre jeweiligen
Lizenzen und Hinweise gelten weiterhin, insbesondere für Bestandteile der
mitgelieferten Firmware-Binärdateien:

- Arduino-ESP32 / Espressif Systems, Version 3.3.12:
  https://github.com/espressif/arduino-esp32/tree/3.3.12
  (einschließlich ESP-IDF und der dort aufgeführten Drittkomponenten)
- TFT_eSPI / Bodmer, Version 2.5.43:
  https://github.com/Bodmer/TFT_eSPI/tree/v2.5.43
  Die mitgelieferte User_Setup.h basiert auf dessen Konfigurationsvorlage.
- esptool / Espressif Systems, getestet mit 5.3.1:
  https://github.com/espressif/esptool
  Wird separat installiert; kein Flasher-Programm ist in diesem Repository enthalten.
- python-chess (nur für PC-Tests): https://github.com/niklasf/python-chess
- pyserial (nur für PC-Tests): https://github.com/pyserial/pyserial
- ESP Web Tools 10.1.1 (Browser-Installer, zur Laufzeit von unpkg geladen):
  https://github.com/esphome/esp-web-tools (Apache-2.0).

Referenzstellungen für die Perft-Tests:
https://chessprogramming.org/Perft_Results

Für den eigenen Projektcode wurde noch keine separate Lizenz festgelegt.
