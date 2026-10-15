#requires -Version 5.1
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$sketch = Join-Path $root 'ESP32ChessTouch'
$build = Join-Path $root 'build/web'
$fqbn = 'esp32:esp32:esp32'
$utf8 = New-Object System.Text.UTF8Encoding($false)
function Write-Json($Path, $Value) {
    [IO.File]::WriteAllText($Path, ($Value | ConvertTo-Json -Depth 12) + "`n", $utf8)
}
Get-Command arduino-cli -ErrorAction Stop | Out-Null
New-Item -ItemType Directory -Force -Path $build | Out-Null
# Force the repository's display setup for every translation unit, without
# modifying the user's installed TFT_eSPI library or other Arduino projects.
$setup = (Join-Path $sketch 'config/User_Setup.h').Replace('\','/')
$flags = '-DUSER_SETUP_LOADED -include "' + $setup + '"'
# Remove machine-specific __FILE__ paths from the distributable binary.
foreach ($prefix in @($root, [Environment]::GetFolderPath('UserProfile'))) {
    if ($prefix) {
        $flags += ' "-ffile-prefix-map=' + $prefix.Replace('\','/') + '=/source"'
        $flags += ' "-ffile-prefix-map=' + $prefix + '=/source"'
    }
}
$argsCompile = @('compile','--fqbn',$fqbn,'--build-path',$build,
    '--build-property',"compiler.cpp.extra_flags=$flags",
    '--build-property',"compiler.c.extra_flags=$flags",$sketch)
Write-Host 'Building ESP32 Chess Touch (clean build)...'
& arduino-cli @argsCompile --clean
if ($LASTEXITCODE -ne 0) { throw 'Arduino compilation failed.' }
$lines = & arduino-cli @argsCompile --show-properties=expanded
if ($LASTEXITCODE -ne 0) { throw 'Cannot read expanded build properties.' }
$props = @{}
foreach ($line in $lines) {
    if ($line -match '^([^=]+)=(.*)$') { $props[$matches[1]] = $matches[2] }
}
foreach ($key in @('build.mcu','build.flash_mode','build.flash_freq','build.flash_size','build.partitions','version')) {
    if (-not $props.ContainsKey($key)) { throw "Missing build property: $key" }
}
if ($props['build.mcu'] -ne 'esp32') { throw 'This installer supports classic ESP32 only.' }
if ($props['version'] -ne '3.3.12') { throw 'Install esp32 core 3.3.12 for this project.' }
# Read input addresses and paths from the actual expanded merge recipe.
# Fail closed if a future core changes the recipe syntax.
$recipes = @($props.Keys | Where-Object { $_ -match '^recipe\..*\.pattern$' -and $props[$_] -match '\smerge-bin\s' })
if ($recipes.Count -ne 1) { throw 'Expected exactly one expanded esptool merge recipe.' }
$recipe = $props[$recipes[0]]
if ($recipe -notmatch '^"([^"]+)"\s+--chip\s+esp32\s+merge-bin\s') { throw 'Unsupported merge recipe.' }
$esptool = $matches[1]
$inputs = [regex]::Matches($recipe, '(0x[0-9a-fA-F]+)\s+"([^"]+\.bin)"')
if ($inputs.Count -lt 2) { throw 'No flash images found in build recipe.' }
$tail = $recipe.Substring($inputs[0].Index)
if ([regex]::Replace($tail, '(0x[0-9a-fA-F]+)\s+"([^"]+\.bin)"', '').Trim()) {
    throw 'Unrecognized flash inputs; review the new core recipe.'
}
$parts = @(); $mergeInputs = @(); $previousEnd = 0
foreach ($inputImage in $inputs) {
    $offset = [Convert]::ToInt32($inputImage.Groups[1].Value.Substring(2),16)
    $file = Get-Item -LiteralPath $inputImage.Groups[2].Value -ErrorAction Stop
    if ($file.Length -eq 0 -or $offset -lt $previousEnd) { throw 'Empty or overlapping flash image.' }
    $previousEnd = $offset + $file.Length
    $parts += [ordered]@{ file=$file.Name; offset=$offset; bytes=$file.Length; sha256=(Get-FileHash $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
    $mergeInputs += @($inputImage.Groups[1].Value, $file.FullName)
}
$source = [IO.File]::ReadAllText((Join-Path $sketch 'ESP32ChessTouch.ino'))
if ($source -notmatch 'ESP32 CHESS ([0-9]+\.[0-9]+(?:\.[0-9]+)?) /') { throw 'Firmware version banner not found.' }
$version = $matches[1]
$merged = Join-Path $build 'ESP32-Schachcomputer.bin'
# Explicit target offset 0: esptool pads gaps with 0xff. Header values come
# from the selected board build, not from guessed defaults or keep/detect.
& $esptool --chip $props['build.mcu'] merge-bin --target-offset 0 --output $merged --flash-mode $props['build.flash_mode'] --flash-freq $props['build.flash_freq'] --flash-size $props['build.flash_size'] @mergeInputs
if ($LASTEXITCODE -ne 0) { throw 'esptool merge failed.' }
$bytes = (Get-Item $merged).Length
if ($bytes -ne $previousEnd) { throw 'Unexpected merged image length.' }
$sha = (Get-FileHash $merged -Algorithm SHA256).Hash.ToLowerInvariant()
$metadata = [ordered]@{
    name='ESP32-Schachcomputer'; version=$version; fqbn=$fqbn; chipFamily='ESP32'; core=$props['version']
    flashMode=$props['build.flash_mode']; flashFrequency=$props['build.flash_freq']; flashSize=$props['build.flash_size']
    partitionScheme=$props['build.partitions']; mergedOffset=0; bytes=$bytes; sha256=$sha
    offsetSource='Expanded Arduino recipe.hooks.objcopy merge-bin recipe'; parts=$parts
}
$manifest = [ordered]@{
    name='ESP32-Schachcomputer'; version=$version; new_install_improv_wait_time=0
    builds=@(@{chipFamily='ESP32'; parts=@(@{path='firmware/ESP32-Schachcomputer.bin'; offset=0})})
}
foreach ($folder in @('firmware','docs/firmware')) {
    $dest = Join-Path $root $folder
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Copy-Item -LiteralPath $merged -Destination (Join-Path $dest 'ESP32-Schachcomputer.bin') -Force
    Write-Json (Join-Path $dest 'build-info.json') $metadata
}
Write-Json (Join-Path $root 'docs/manifest.json') $manifest
& (Join-Path $PSScriptRoot 'Test-Web-Firmware.ps1')
Write-Host "Firmware: firmware/ESP32-Schachcomputer.bin ($bytes bytes), version $version"
Write-Host "SHA256: $sha"
