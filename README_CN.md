# Senrayvar AI Sensor SDK 🚀

一款专为 AI 视觉传感器打造的跨平台串口通讯 SDK，提供一致性的开发体验。

## 📦 支持平台
* **PC (Python)**: Windows, Linux, macOS
* **MicroPython**: ESP32, K210, Raspberry Pi Pico
* **Arduino (C++)**: ESP32, STM32, Arduino 全系列

---

# 🚀 快速上手 (Quick Start)

请根据您的开发环境选择对应的指南进行操作。

## 前提条件：需要在web/app上面设置接口为串口和相应的波特率

## 1. Python 用户 (PC / 树莓派 / Jetson)

**前提条件**：已安装 Python 3.x。

### 🛠️ 安装与准备
1. **安装依赖**：
   ```bash
   pip install pyserial
   ```

2. **硬件连接**：将传感器通过 USB 转 TTL 模块连接至电脑。
3. **配置**：
    a. 进入 `python/` 目录，
    b. pip install -e .
    c. 打开 `examples/pc_demo.py`，修改串口号（如 `COM3` 或 `/dev/ttyUSB0`）。
4. **运行**：
    ```bash
    python examples/pc_demo.py
    ```

## 2. MicroPython 用户 (ESP32 / STM32)
* **部署**：使用 Thonny 或 ampy 将 `python/ai_sensor/` 文件夹上传至开发板的 `/lib` 目录。
* **配置**：打开 `examples/mpy_demo.py`，修改 `TX/RX` 引脚编号。
* **运行**：在 IDE 中点击 **“运行”** 即可查看结果。

## 3. Arduino / ESP32 用户
* **依赖**：需安装 `ArduinoJson` 库(>=7.4.3)。
* **安装**：在 Arduino IDE 中选择 `项目` -> `包含库` -> `添加 .ZIP 库...`，导入本 SDK 压缩包。
* **运行**：
    1. 打开 `文件` -> `示例` -> `My_AI_Sensor_SDK` -> `BasicRead`。
    2. 设置波特率为 `115200`，点击 **上传** 并打开 **串口监视器**。

---

# 🛠️ SenRayVarVision SDK 核心 API 参考

该 SDK 为视觉传感器提供了一套跨平台（C++/Python）的一致性接口，用于模型配置、数据获取及结果解析。

## 1. 基础配置接口

| 方法 (C++ / Python) | 参数说明 | 功能描述 |
| :--- | :--- | :--- |
| `setModel(type)` | `type`: 模型枚举值 | 设置传感器运行的 AI 模型（如 `MODEL_DETECT`, `MODEL_TRACK` 等）。 |
| `setModelParam(...)` | `prob`, `prob2`, `h`, `s`, `v` | 动态调整识别阈值（置信度）及颜色识别的 H/S/V 过滤参数。 |
| `setZoom(factor)` | `factor`: 1.0 ~ 5.0 | 控制摄像头的数码变焦倍率，范围限制在 1.0 到 5.0 之间。 |
| `setDetectTrack(track)` | `track`: 0 或 1 | 开启或关闭检测目标的自动追踪功能。 |

## 2. 传感器控制接口

| 方法 | 参数说明 | 功能描述 |
| :--- | :--- | :--- |
| `start(is_async, timeout)` | `is_async`: 是否异步 (默认 true)<br>`timeout`: 同步等待超时(ms) | 将当前配置部署到硬件并启动传感器。若为同步模式，将等待硬件 ACK 响应。 执行过程中如果有更新配置，也可以使用 |
| `stop(is_async, timeout)` | 同上 | 停止当前运行的模型识别任务, 使用start开始 |
| `save(is_async, timeout)` | 同上 | 将当前模型配置持久化保存到传感器 Flash 中; 开机时自动运行程序;start更新会被覆盖 |
| `process()` | 无 | **核心轮询函数**。需放在 `loop()` 或主循环中，负责解析串口数据并触发回调。 |

## 3. 数据获取与解析

### 结果状态
* `set_callback()`: 通过设置回调函数，当有识别到物体的时候会调用此函数，与hasNewData 二选一， demo里面两种使用方式都有
* `hasNewData()`: 检查自上次调用以来是否有新的识别结果到达。
* `getItemSize()`: 返回当前识别到的目标总数。

### 目标详情解析 (静态方法/类方法)
通过传入识别结果中的 `JsonObject` (C++) 或 `dict` (Python) 进行解析：

| 方法 | 说明 |
| :--- | :--- |
| `getLabel(obj)` | 获取目标的分类标签名称。 |
| `getScore(obj)` | 获取目标的识别置信度分数。 |
| `getBox(obj, coord)` | 获取边界框坐标。`coord` 取值：`BOX_LEFT`, `BOX_TOP`, `BOX_RIGHT`, `BOX_BOTTOM`。 |
| `getBoxCenter(obj, axis)`| 获取边界框中心点。`axis` 取值：`POINT_X`, `POINT_Y`。 |
| `getPoint(obj, idx, axis)`| 获取关键点数据。`idx` 为点索引，`axis` 为坐标轴。 |

### 便捷查询接口 (按 ID 或 Label)
支持直接通过索引 `id` 或标签字符串 `label` 快速获取数据，例如：
* `getBoxById(id, coord)`
* `getBoxByLabel(label, coord)`
* `hasLabel(label)`: 检查当前帧是否包含特定标签。

## 4. 常量定义 (枚举)

| 类型 | 枚举值示例 |
| :--- | :--- |
| **ModelType** | `MODEL_DETECT`(1), `MODEL_CLASSFY`(2), `MODEL_PPOCR`(3), `MODEL_FACEREC`(4),`MODEL_TRACK`(5),`MODEL_POSE`(6), `MODEL_QRCODE`(7),`MODEL_COLOR_DETECT`(8),`MODEL_LINE_TRACK`(9) |
| **BoxCoord** | `BOX_LEFT`(0), `BOX_TOP`(1), `BOX_RIGHT`(2), `BOX_BOTTOM`(3) |
| **PointCoord**| `POINT_X`(0), `POINT_Y`(1) |

