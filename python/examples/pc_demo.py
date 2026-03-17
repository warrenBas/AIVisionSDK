import time
from ai_sensor import AISensor, UartAdapter

def on_result(results):
    for obj in results:
        print(obj)
        print(f"Detect: {AISensor.get_label(obj)}, Score: {AISensor.get_score(obj)}")
        print(f"Box X: {AISensor.get_box(obj, AISensor.BOX_LEFT)}")

if __name__ == "__main__":
    adapter = UartAdapter("/dev/cu.usbserial-58570111141", 115200) # 根据实际串口修改
    sensor = AISensor(adapter)
    sensor.set_callback(on_result)
    
    sensor.set_model(AISensor.MODEL_DETECT)
    sensor.set_model_param(probability=80)
    sensor.set_zoom(2)
    
    print("Starting sensor...")
    sensor.exec()
    
    try:
        while True:
            sensor.poll()
            time.sleep(0.01)
    except KeyboardInterrupt:
        sensor.stop()