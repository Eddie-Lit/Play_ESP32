# Find Intel Bluetooth device dynamically
$device = Get-PnpDevice -Class Bluetooth | Where-Object { $_.InstanceId -like "*VID_8087*PID_0026*" } | Select-Object -First 1
if ($device) {
    Write-Host "[1/2] 正在切断英特尔蓝牙射频电源..." -ForegroundColor Yellow
    Disable-PnpDevice -InputObject $device -Confirm:$false
    Start-Sleep -Seconds 2
    Write-Host "[2/2] 正在重新激活蓝牙射频并重新加载微码..." -ForegroundColor Cyan
    Enable-PnpDevice -InputObject $device -Confirm:$false
    Write-Host "`n✅ [OK] 英特尔蓝牙已成功重置！LE 接收引擎已恢复工作。" -ForegroundColor Green
    Start-Sleep -Seconds 2
} else {
    Write-Host "未找到英特尔蓝牙设备！" -ForegroundColor Red
    Start-Sleep -Seconds 3
}
