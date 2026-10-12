param(
    [string]$Port = ""
)

$ErrorActionPreference = "Stop"
$FQBN = "esp32:esp32:esp32"

if ([string]::IsNullOrWhiteSpace($Port)) {
    $Port = & "$PSScriptRoot\Find-ESP32Port.ps1"
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($Port)) {
        throw "COM-Port konnte nicht ermittelt werden."
    }
    $Port = $Port.Trim()
}

Write-Host ""
Write-Host "Kompiliere Projekt ..." -ForegroundColor Cyan
arduino-cli compile --fqbn $FQBN "$PSScriptRoot"
if ($LASTEXITCODE -ne 0) {
    throw "Kompilierung fehlgeschlagen."
}

Write-Host ""
Write-Host "Flashe ESP32 an $Port ..." -ForegroundColor Cyan
arduino-cli upload -p $Port --fqbn $FQBN "$PSScriptRoot"
if ($LASTEXITCODE -ne 0) {
    throw "Upload fehlgeschlagen."
}

Write-Host ""
Write-Host "Upload erfolgreich." -ForegroundColor Green
