param([string]$Port = '', [string[]]$Commands = @())
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($Port)) {
    $Port = & "$PSScriptRoot\Find-ESP32Port.ps1"
    if ([string]::IsNullOrWhiteSpace($Port)) { throw 'Kein ESP32-Port gefunden.' }
    $Port = $Port.Trim()
}
$serial = New-Object System.IO.Ports.SerialPort $Port, 115200, 'None', 8, 'One'
$serial.DtrEnable = $false
$serial.RtsEnable = $false
$serial.ReadTimeout = 200
$serial.NewLine = "`n"
function Send-ChessCommand([string]$Text) {
    $serial.DiscardInBuffer()
    $serial.WriteLine($Text)
    $watch = [System.Diagnostics.Stopwatch]::StartNew()
    while ($watch.Elapsed.TotalSeconds -lt 60) {
        try {
            $line = $serial.ReadLine().TrimEnd("`r")
            Write-Host $line
            if ($line -eq 'READY') { return }
        } catch [System.TimeoutException] { }
    }
    throw "ESP32 antwortet nicht vollstaendig auf '$Text'."
}
try {
    $serial.Open()
    Write-Host "ESP32-Schach an $Port, 115200 Baud."
    if ($Commands.Count -gt 0) {
        foreach ($command in $Commands) { Send-ChessCommand $command }
    } else {
        Write-Host 'Befehle: new, e2e4, board, legal, help. exit beendet nur den Client.'
        Send-ChessCommand 'board'
        while ($true) {
            $command = Read-Host 'Schach'
            if ($command -eq 'exit') { break }
            if (-not [string]::IsNullOrWhiteSpace($command)) { Send-ChessCommand $command }
        }
    }
} finally {
    if ($serial.IsOpen) { $serial.Close() }
    $serial.Dispose()
}
