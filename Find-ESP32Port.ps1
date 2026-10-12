$ErrorActionPreference = "Stop"

$ports = @(Get-CimInstance Win32_SerialPort |
    Select-Object DeviceID, Name, Description)

if ($ports.Count -eq 0) {
    Write-Error "Kein serieller COM-Port gefunden. Ist der ESP32 per Daten-USB-Kabel angeschlossen?"
    exit 1
}

# Bevorzugt typische USB-UART-Wandler und ESP32-Bezeichnungen.
$preferred = @($ports | Where-Object {
    $_.Name -match "CP210|CH340|CH341|Silicon Labs|USB.*Serial|UART|ESP32|Espressif"
})

if ($preferred.Count -eq 1) {
    Write-Output $preferred[0].DeviceID
    exit 0
}

if ($preferred.Count -gt 1) {
    Write-Host "Mehrere moegliche ESP32-Ports gefunden:" -ForegroundColor Yellow
    $preferred | Format-Table -AutoSize
    Write-Error "Der ESP32-Port ist nicht eindeutig."
    exit 2
}

# Falls nur genau ein COM-Port existiert, verwenden wir diesen.
if ($ports.Count -eq 1) {
    Write-Output $ports[0].DeviceID
    exit 0
}

Write-Host "Vorhandene COM-Ports:" -ForegroundColor Yellow
$ports | Format-Table -AutoSize
Write-Error "ESP32-Port konnte nicht automatisch eindeutig bestimmt werden."
exit 3
