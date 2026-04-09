## 📦 Supported Platforms
* **PC (Python)**: Windows, Linux, macOS
* **MicroPython**: ESP32, K210, Raspberry Pi Pico
* **Arduino (C++)**: ESP32, STM32, full Arduino series

---

# 🚀 Quick Start

Please select the appropriate guide based on your development environment.

## Prerequisites: The interface needs to be configured as a serial port and the corresponding baud rate on the web/app.

## 1. Python User (PC / Raspberry Pi / Jetson)

**Prerequisites**: Python 3.x installed.

### 🛠️ Installation & Preparation
1. **Install Dependencies**:
   ```bash
   pip install pyserial
   ```

2. **Hardware Connection**: Connect the sensor to your computer via a USB-to-TTL module.
3. **Configuration**:
   a. Navigate to the `python/` directory,
   b. Run `pip install -e .`
   c. Open `examples/pc_demo.py` and modify the serial port (e.g., `COM3` or `/dev/ttyUSB0`).
4. **Run**:
   ```bash
   python examples/pc_demo.py
   ```

## 2. MicroPython User (ESP32 / STM32)
* **Deployment**: Use Thonny or ampy to upload the `python/ai_sensor/` folder to the `/lib` directory on your board.
* **Configuration**: Open `examples/mpy_demo.py` and modify the `TX/RX` pin numbers.
* **Run**: Click **"Run"** in the IDE to view the results.

## 3. Arduino / ESP32 User
* **Dependencies**: Requires the `ArduinoJson` library(>=7.4.3).
* **Installation**: In Arduino IDE, select `Sketch` -> `Include Library` -> `Add .ZIP Library...`, and import the SDK zip file.
* **Run**:
   1. Open `File` -> `Examples` -> `My_AI_Sensor_SDK` -> `BasicRead`.
   2. Set baud rate to `115200`, click **Upload** and open the **Serial Monitor**.

---

# 🛠️ SenRayVarVision SDK Core API Reference

This SDK provides a set of cross-platform (C++/Python) consistent interfaces for the vision sensor, used for model configuration, data acquisition, and result parsing.

## 1. Basic Configuration Interface

| Method (C++ / Python) | Parameters | Description |
| :--- | :--- | :--- |
| `setModel(type)` | `type`: Model enum value | Sets the AI model to run on the sensor (e.g., `MODEL_DETECT`, `MODEL_TRACK`, etc.). |
| `setModelParam(...)` | `prob`, `prob2`, `h`, `s`, `v` | Dynamically adjusts recognition threshold (confidence) and H/S/V filtering parameters for color detection. |
| `setZoom(factor)` | `factor`: 1.0 ~ 5.0 | Controls the camera's digital zoom factor, limited to the range of 1.0 to 5.0. |
| `setDetectTrack(track)` | `track`: 0 or 1 | Enables or disables auto-tracking of detection targets. |

## 2. Sensor Control Interface

| Method | Parameters | Description |
| :--- | :--- | :--- |
| `start(is_async, timeout)` | `is_async`: Async mode (default true)<br>`timeout`: Sync wait timeout (ms) | Deploys the current configuration to the hardware and starts the sensor. In sync mode, waits for hardware ACK response. If the configuration is updated during execution, it can also be used. |
| `stop(is_async, timeout)` | Same as above | Stops the currently running model recognition task, use start to start. |
| `save(is_async, timeout)` | Same as above | Persists the current model configuration to the sensor's flash memory. The program runs automatically when the sensor boots up; the start update will be overwritten.|
| `process()` | None | **Core polling function**. Must be placed in `loop()` or the main loop; responsible for parsing serial data and triggering callbacks. |

## 3. Data Acquisition & Parsing

### Result Status
* `set_callback()`: By setting a callback function, it will be called when an object is detected. Use either this or `hasNewData()` - the demo shows both approaches.
* `hasNewData()`: Checks if there are new recognition results since the last call.
* `getItemSize()`: Returns the total number of targets recognized in the current frame.

### Target Detail Parsing (Static/Class Methods)
Pass the `JsonObject` (C++) or `dict` (Python) from the recognition result:

| Method | Description |
| :--- | :--- |
| `getLabel(obj)` | Gets the classification label name of the target. |
| `getScore(obj)` | Gets the recognition confidence score of the target. |
| `getBox(obj, coord)` | Gets bounding box coordinates. `coord` options: `BOX_LEFT`, `BOX_TOP`, `BOX_RIGHT`, `BOX_BOTTOM`. |
| `getBoxCenter(obj, axis)` | Gets the center point of the bounding box. `axis` options: `POINT_X`, `POINT_Y`. |
| `getPoint(obj, idx, axis)` | Gets keypoint data. `idx` is the point index, `axis` is the coordinate axis. |

### Convenience Query Interface (by ID or Label)
Supports quick data retrieval by index `id` or label string `label`, such as:
* `getBoxById(id, coord)`
* `getBoxByLabel(label, coord)`
* `hasLabel(label)`: Checks if the current frame contains a specific label.

## 4. Constants (Enums)

| Type | Enum Values |
| :--- | :--- |
| **ModelType** | `MODEL_DETECT`(1), `MODEL_CLASSFY`(2), `MODEL_PPOCR`(3), `MODEL_FACEREC`(4),`MODEL_TRACK`(5),`MODEL_POSE`(6), `MODEL_QRCODE`(7),`MODEL_COLOR_DETECT`(8),`MODEL_LINE_TRACK`(9) |
| **BoxCoord** | `BOX_LEFT`(0), `BOX_TOP`(1), `BOX_RIGHT`(2), `BOX_BOTTOM`(3) |
| **PointCoord**| `POINT_X`(0), `POINT_Y`(1) |