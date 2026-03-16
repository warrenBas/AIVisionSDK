import json
from ai_sensor.protocol import pack_packet, unpack_header, HEADER_SIZE, calc_crc

class AISensor:
    U_CMD_PYTHON_CODE = 1
    U_CMD_INTERFACE_OUTPUT = 40

    # 1. Model enumeration
    MODEL_DETECT = 1
    MODEL_CLASSFY = 2
    MODEL_PPOCR = 3
    MODEL_FACEREC = 4
    MODEL_TRACK = 5
    MODEL_POSE = 6
    MODEL_QRCODE = 7
    MODEL_COLOR_DETECT = 8
    MODEL_LINE_TRACK = 9
    MODEL_VOICE_ASR = 10

    # 2. Coordinate enumeration
    BOX_LEFT = 1
    BOX_TOP = 2
    BOX_RIGHT = 3
    BOX_BOTTOM = 4

    def __init__(self, uart_adapter):
        self.uart = uart_adapter
        self.model_type = self.MODEL_DETECT
        self.model_params = {"probability": 65, "probability2": 65, "color_h": 10, "color_s": 40, "color_v": 40}
        self.zoom = 1.0
        self.msg_id_counter = 0
        self.callback = None
        self.rx_buffer = bytearray()

    def set_model(self, model_type: int):
        self.model_type = model_type

    #support probability, probability2, color_h, color_s, color_v
    def set_model_param(self, **kwargs):
        self.model_params.update(kwargs)

    def set_zoom(self, zoom_factor: float):
        self.zoom = max(1.0, min(5.0, float(zoom_factor)))

    def _generate_code(self) -> str:
        prob = self.model_params.get("probability", 65)
        prob2 = self.model_params.get("probability2", 0)
        color_h = self.model_params.get("color_h", 0)
        color_s = self.model_params.get("color_s", 0)
        color_v = self.model_params.get("color_v", 0)
        
        #MODEL_TRACK Probability calculation logic
        if self.model_type == self.MODEL_TRACK:
            final_prob = prob * 100 + prob2
        else:
            final_prob = prob

        return f"""def program_main():
    global __output__result
    global __output__senrayvar_start
    global __output__model_type
    global __output__model_probability_{self.model_type}
    global __output__camera_zoom
    __output__model_type = {self.model_type}
    __output__model_probability_{self.model_type} = {final_prob}
    __output__color_detect_h = {color_h}
    __output__color_detect_s = {color_s}
    __output__color_detect_v = {color_v}
    __output__camera_zoom = float(max(1.0, min(5.0, {self.zoom})))
    import json
    __output__result = json.dumps(__input__model_result)
    return

if __name__ == "__main__":
    program_main()
"""

    def exec(self):
        self.stop()
        code = self._generate_code()
        payload = json.dumps({"code": code, "mode": 0}).encode('utf-8')
        return self._send(self.U_CMD_PYTHON_CODE, payload)

    def stop(self):
        payload = json.dumps({"code": "", "mode": 1}).encode('utf-8')
        return self._send(self.U_CMD_PYTHON_CODE, payload)

    def get_results(self, timeout=2.0):
        start_time = self.uart.get_time_ms()
        timeout_ms = timeout * 1000
        while (self.uart.get_time_ms() - start_time) < timeout_ms:
            self.poll()
        raise Exception("Wait for result timeout")

    def set_callback(self, callback_func):
        self.callback = callback_func

    def _send(self, cmd, payload):
        self.uart.read_all() 
        self.rx_buffer.clear()
        
        self.msg_id_counter = (self.msg_id_counter + 1) & 0xFFFF
        target_msg_id = self.msg_id_counter
        packet = pack_packet(target_msg_id, cmd, payload)
        self.uart.write(packet)
        
    def poll(self):
        data = self.uart.read_all()
        if data:
            self.rx_buffer.extend(data)
            
        last_cmd_response = None

        while len(self.rx_buffer) >= HEADER_SIZE:
            if self.rx_buffer[0] != 0x88 or self.rx_buffer[1] != 0xaa:
                self.rx_buffer.pop(0)
                continue

            header = unpack_header(self.rx_buffer)
            total_len = HEADER_SIZE + header['data_len']
            
            if len(self.rx_buffer) < total_len:
                break

            packet_data = self.rx_buffer[:total_len]
            del self.rx_buffer[:total_len]

            saved_crc = packet_data[10]
            temp_packet = bytearray(packet_data)
            temp_packet[10] = 0
            if calc_crc(temp_packet) == saved_crc:
                try:
                    payload = packet_data[HEADER_SIZE:].decode('utf-8', 'ignore')
                    
                    if header['cmd'] == self.U_CMD_INTERFACE_OUTPUT:
                        if self.callback and payload.strip() != "[]":
                            self.callback(json.loads(payload))
                    else:
                        last_cmd_response = {
                            "msg_id": header['msg_id'], 
                            "cmd": header['cmd'], 
                            "data": payload
                        }
                except Exception as e:
                    print(f"Decode error: {e}")
            
        return last_cmd_response

    @staticmethod
    def get_box(obj, coord=None):
        box = obj.get('box', {})
        if coord == AISensor.BOX_LEFT: return box.get('left', 0)
        if coord == AISensor.BOX_TOP: return box.get('top', 0)
        if coord == AISensor.BOX_RIGHT: return box.get('right', 0)
        if coord == AISensor.BOX_BOTTOM: return box.get('bottom', 0)
        return [box.get('left', 0), box.get('top', 0), box.get('right', 0), box.get('bottom', 0)]
    
    @staticmethod
    def get_label(obj):
        return obj.get('label', '')

    @staticmethod
    def get_score(obj):
        return obj.get('score', 0.0)