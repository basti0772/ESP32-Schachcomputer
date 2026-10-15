#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$docs = Join-Path $root 'docs'
$manifest = Get-Content (Join-Path $docs 'manifest.json') -Raw | ConvertFrom-Json
$info = Get-Content (Join-Path $root 'firmware/build-info.json') -Raw | ConvertFrom-Json
$source = Get-Content (Join-Path $root 'ESP32ChessTouch/ESP32ChessTouch.ino') -Raw
if ($source -notmatch 'ESP32 CHESS ([0-9]+\.[0-9]+(?:\.[0-9]+)?) /') { throw 'Missing source version.' }
if ($manifest.version -ne $matches[1] -or $info.version -ne $manifest.version) { throw 'Version mismatch.' }
if ($manifest.builds.Count -ne 1 -or $manifest.builds[0].chipFamily -ne 'ESP32' -or $info.chipFamily -ne 'ESP32') { throw 'Wrong chip family.' }
if ($manifest.builds[0].parts.Count -ne 1) { throw 'Expected one merged image.' }
$part = $manifest.builds[0].parts[0]
if ($part.offset -ne 0 -or $info.mergedOffset -ne 0) { throw 'Wrong merged offset.' }
# Resolve against /docs only, just like branch-based GitHub Pages.
if ($part.path -ne 'firmware/ESP32-Schachcomputer.bin') { throw 'Unexpected Pages firmware path.' }
$published = Join-Path $docs $part.path
$canonical = Join-Path $root 'firmware/ESP32-Schachcomputer.bin'
foreach ($file in @($published,$canonical)) {
    if ((Get-Item $file).Length -ne $info.bytes) { throw 'Firmware size mismatch.' }
    if ((Get-FileHash $file -Algorithm SHA256).Hash -ne $info.sha256) { throw 'Firmware hash mismatch.' }
}
$data = [IO.File]::ReadAllBytes($canonical)
$end = 0
foreach ($image in $info.parts) {
    if ($image.offset -lt $end -or $image.bytes -le 0) { throw 'Overlapping/empty input image.' }
    $end = $image.offset + $image.bytes
    if ($end -gt $data.Length) { throw 'Image outside merged binary.' }
    $hasher = [Security.Cryptography.SHA256]::Create()
    try {
        $hash = [BitConverter]::ToString($hasher.ComputeHash($data, [int]$image.offset, [int]$image.bytes)).Replace('-','').ToLowerInvariant()
        if ($hash -ne $image.sha256) { throw "Merged content differs from build input: $($image.file)" }
    } finally { $hasher.Dispose() }
}
if ($end -ne $data.Length) { throw 'Merged length mismatch.' }
if ($data[$info.parts[0].offset] -ne 0xE9) { throw 'ESP bootloader header missing at build offset.' }
$boot = [int]$info.parts[0].offset
# Espressif image header: DIO=2, size 4MB=2, frequency 80MHz=0xf.
if ($info.flashMode -ne 'dio' -or $info.flashFrequency -ne '80m' -or $info.flashSize -ne '4MB' -or $data[$boot+2] -ne 2 -or $data[$boot+3] -ne 0x2f) { throw 'Unexpected flash configuration/header.' }
$partition = @($info.parts | Where-Object { $_.file -match '\.partitions\.bin$' })
$application = @($info.parts | Where-Object { $_.file -match '\.ino\.bin$' })
if ($partition.Count -ne 1 -or $application.Count -ne 1) { throw 'Partition/app image missing.' }
$foundApp = $false
for ($i = [int]$partition[0].offset; $i -lt $partition[0].offset + $partition[0].bytes; $i += 32) {
    if ([BitConverter]::ToUInt16($data,$i) -ne 0x50aa) { break }
    if ($data[$i+2] -eq 0 -and $data[$i+3] -eq 0x10) {
        $foundApp = $true
        if ([BitConverter]::ToUInt32($data,$i+4) -ne $application[0].offset -or [BitConverter]::ToUInt32($data,$i+8) -lt $application[0].bytes) { throw 'Application does not match partition table.' }
    }
}
if (-not $foundApp) { throw 'OTA0 application partition not found.' }
if ((Get-Content (Join-Path $docs 'firmware/build-info.json') -Raw) -ne (Get-Content (Join-Path $root 'firmware/build-info.json') -Raw)) { throw 'Metadata copies differ.' }
$html = Get-Content (Join-Path $docs 'index.html') -Raw
if ($html -notmatch 'esp-web-install-button' -or $html -notmatch 'manifest="manifest.json"') { throw 'Installer element missing.' }
Write-Host 'PASS: version, chip, offsets, size, SHA256, Pages paths and installer element.'
