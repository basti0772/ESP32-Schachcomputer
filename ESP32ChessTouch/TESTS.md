Build- und Hardware-Pruefung (27.09.2026)

Die Projektkopie enthaelt dieselben Quellen wie der Hauptsketch.
Der bisherige Sketch wurde als ESP32Codex.before-touch.txt gesichert.
Die Tests verwenden den seriellen Diagnosebefehl tap X Y und pruefen die
resultierenden Zuege/Stellungen unabhaengig mit python-chess. Das prueft die
Touch-Bedienlogik, aber ersetzt keinen realen Finger-/Stifttest am Display.

Ergebnis:
- Hauptsketch kompiliert: 354260 Bytes Flash (27 %), 31036 Bytes globale Daten.
- Upload auf automatisch erkanntem COM3 erfolgreich; Flash-Hash verifiziert.
- Alle vier eingebauten Perft-Regeltests bestanden.
- Simulierte Touch-Auswahl e2 und Zug e2e4 bestanden; e2e5 abgelehnt.
- Alle vier Umwandlungsoptionen per Touch-Menue erfolgreich geprueft.
- Touch-Zug mit automatischer Engine-Antwort erfolgreich geprueft.
- Neue Partie mit Schwarz und zweiter Bestaetigung erfolgreich geprueft.
- Serielle Befehle bleiben waehrend der Kalibrierung verfuegbar.
- Reale Touch-Kalibrierung und Sichtpruefung stehen ohne Benutzerrueckmeldung aus.

Das detaillierte Protokoll liegt in touch-test-results.json.

Auch die separate Arduino-Projektkopie wurde erfolgreich kompiliert (identische Speicherbelegung).
