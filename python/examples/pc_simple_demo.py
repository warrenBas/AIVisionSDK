import sys
import os
import time
from ai_sensor import AISensor, UartAdapter

def on_result(results):
    for obj in results:
        print(f"Detect: {AISensor.get_label(obj)}, Score: {AISensor.get_score(obj)}")
        print(f"Box X: {AISensor.get_box(obj, AISensor.BOX_LEFT)}")

if __name__ == "__main__":
    adapter = UartAdapter("/dev/cu.usbserial-58570111141", 115200) # 根据实际串口修改
    sensor = AISensor(adapter)
    sensor.set_callback(on_result)
    
    try:
        while True:
            sensor.poll()
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("exiting...")