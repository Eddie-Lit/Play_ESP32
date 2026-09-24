@echo off
chcp 65001 >nul
echo 正在请求管理员权限以硬件级重启英特尔蓝牙适配器...
powershell -Command "Start-Process powershell -Verb RunAs -ArgumentList '-ExecutionPolicy Bypass -NoProfile -File \"\"%~dp0reset_intel_bt.ps1\"\"'"
