# ESP-IDF BLE 舵机与 LED 智能控制工程（Web Bluetooth）

这是一个基于 ESP-IDF (NimBLE 蓝牙协议栈) 与 Web Bluetooth API 实现的无线控制工程，支持电脑 Chrome / Edge 浏览器无线连接 ESP32，实现对舵机角度与 LED 的联动控制，并与板载/外接 KY040 旋转编码器双向实时同步。

---

## 硬件引脚分配 (Pinout)

| 硬件外设 | ESP32 引脚 | 信号说明 |
| :--- | :--- | :--- |
| **SG90 / 舵机信号线** | **GPIO 2** | 50Hz LEDC PWM (脉宽 0.5ms~2.5ms 对应 0°~180° 旋转) |
| **LED 指示灯** | **GPIO 4** | 数字输出 (低电平 0V 熄灭，高电平 3.3V 点亮) |
| **KY040 旋转编码器 CLK** | **GPIO 18** | GPIO 中断输入（内部上拉） |
| **KY040 旋转编码器 DT** | **GPIO 19** | GPIO 中断输入（内部上拉） |
| **KY040 旋转编码器 SW** | **GPIO 21** | 按钮输入（内部上拉，按下接地） |

---

## 功能特性

1. **GPIO 2 舵机 0°~180° 精准角度控制**：
   - 采用 14-bit 高精度 LEDC PWM，标准 50Hz (20ms 周期)；
   - 脉冲宽度线性映射（0.5ms 对应 0°，1.5ms 对应 90°，2.5ms 对应 180°）。
2. **GPIO 4 LED 状态联动**：
   - 0° 对应 LED 0（关灯）；
   - 180° 对应 LED 1（开灯）。
3. **网页端一键切换控制 (Web BLE)**：
   - 主按钮支持一键在 **0° (关灯)** 与 **180° (开灯)** 之间瞬间切换；
   - 配备实时动态表盘（舵机指针随角度偏转）与仿真发光灯泡；
   - 提供 0°~180° 连续滑块及常用预设角度按钮（0°/45°/90°/135°/180°）。
4. **旋钮保留连续角度控制 (KY040)**：
   - 顺时针旋转：舵机角度逐步增加（步长 5°）；
   - 逆时针旋转：舵机角度逐步减少（步长 5°）；
   - SW 按键短按：一键在 0°/关灯 与 180°/开灯 间切换；
   - 旋钮所有操作均通过 BLE Notify 实时反馈到网页仪表盘。

---

## 快速上手

### 1. 编译并烧录固件
在工程根目录执行编译脚本：
```powershell
powershell -ExecutionPolicy Bypass -File .\build_firmware.ps1
```
烧录至 ESP32（默认 COM5 端口，或指定端口）：
```powershell
powershell -ExecutionPolicy Bypass -File .\flash_firmware.ps1 -Port COM5
```

### 2. 启动网页控制端
运行一键启动脚本：
```powershell
powershell -ExecutionPolicy Bypass -File .\start_web_controller.ps1
```
或手动开启本地服务器：
```powershell
python -m http.server 8000
```
在 Chrome 或 Edge 浏览器中访问：
👉 **http://localhost:8000/web_ble_controller.html** (或 `http://localhost:8000/index.html`)

### 3. 操作指引
1. 打开网页并确保电脑已开启蓝牙；
2. 点击 **“扫描并连接设备”**，在弹出的窗口中选择 **`ESP32-BLE-LED`** 并配对；
3. 点击 **“一键转到 180° (开灯)”** 或 **“一键转到 0° (关灯)”**；
4. 旋转物理旋钮，观察舵机转动及网页表盘指针实时同步。
