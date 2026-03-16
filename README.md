# Senrayvar AI Sensor SDK 🚀

A cross-platform serial communication SDK designed for AI vision sensors, supporting **PC (Python)**, **MicroPython (ESP32)**, and **Arduino (C++/ESP32/STM32)**.

---

# 🚀 Quick Start

### 1. Python Users (PC / Raspberry Pi / Jetson)
* **install package**:
   ```bash
   pip install pyserial
   ```
* **Hardware Connection**: Connect the sensor to your computer via a USB-to-TTL module.
* **Configuration**: 
    1. Navigate to the `python/` directory.
    2. Open `examples/pc_demo.py`.
    3. Modify the serial port name (e.g., `COM3` for Windows or `/dev/ttyUSB0` for Linux).
* **Run**:
    ```bash
    python examples/pc_demo.py
    ```

### 2. MicroPython Users (ESP32 / K210)
* **Prerequisite**: Ensure the MicroPython firmware is flashed.
* **Deployment**: 
    1. Use Thonny IDE or the `ampy` tool to upload the `python/ai_sensor/` folder to the `/lib` directory on your development board.
* **Configuration**: Open `examples/esp32_mpy_demo.py` and update the `TX/RX` pin numbers according to your wiring.
* **Run**: Open the script in your IDE and click **"Run"** to view AI recognition results in the console.

### 3. Arduino / ESP32-Arduino Users
* **Dependency**: Requires the `ArduinoJson` library.
* **Library Installation**: 
    1. Download the SDK as a `.zip` file.
    2. In Arduino IDE, go to: `Sketch` -> `Include Library` -> `Add .ZIP Library...` and select the downloaded file.
* **Run**:
    1. Open the example: `File` -> `Examples` -> `My_AI_Sensor_SDK` -> `BasicRead`.
    2. Select your board and port, then click **Upload**.
    3. Open the **Serial Monitor** and set the baud rate to `115200`.

---

## 🛠️ Core API Reference

The SDK provides a consistent interface across all supported platforms:

| Function | Description |
| :--- | :--- |
| `setModel(type)` | Sets the AI model using built-in enums (e.g., `MODEL_DETECT`). |
| `setModelParam(...)` | Dynamically adjusts parameters (Probability, Color H/S/V thresholds, etc.). |
| `setZoom(factor)` | Controls camera zoom level (Supported range: 1.0 - 5.0). |
| `exec()` | Deploys the configuration and starts the sensor. |
| `poll()` | Continuously polls serial data and triggers the result callback. |
| `getBox(obj, coord)` | Retrieves bounding box coordinates (`LEFT`, `TOP`, `RIGHT`, `BOTTOM`). |