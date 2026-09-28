$env:IDF_PATH = 'C:\esp\v6.1\esp-idf'
$env:PATH = 'C:\Espressif\tools\python\v6.1\venv\Scripts;C:\Espressif\tools\ccache\4.12.1\ccache-4.12.1-windows-x86_64;C:\Espressif\tools\xtensa-esp-elf\esp-15.2.0_20251204\xtensa-esp-elf\bin;C:\Espressif\tools\ninja\1.12.1;C:\Espressif\tools\cmake\4.0.3\bin;' + $env:PATH

Write-Host "Building ESP32 firmware with Ninja..." -ForegroundColor Cyan
ninja -C "$PSScriptRoot\build"
