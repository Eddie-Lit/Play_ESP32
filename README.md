# ESP-IDF BLE LED 控制工程（Web Bluetooth）

这是一个基于 ESP-IDF (NimBLE 蓝牙栈) 与 Web Bluetooth API 实现的无线控制 LED 开关工程。可以通过电脑的 Chrome 或 Edge 浏览器直接配对 ESP32，实现无线点灯与双向状态同步。

## 架构说明

- **ESP32 固件端**：
  - 采用轻量低功耗的 **NimBLE** 蓝牙协议栈，运行 BLE GATT Server。
  - **设备名称**：`ESP32-BLE-LED`
  - **服务 UUID (Service)**：`4fafc201-1fb5-459e-8fcc-c5c9c331914b`
  - **特征值 UUID (Characteristic)**：`beb5483e-36e1-4688-b7f5-ea07361b26a8`
  - 支持 **Read**（获取当前灯状态）、**Write**（控制 1/0）、**Notify**（硬件状态变更主动通知网页）。
  - 控制引脚：默认 **GPIO 2**（典型 ESP32 开发板板载 LED）。

- **电脑控制端**：
  - [web_ble_controller.html](file:///d:/Play_ESP32/web_ble_controller.html)：基于 Web Bluetooth API 的现代化控制页面。
  - 支持实时扫描设备、状态灯泡仿真发光动效、双向数据通信日志。

---

## 快速上手指南

### 1. 编译并烧录固件到 ESP32
连接 ESP32 开发板至电脑（如 COM5 端口），在终端运行：
```powershell
idf.py build
idf.py -p COM5 flash monitor
```
烧录后观察串口输出，显示 `Waiting for Web Bluetooth connection...` 表示蓝牙广播已正常启动。

### 2. 打开网页控制端
> ⚠️ **重要提示**：现代浏览器安全策略要求 Web Bluetooth 必须运行在 HTTPS 或 `http://localhost` 安全上下文中，不能直接双击用 `file://` 打开。

运行工程根目录下的一键脚本：
```powershell
powershell -ExecutionPolicy Bypass -File .\start_web_controller.ps1
```
或者手动执行：
```powershell
python -m http.server 8000
```
并在 Chrome 或 Edge 浏览器中访问：
👉 **http://localhost:8000/web_ble_controller.html**

### 3. 操作步骤
1. 确保电脑已开启蓝牙。
2. 在网页上点击 **“扫描并连接设备”**。
3. 在浏览器弹出的设备列表中选中 **`ESP32-BLE-LED`**，点击“配对”。
4. 配对成功后，点击 **“点我开灯” / “点我关灯”** 即可实时控制 ESP32 开发板上的 LED！
