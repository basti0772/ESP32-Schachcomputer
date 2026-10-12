param(
    [string]$Port = "",
    [int]$Baud = 115200,
    [int]$Seconds = 10
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($Port)) {
    $Port = & "$PSScriptRoot\Find-ESP32Port.ps1"
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($Port)) {
        throw "COM-Port konnte nicht ermittelt werden."
    }
    $Port = $Port.Trim()
}

Write-Host "Lese $Port mit $Baud Baud fuer $Seconds Sekunden ..."

$serial = New-Object System.IO.Ports.SerialPort $Port, $Baud, "None", 8, "One"
$serial.ReadTimeout = 200
$serial.DtrEnable = $false
$serial.RtsEnable = $false

try {
    $serial.Open()
    $end = (Get-Date).AddSeconds($Seconds)

    while ((Get-Date) -lt $end) {
        try {
            $text = $serial.ReadExisting()
            if ($text) {
                Write-Host -NoNewline $text
            }
        }
        catch {
            # Bei kurzem Read-Timeout einfach weiter.
        }
        Start-Sleep -Milliseconds 50
    }
}
finally {
    if ($serial.IsOpen) {
        $serial.Close()
    }
    $serial.Dispose()
}
