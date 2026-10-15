# Browser installer verification — 2026-10-15

- Full clean local build: Arduino CLI 1.5.2-rc.1, Arduino-ESP32 3.3.12,
  TFT_eSPI 2.5.43, FQBN `esp32:esp32:esp32`.
- Existing firmware sources unchanged; display configuration injected only
  for this build. Version 2.0 read from the existing startup banner.
- Build: 354244 bytes program storage, 31036 bytes static RAM.
- Merged output: 419936 bytes at offset 0, SHA256
  `5c00f84f4da5384604669aca6e89cc181cf8eee02a3b750d27b71de080694ae0`.
- Offsets read from the expanded Arduino merge recipe: bootloader `0x1000`,
  partitions `0x8000`, boot_app0 `0xe000`, application `0x10000`.
- All four embedded input images match their build SHA256 hashes. The parsed
  partition table agrees with the application offset and capacity.
- Bootloader header checked: classic ESP32, DIO, 80 MHz, 4 MB.
- Manifest version, chip family, offset, hashes and both firmware copies pass
  `tools/Test-Web-Firmware.ps1`.
- Local HTTP server served only `docs/`. Manifest-relative binary URL returned
  the exact expected size and SHA256. Installer loaded in a browser with
  version 2.0, screenshot and enabled Install button; visually inspected.
- Actual USB upload of the same build as separate images completed with
  esptool hash verification on ESP32-D0WDQ6 revision 1.0. All four serial
  perft selftests passed; after e2e4 the engine replied b8c6.
- Device currently reports no saved chess touch calibration and displays the
  calibration prompt. Complete the four-point calibration on the display.
- A complete Web Serial flash was not performed. USB verification used the
  separate build images, avoiding the merged file's NVS padding writes.
- Repository text and binary scan found no private keys, credential patterns
  or absolute user-profile paths. Existing COM-port examples and historical
  test records were retained; scripts detect ports rather than fixing one.
- GitHub Pages requires the one-time `main` / `/docs` setting described in
  README. The local test does not establish that Pages is already enabled.
