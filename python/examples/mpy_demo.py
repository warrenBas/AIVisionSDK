import time
from ai_sensor import SenRayVarVision, UartAdapter

def on_result(results):
    for obj in results:
        print(obj)
        print(f"Detect: {SenRayVarVision.get_label(obj)}, Score: {SenRayVarVision.get_score(obj)}")
        print(f"Box X: {SenRayVarVision.get_box(obj, SenRayVarVision.BOX_LEFT)}")

if __name__ == "__main__":
    adapter = UartAdapter(1, 115200, 16, 17) #tx, rx
    sensor = SenRayVarVision(adapter)
    sensor.set_callback(on_result)
    
    sensor.set_model(SenRayVarVision.MODEL_DETECT)
    sensor.set_model_param(probability=65)
    sensor.set_zoom(1)
    
    print("Starting sensor...")
    if (sensor.start(False)):
        print("Starting sensor success")
    else:
        print("Starting sensor failed")

    try:
        while True:
            sensor.process()
            time.sleep(0.01)
    except KeyboardInterrupt:
        sensor.stop()


# if __name__ == "__main__":
#     adapter = UartAdapter(1, 115200, 16, 17) #tx, rx
#     sensor = SenRayVarVision(adapter)
#     # sensor.set_callback(on_result)
    
#     sensor.set_model(SenRayVarVision.MODEL_DETECT)
#     sensor.set_model_param(probability=65)
#     sensor.set_zoom(2)
    
#     print("Starting sensor...")
#     if (sensor.start(False)):
#         print("Starting sensor success")
#     else:
#         print("Starting sensor failed")

#     try:
#         while True:
#             sensor.process()
#             if (sensor.has_new_data()):
#                 print(f"getItemSize: {sensor.get_item_size()}");
#                 print(f"label: {sensor.get_label_by_id(0)}");
#                 print(f"Score: {sensor.get_score_by_id(0)}");
#                 print(f"Box Left: {sensor.get_box_center_by_id(0, SenRayVarVision.POINT_X)}");
            
#             time.sleep(0.01)
#     except KeyboardInterrupt:
#         sensor.stop()