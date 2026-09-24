Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "  启动 ESP32 Web BLE 控制台本地服务...        " -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan

$pythonExe = "C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe"
if (-not (Test-Path $pythonExe)) {
    $pythonExe = "python"
}

$url = "http://localhost:8000/web_ble_controller.html"
Write-Host "正在打开浏览器: $url" -ForegroundColor Green
Start-Process $url

Write-Host "本地 HTTP 服务已在端口 8000 启动 (按 Ctrl+C 可停止)..." -ForegroundColor Yellow
& $pythonExe -m http.server 8000
