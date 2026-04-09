import json
import time
from ai_sensor.protocol import pack_packet, unpack_header, HEADER_SIZE, calc_crc

class SenRayVarVision:
    U_CMD_PYTHON_CODE = 7
    U_CMD_INTERFACE_OUTPUT = 40

    # 1. Model enumeration (ModelType)
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

    # 2. Frame coordinate enumeration (BoxCoord)
    BOX_LEFT = 0
    BOX_TOP = 1
    BOX_RIGHT = 2
    BOX_BOTTOM = 3

    # 3. Point coordinate enumeration (PointCoord)
    POINT_X = 0
    POINT_Y = 1

    def __init__(self, uart_adapter):
        self.uart = uart_adapter
        
        # Configuration parameters
        self.model_type = self.MODEL_DETECT
        self.probability = 65
        self.probability2 = 0
        self.color_h = 0
        self.color_s = 0
        self.color_v = 0
        self.detect_track = 0
        self.zoom = 1.0
        
        # state variables
        self.msg_id_counter = 0
        self.callback = None
        self.rx_buffer = bytearray()
        
        # Data reception related
        self.latest_result = []
        self.new_data_ready = False
        self.exec_cmd_ack = False

    # ========================== Parameter configuration ==========================

    def set_model(self, model_type: int):
        self.model_type = model_type

    def set_model_param(self, probability: int, probability2: int = 65, color_h: int = 10, color_s: int = 40, color_v: int = 0):
        self.probability = probability
        self.probability2 = probability2
        self.color_h = color_h
        self.color_s = color_s
        self.color_v = color_v

    def set_detect_track(self, track: int):
        self.detect_track = track

    def set_zoom(self, zoom_factor: float):
        if zoom_factor < 1.0:
            self.zoom = 1.0
        elif zoom_factor > 5.0:
            self.zoom = 5.0
        else:
            self.zoom = float(zoom_factor)

    def _generate_code(self) -> str:
        final_prob = self.probability * 100 + self.probability2 if self.model_type == self.MODEL_TRACK else self.probability

        code = "def program_main():\n"
        code += "    global __output__result\n"
        code += "    global __output__senrayvar_start\n"
        code += "    global __output__model_type\n"
        code += f"    global __output__model_probability_{self.model_type}\n"
        code += "    global __output__camera_zoom\n"
        code += "    global __output__detect_track\n"
        if self.detect_track > 0:
            code += "    global __output__model_probability_5\n"
            code += f"    __output__model_probability_5 = {self.probability * 100 + self.probability2}\n"
        code += f"    __output__model_type = {self.model_type}\n"
        code += f"    __output__model_probability_{self.model_type} = {final_prob}\n"
        code += f"    __output__color_detect_h = {self.color_h}\n"
        code += f"    __output__color_detect_s = {self.color_s}\n"
        code += f"    __output__color_detect_v = {self.color_v}\n"
        code += f"    __output__detect_track = {self.detect_track}\n"
        code += f"    __output__camera_zoom = float(max(1.0, min(5.0, {self.zoom})))\n"
        code += "    import json\n"
        code += "    if len(__input__model_result) > 0:\n"
        code += "        for res in __input__model_result:\n"
        code += "            if 'box' in res and type(res['box']) is dict:\n"
        code += "                b = res['box']\n"
        code += "                res['box'] = [b.get('left',0), b.get('top',0), b.get('right',0), b.get('bottom',0)]\n"
        code += "        __output__result = json.dumps(__input__model_result, separators=(',', ':'))\n"
        code += "    return\n\n"
        code += "if __name__ == '__main__':\n    program_main()\n"
        
        return code

    # ========================== command control ==========================

    def _wait_for_ack(self, timeout_ms: int) -> bool:
        # Compatible with different platforms for obtaining millisecond timestamps
        get_ms = getattr(self.uart, "get_time_ms", lambda: int(time.time() * 1000))
        start_time = get_ms()
        
        while (get_ms() - start_time) < timeout_ms:
            self.process()
            if self.exec_cmd_ack:
                return True
            time.sleep(0.002)
        return False

    def start(self, is_async: bool = True, timeout_ms: int = 2000) -> bool:
        self.exec_cmd_ack = False
        code = self._generate_code()
        payload = json.dumps({"code": code, "mode": 0}).encode('utf-8')
        self._send(self.U_CMD_PYTHON_CODE, payload)
        
        if is_async: return True
        return self._wait_for_ack(timeout_ms)

    def stop(self, is_async: bool = True, timeout_ms: int = 2000) -> bool:
        self.exec_cmd_ack = False
        payload = json.dumps({"code": "", "mode": 1}).encode('utf-8')
        self._send(self.U_CMD_PYTHON_CODE, payload)
        
        if is_async: return True
        return self._wait_for_ack(timeout_ms)

    def save(self, is_async: bool = True, timeout_ms: int = 2000) -> bool:
        self.exec_cmd_ack = False
        code = self._generate_code()
        payload = json.dumps({"code": code, "mode": 2}).encode('utf-8')
        self._send(self.U_CMD_PYTHON_CODE, payload)
        
        if is_async: return True
        return self._wait_for_ack(timeout_ms)

    # ========================== Data reception and processing ==========================

    def _send(self, cmd: int, payload: bytes):
        self.uart.read_all()
        self.rx_buffer.clear()
        
        self.msg_id_counter = (self.msg_id_counter + 1) & 0xFFFF
        
        length = len(payload)
        offset = 0
        fragment_id = 0
        MAX_PAYLOAD_SIZE = 1000

        while True:
            frag_data_len = length - offset if length > offset else 0
            if frag_data_len > MAX_PAYLOAD_SIZE:
                frag_data_len = MAX_PAYLOAD_SIZE
            
            is_first = 1 if offset == 0 else 0
            is_last = 1 if (offset + frag_data_len >= length) else 0
            
            frag_payload = payload[offset : offset + frag_data_len]
            
            packet = pack_packet(
                msg_id=self.msg_id_counter,
                cmd=cmd,
                payload=frag_payload,
                fragment_id=fragment_id,
                is_first=is_first,
                is_last=is_last
            )
            
            self.uart.write(packet)
            
            offset += frag_data_len
            fragment_id += 1
            
            if offset < length:
                time.sleep(0.005)
            else:
                break

    def set_callback(self, callback_func):
        self.callback = callback_func

    def has_new_data(self) -> bool:
        if self.new_data_ready:
            self.new_data_ready = False
            return True
        return False

    def process(self):
        data = self.uart.read_all()
        if data:
            self.rx_buffer.extend(data)

        while len(self.rx_buffer) >= HEADER_SIZE:
            # Find frame header
            if self.rx_buffer[0] != 0xaa or self.rx_buffer[1] != 0x88: # 0xAA88 in little-endian
                if self.rx_buffer[0] == 0x88 and self.rx_buffer[1] == 0xaa:
                    pass
                else:
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
                if header['cmd'] == self.U_CMD_INTERFACE_OUTPUT:
                    try:
                        payload_str = packet_data[HEADER_SIZE:].decode('utf-8', 'ignore')
                        parsed = json.loads(payload_str)
                        self.latest_result = parsed if isinstance(parsed, list) else []
                        self.new_data_ready = True
                        if self.callback:
                            self.callback(self.latest_result)
                    except Exception as e:
                        print(f"JSON Decode error: {e}")
                        
                elif header['cmd'] == self.U_CMD_PYTHON_CODE:
                        # Capture code execution status callback (read the first two bytes of the payload)                    if len(packet_data) >= HEADER_SIZE + 2:
                        status = packet_data[HEADER_SIZE] | (packet_data[HEADER_SIZE + 1] << 8)
                        if status == 0:
                            self.exec_cmd_ack = True

    def await_result(self, timeout_ms: int = 200) -> bool:
        self.new_data_ready = False
        get_ms = getattr(self.uart, "get_time_ms", lambda: int(time.time() * 1000))
        start_time = get_ms()
        
        while (get_ms() - start_time) < timeout_ms:
            self.process()
            if self.new_data_ready:
                self.new_data_ready = False
                return True
            time.sleep(0.002)
        return False

    def get_latest_result(self) -> list:
        return self.latest_result

    def get_item_size(self) -> int:
        return len(self.latest_result)

    # ========================== Object Extract data ==========================

    @staticmethod
    def get_score(obj: dict) -> float:
        return float(obj.get('score', 0.0))

    @staticmethod
    def get_label(obj: dict) -> str:
        return str(obj.get('label', ''))

    @staticmethod
    def get_box(obj: dict, coord: int) -> int:
        box = obj.get('box', [])
        if not box or len(box) < 4: return 0
        return int(box[coord])

    @staticmethod
    def get_box_center(obj: dict, coord: int) -> int:
        box = obj.get('box', [])
        if not box or len(box) < 4: return 0
        if coord == SenRayVarVision.POINT_X:
            return (int(box[0]) + int(box[2])) // 2
        elif coord == SenRayVarVision.POINT_Y:
            return (int(box[1]) + int(box[3])) // 2
        return 0

    @staticmethod
    def get_point(obj: dict, point: int, coord: int) -> int:
        key_points = obj.get('key_points', [])
        if not key_points or point >= len(key_points): return 0
        sub_point = key_points[point]
        if not sub_point or len(sub_point) < 2: return 0
        return int(sub_point[coord])

    # ========================== ID Extract data ==========================

    def get_label_by_id(self, item_id: int) -> str:
        if item_id >= len(self.latest_result): return ""
        return self.get_label(self.latest_result[item_id])

    def get_score_by_id(self, item_id: int) -> float:
        if item_id >= len(self.latest_result): return 0.0
        return self.get_score(self.latest_result[item_id])

    def get_box_by_id(self, item_id: int, coord: int) -> int:
        if item_id >= len(self.latest_result): return 0
        return self.get_box(self.latest_result[item_id], coord)

    def get_box_center_by_id(self, item_id: int, coord: int) -> int:
        if item_id >= len(self.latest_result): return 0
        return self.get_box_center(self.latest_result[item_id], coord)

    def get_point_by_id(self, item_id: int, point: int, coord: int) -> int:
        if item_id >= len(self.latest_result): return 0
        return self.get_point(self.latest_result[item_id], point, coord)

    # ========================== Label Extract data ==========================

    def has_label(self, label: str) -> bool:
        for obj in self.latest_result:
            if self.get_label(obj) == label:
                return True
        return False

    def get_score_by_label(self, label: str) -> float:
        for obj in self.latest_result:
            if self.get_label(obj) == label:
                return self.get_score(obj)
        return 0.0

    def get_box_by_label(self, label: str, coord: int) -> int:
        for obj in self.latest_result:
            if self.get_label(obj) == label:
                return self.get_box(obj, coord)
        return 0

    def get_box_center_by_label(self, label: str, coord: int) -> int:
        for obj in self.latest_result:
            if self.get_label(obj) == label:
                return self.get_box_center(obj, coord)
        return 0

    def get_point_by_label(self, label: str, point: int, coord: int) -> int:
        for obj in self.latest_result:
            if self.get_label(obj) == label:
                return self.get_point(obj, point, coord)
        return 0