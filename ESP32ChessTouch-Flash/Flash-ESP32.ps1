#requires -Version 5.1
<#
.SYNOPSIS
Flasht den mitgelieferten ESP32-Schach-Binarstand ohne Kompilieren.
.EXAMPLE
powershell -ExecutionPolicy Bypass -File .\Flash-ESP32.ps1
.EXAMPLE
powershell -ExecutionPolicy Bypass -File .\Flash-ESP32.ps1 -Port COM3 -Baud 115200
.EXAMPLE
powershell -ExecutionPolicy Bypass -File .\Flash-ESP32.ps1 -CheckOnly
#>
[CmdletBinding()]
param(
    [ValidatePattern('^COM[0-9]+$')][string]$Port,
    [ValidateSet(115200,230400,460800,921600)][int]$Baud = 460800,
    [string]$EsptoolPath,
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Find-Flasher {
    if ($EsptoolPath) {
        if (-not (Test-Path -LiteralPath $EsptoolPath -PathType Leaf)) { throw "Flasher nicht gefunden: $EsptoolPath" }
        return (Resolve-Path -LiteralPath $EsptoolPath).Path
    }
    $checked = New-Object 'System.Collections.Generic.List[string]'
    $localCandidates = @((Join-Path $PSScriptRoot 'esptool.exe'), (Join-Path $PSScriptRoot 'tools\esptool.exe'))
    $pathFile = Join-Path $PSScriptRoot 'esptool-path.txt'
    if (Test-Path -LiteralPath $pathFile -PathType Leaf) {
        $savedPath = (Get-Content -LiteralPath $pathFile -Raw).Trim().Trim('"')
        if ($savedPath) { $localCandidates += [Environment]::ExpandEnvironmentVariables($savedPath) }
    }
    foreach ($candidate in $localCandidates) {
        $checked.Add($candidate)
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { return (Resolve-Path -LiteralPath $candidate).Path }
    }
    $dataRoots = @()
    if ($env:LOCALAPPDATA) { $dataRoots += Join-Path $env:LOCALAPPDATA 'Arduino15' }
    if ($env:APPDATA) { $dataRoots += Join-Path $env:APPDATA 'Arduino15' }
    if ($env:USERPROFILE) { $dataRoots += Join-Path $env:USERPROFILE 'Documents\ArduinoData' }
    $dataRoots += Join-Path $PSScriptRoot 'portable'
    foreach ($dataRoot in ($dataRoots | Select-Object -Unique)) {
        $toolRoot = Join-Path $dataRoot 'packages\esp32\tools\esptool_py'
        $checked.Add($toolRoot)
        if (-not (Test-Path -LiteralPath $toolRoot -PathType Container)) { continue }
        $versions = @(Get-ChildItem -LiteralPath $toolRoot -Directory | Where-Object { $_.Name -match '^5\.' } | Sort-Object LastWriteTime -Descending)
        foreach ($version in $versions) {
            $candidate = Join-Path $version.FullName 'esptool.exe'
            if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
            $nested = Get-ChildItem -LiteralPath $version.FullName -Recurse -Filter 'esptool.exe' -File -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($null -ne $nested) { return $nested.FullName }
        }
    }
    foreach ($name in @('esptool.exe','esptool')) {
        $tool = Get-Command $name -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($null -ne $tool) { return $tool.Source }
    }
    Write-Host ('Gepruefte Pfade (Benutzer ' + [Environment]::UserName + '):')
    foreach ($path in $checked) { Write-Host ('  ' + $path) }
    throw 'esptool 5.x nicht gefunden. ESP32-Boardpaket 3.3.12 in Arduino IDE installieren oder -EsptoolPath C:\Pfad\esptool.exe angeben. Siehe README.md.'
}

function Find-BoardPort {
    $ports = @(Get-CimInstance Win32_SerialPort | Select-Object DeviceID,Name)
    if ($ports.Count -eq 0) { throw 'Kein serieller Port gefunden. USB-Datenkabel und Treiber pruefen.' }
    $preferred = @($ports | Where-Object { $_.Name -match 'CP210|CH340|CH341|Silicon Labs|USB.*Serial|UART|ESP32|Espressif' })
    if ($preferred.Count -eq 1) { return $preferred[0].DeviceID }
    if ($ports.Count -eq 1) { return $ports[0].DeviceID }
    $ports | Format-Table -AutoSize | Out-Host
    throw 'Mehrere Ports vorhanden. Bitte -Port COMx angeben.'
}

try {
    $binaryDir = Join-Path $PSScriptRoot 'firmware'
    $manifest = Get-Content -LiteralPath (Join-Path $binaryDir 'manifest.json') -Raw | ConvertFrom-Json
    if ($manifest.format -ne 1 -or $manifest.chip -ne 'esp32') { throw 'Unpassendes Firmware-Manifest.' }
    $segments = @(
        @{Name='bootloader.bin'; Address='0x1000'},
        @{Name='partitions.bin'; Address='0x8000'},
        @{Name='boot_app0.bin'; Address='0xe000'},
        @{Name='firmware.bin'; Address='0x10000'}
    )
    foreach ($segment in $segments) {
        $file = Join-Path $binaryDir $segment.Name
        $entry = @($manifest.files | Where-Object { $_.file -eq $segment.Name })
        if ($entry.Count -ne 1 -or $entry[0].address -ne $segment.Address) { throw "Ungueltiger Manifest-Eintrag: $($segment.Name)" }
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Binaerdatei fehlt: $file. Bitte das gesamte ZIP entpacken." }
        if ((Get-Item -LiteralPath $file).Length -ne $entry[0].bytes -or (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $entry[0].sha256) {
            throw "Pruefsummenfehler: $($segment.Name). Bitte das Originalpaket erneut entpacken."
        }
    }
    $flasher = Find-Flasher
    $versionOutput = @(& $flasher version)
    if ($LASTEXITCODE -ne 0 -or ($versionOutput -join ' ') -notmatch '\b5\.\d+') { throw 'Dieses Skript benoetigt esptool 5.x (getestet mit 5.3.1).' }
    if ([string]::IsNullOrWhiteSpace($Port)) { $Port = Find-BoardPort }
    $flashArgs = @('--chip','esp32','--port',$Port,'--baud',"$Baud",'--before','default-reset','--after','hard-reset','write-flash','-z','--flash-mode','keep','--flash-freq','keep','--flash-size','keep')
    foreach ($segment in $segments) { $flashArgs += @($segment.Address,(Join-Path $binaryDir $segment.Name)) }
    Write-Host "Firmware: $($manifest.name)" -ForegroundColor Cyan
    Write-Host "Port: $Port | Baud: $Baud"
    Write-Host "Flasher: $flasher"
    Write-Host 'Alle vier SHA256-Pruefsummen sind korrekt.'
    if ($CheckOnly) {
        Write-Host 'Pruefung erfolgreich. Es wurde NICHT geflasht oder ein serieller Port geoeffnet.' -ForegroundColor Green
        exit 0
    }
    Write-Host 'Flashen startet. Serielle Monitore muessen geschlossen sein.' -ForegroundColor Cyan
    & $flasher @flashArgs
    if ($LASTEXITCODE -ne 0) {
        throw "esptool fehlgeschlagen (Exitcode $LASTEXITCODE). Bei 'Failed to connect' BOOT gedrueckt halten und erneut starten; nach Beginn des Schreibens loslassen. Bei Uebertragungsfehlern -Baud 115200 versuchen."
    }
    Write-Host 'Erfolgreich geflasht und durch esptool verifiziert. ESP32 startet neu.' -ForegroundColor Green
    Write-Host 'Vorhandene Touch-Kalibrierung bleibt erhalten. Ohne Kalibrierung die vier Kreuze antippen.'
} catch {
    Write-Host "FEHLER: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
