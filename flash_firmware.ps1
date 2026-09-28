param(
    [string]$Port = "COM5"
)

$python = "C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe"
$esptool = "C:\esp\v6.1\esp-idf\components\esptool_py\esptool\esptool.py"

Write-Host "Flashing firmware to ESP32 on $Port..." -ForegroundColor Cyan

Push-Location "$PSScriptRoot\build"
try {
    & $python $esptool -p $Port -b 460800 --before default-reset --after hard-reset --chip esp32 write_flash "@flasher_args.json"
} finally {
    Pop-Location
}
