# Senrayvar AI Sensor SDK 🚀

一款专为 AI 视觉传感器打造的跨平台串口通讯 SDK，提供一致性的开发体验。

## 📦 支持平台
* **PC (Python)**: Windows, Linux, macOS
* **MicroPython**: ESP32, K210, Raspberry Pi Pico
* **Arduino (C++)**: ESP32, STM32, Arduino 全系列

---

# 🚀 快速上手 (Quick Start)

请根据您的开发环境选择对应的指南进行操作。

## 1. Python 用户 (PC / 树莓派 / Jetson)

**前提条件**：已安装 Python 3.x。

### 🛠️ 安装与准备
1. **安装依赖**：
   ```bash
   pip install pyserial
   ```

2. **硬件连接**：将传感器通过 USB 转 TTL 模块连接至电脑。
3. **配置**：进入 `python/` 目录，打开 `examples/pc_demo.py`，修改串口号（如 `COM3` 或 `/dev/ttyUSB0`）。
4. **运行**：
    ```bash
    # PC端只读demo
    python examples/pc_simple_demo.py
    # PC端写配置和读demo
    python examples/pc_demo.py
    ```

## 2. MicroPython 用户 (ESP32 / K210)
* **部署**：使用 Thonny 或 ampy 将 `python/ai_sensor/` 文件夹上传至开发板的 `/lib` 目录。
* **配置**：打开 `examples/esp32_mpy_demo.py`，修改 `TX/RX` 引脚编号。
* **运行**：在 IDE 中点击 **“运行”** 即可查看结果。

## 3. Arduino / ESP32 用户
* **依赖**：需安装 `ArduinoJson` 库。
* **安装**：在 Arduino IDE 中选择 `项目` -> `包含库` -> `添加 .ZIP 库...`，导入本 SDK 压缩包。
* **运行**：
    1. 打开 `文件` -> `示例` -> `My_AI_Sensor_SDK` -> `BasicRead`。
    2. 设置波特率为 `115200`，点击 **上传** 并打开 **串口监视器**。

---

# 🛠️ 核心功能接口 (API)

SDK 在不同平台上提供了一致的调用接口：

| 函数 | 说明 |
| :--- | :--- |
| `setModel(type)` | 设置 AI 模型（使用内置枚举值，如 `MODEL_DETECT`） |
| `setModelParam(...)` | 动态调整参数（置信度、颜色 H/S/V 阈值等） |
| `setZoom(factor)` | 控制摄像头缩放（支持 1.0 - 5.0） |
| `exec()` | 下发配置并启动传感器工作 |
| `poll()` | 持续轮询串口数据并触发回调函数 |
| `getBox(obj, coord)` | 获取识别框坐标（`LEFT`, `TOP`, `RIGHT`, `BOTTOM`） |

