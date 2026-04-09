#include "SenRayVarVision.h"

SenRayVarVision::SenRayVarVision(Stream* uart)
    : _uart(uart), _model_type(1), _probability(65), _probability2(0),
      _color_h(0), _color_s(0), _color_v(0), _detect_track(0), _zoom(1.0),
      _msg_id(0), _callback(nullptr), _doc(VISION_JSON_DOC_SIZE), 
      _new_data_ready(false), _latest_result_ts_ms(0), _exec_cmd_ack(false), rx_len(0) {}

void SenRayVarVision::setModel(int model_type) { _model_type = model_type; }

void SenRayVarVision::setModelParam(int probability, int probability2, int color_h,
                             int color_s, int color_v) {
  _probability = probability; _probability2 = probability2;
  _color_h = color_h; _color_s = color_s; _color_v = color_v;
}

void SenRayVarVision::setDetectTrack(int track) {_detect_track = track;}

void SenRayVarVision::setZoom(float zoom_factor) {
  if (zoom_factor < 1.0) _zoom = 1.0;
  else if (zoom_factor > 5.0) _zoom = 5.0;
  else _zoom = zoom_factor;
}

String SenRayVarVision::generateCode4Full() {
  int final_prob = _probability;
  if (_model_type == MODEL_TRACK) final_prob = _probability * 100 + _probability2;

  String code = "def program_main():\n";
  code += "    global __output__result\n";
  code += "    global __output__senrayvar_start\n";
  code += "    global __output__model_type\n";
  code += "    global __output__model_probability_" + String(_model_type) + "\n";
  code += "    global __output__camera_zoom\n";
  code += "    global __output__detect_track\n";
  if (_detect_track > 0) {
    code += "    global __output__model_probability_5\n";
    code += "    __output__model_probability_5 = " + String(_probability * 100 + _probability2) + "\n";
  }
  code += "    __output__model_type = " + String(_model_type) + "\n";
  code += "    __output__model_probability_" + String(_model_type) + " = " + String(final_prob) + "\n";
  code += "    __output__color_detect_h = " + String(_color_h) + "\n";
  code += "    __output__color_detect_s = " + String(_color_s) + "\n";
  code += "    __output__color_detect_v = " + String(_color_v) + "\n";
  code += "    __output__detect_track = " + String(_detect_track) + "\n";
  code += "    __output__camera_zoom = float(max(1.0, min(5.0, " + String(_zoom) + ")))\n";
  code += "    import json\n";
  
  code += "    if len(__input__model_result) > 0:\n";
  code += "        for res in __input__model_result:\n";
  code += "            if 'box' in res and type(res['box']) is dict:\n";
  code += "                b = res['box']\n";
  code += "                res['box'] = [b.get('left',0), b.get('top',0), b.get('right',0), b.get('bottom',0)]\n";
  code += "        __output__result = json.dumps(__input__model_result, separators=(',', ':'))\n";
  code += "    return\n\n";
  code += "if __name__ == '__main__':\n    program_main()\n";
  return code;
}

String SenRayVarVision::generateCode4Simple() {
  int final_prob = _probability * 100 + _probability2;

  DynamicJsonDocument doc(VISION_EXEC_DOC_SIZE);
  doc["model_type"] = _model_type;
  doc["color_h"] = _color_h;
  doc["color_s"] = _color_s; 
  doc["color_v"] = _color_v;
  doc["detect_track"] = _detect_track; 
  doc["zoom"] = _zoom;
  doc["prob"] = final_prob;

  String code;
  serializeJson(doc, code);
  return code;
}

uint8_t SenRayVarVision::calc_crc(const uint8_t* data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; ++i) crc ^= data[i];
  return crc;
}

bool SenRayVarVision::send(uint8_t cmd, String payload) {
  if (!_uart) return false;
  _msg_id++;
  
  size_t header_len = sizeof(ble_packet_t);
  size_t length = payload.length();
  size_t offset = 0;
  uint8_t fragment_id = 0;
  
  const size_t MAX_PAYLOAD_SIZE = 1000; 

  do {
    size_t frag_data_len = (length > offset) ? (length - offset) : 0;
    if (frag_data_len > MAX_PAYLOAD_SIZE) {
        frag_data_len = MAX_PAYLOAD_SIZE;
    }

    size_t total_len = header_len + frag_data_len;
    uint8_t* buffer = (uint8_t*)malloc(total_len);
    if (!buffer) return false;

    ble_packet_t* pkt = (ble_packet_t*)buffer;
    pkt->magic = 0xAA88; 
    pkt->msg_id = _msg_id; 
    pkt->fragment_id = fragment_id;
    pkt->data_len = frag_data_len; 
    pkt->is_first = (offset == 0) ? 1 : 0; 
    pkt->is_last = (offset + frag_data_len >= length) ? 1 : 0;
    pkt->reserve_bits = 0; 
    pkt->cmd = cmd; 
    pkt->crc = 0;

    if (frag_data_len > 0) {
        memcpy(buffer + header_len, payload.c_str() + offset, frag_data_len);
    }

    pkt->crc = calc_crc(buffer, total_len);
    _uart->write(buffer, total_len);
    free(buffer);

    offset += frag_data_len;
    fragment_id++;

    if (offset < length) {
        delay(5); 
    }

  } while (offset < length);

  return true;
}

bool SenRayVarVision::start(bool is_async, uint32_t timeout_ms) {  
  _exec_cmd_ack = false;
  bool send_ok = false;
#if defined(__AVR__) || defined(ARDUINO_ARCH_NRF51) || defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_MICROBIT)
  String pyCode = generateCode4Simple();
  pyCode.replace("\"", "\\\""); 
  pyCode.replace("\n", "\\n");  
  String payload = "{\"code\":\"" + pyCode + "\",\"mode\":0,\"type\":1}";
  send_ok = send(U_CMD_PYTHON_CODE, payload);
#else
  String pyCode = generateCode4Full();
  DynamicJsonDocument doc(VISION_EXEC_DOC_SIZE);
  doc["code"] = pyCode;
  doc["mode"] = 0;
  String payload;
  serializeJson(doc, payload);
  send_ok = send(U_CMD_PYTHON_CODE, payload);
#endif

  if (!send_ok) return false;
  if (is_async) return true; // Asynchronous mode returns success immediately

  // Synchronous mode: wait for response
  uint32_t start_time = millis();
  while (millis() - start_time < timeout_ms) {
    process();
    if (_exec_cmd_ack) {
      return true; // Received success flag
    }
    delay(2); 
  }
  return false; // Timeout
}

bool SenRayVarVision::stop(bool is_async, uint32_t timeout_ms) {
  bool send_ok = false;
  _exec_cmd_ack = false;
  send_ok = send(U_CMD_PYTHON_CODE, "{\"code\":\"\",\"mode\":1}");
  if (!send_ok) return false;
  if (is_async) return true; // Asynchronous mode returns success immediately

  // Synchronous mode: wait for response
  uint32_t start_time = millis();
  while (millis() - start_time < timeout_ms) {
    process();
    if (_exec_cmd_ack) {
      return true; // Received success flag
    }
    delay(2); 
  }
  return false; // Timeout
}

bool SenRayVarVision::save(bool is_async, uint32_t timeout_ms) {
  bool send_ok = false;
  _exec_cmd_ack = false;

#if defined(__AVR__) || defined(ARDUINO_ARCH_NRF51) || defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_MICROBIT)
  String pyCode = generateCode4Simple();
  pyCode.replace("\"", "\\\""); 
  pyCode.replace("\n", "\\n");  
  String payload = "{\"code\":\"" + pyCode + "\",\"mode\":2,\"type\":1}";
  send_ok = send(U_CMD_PYTHON_CODE, payload);
#else
  String pyCode = generateCode4Full();
  DynamicJsonDocument doc(VISION_EXEC_DOC_SIZE);
  doc["code"] = pyCode;
  doc["mode"] = 2;
  String payload;
  serializeJson(doc, payload);
  send_ok = send(U_CMD_PYTHON_CODE, payload);
#endif

  if (!send_ok) return false;
  if (is_async) return true; // Asynchronous mode returns success immediately

  // Synchronous mode: wait for response
  uint32_t start_time = millis();
  while (millis() - start_time < timeout_ms) {
    process();
    if (_exec_cmd_ack) {
      return true; // Received success flag
    }
    delay(2); 
  }
  return false; // Timeout
}

void SenRayVarVision::setCallback(ResultCallback cb) { _callback = cb; }
bool SenRayVarVision::hasNewData() {
  if (_new_data_ready) {
    _new_data_ready = false; 
    return true;
  }
  return false;
}

void SenRayVarVision::process() {
  if (!_uart) return;
  
  // 1. Read all available data into buffer
  while (_uart->available()) {
    if (rx_len < sizeof(rx_buf)) {
      rx_buf[rx_len++] = _uart->read();
    } else {
      // Buffer full: remove the oldest byte to make space (prevents deadlock)
      memmove(rx_buf, rx_buf + 1, rx_len - 1);
      rx_buf[rx_len - 1] = _uart->read();
    }
  }

  // 2. Process all complete packets in the buffer
  while (rx_len >= sizeof(ble_packet_t)) {
    ble_packet_t* pkt = (ble_packet_t*)rx_buf;
    
    if (pkt->magic == 0xAA88) {
      size_t total_len = sizeof(ble_packet_t) + pkt->data_len;
      
      // Prevent oversized abnormal packets from causing memory deadlock
      if (total_len > sizeof(rx_buf)) {
         memmove(rx_buf, rx_buf + 2, rx_len - 2);
         rx_len -= 2;
         continue;
      }

      if (rx_len >= total_len) {
        uint8_t saved_crc = pkt->crc;
        pkt->crc = 0;
        
        // CRC verification passed
        if (calc_crc(rx_buf, total_len) == saved_crc) {
          if (pkt->cmd == U_CMD_INTERFACE_OUTPUT) {
            DeserializationError error = deserializeJson(_doc, rx_buf + sizeof(ble_packet_t), pkt->data_len);
            if (!error) {
              _latest_result = _doc.as<JsonArray>();
              _latest_result_ts_ms = millis();
              _new_data_ready = true;
              if (_callback) {
                _callback(_latest_result); 
              }
            }
          } 
          // 💡 Added: Capture code execution status callback 
          else if (pkt->cmd == U_CMD_PYTHON_CODE) {
             if (pkt->data_len >= 2) {
                  int16_t status = (int16_t)(rx_buf[sizeof(ble_packet_t)] | (rx_buf[sizeof(ble_packet_t) + 1] << 8));
                 if (status == 0) {
                     _exec_cmd_ack = true;
                 }
             }
          } 
          // else {
          //   uint8_t* p = (uint8_t*)pkt;
          //   for (int i = 0; i < total_len; i++) {
          //     if (p[i] < 0x10) Serial.print("0");
          //     Serial.print(p[i], HEX); Serial.print(" ");
          //   }
          //   Serial.println();
          // }
        }
        
        // Remove parsed packet with memmove regardless of CRC pass/fail, instead of blindly setting rx_len = 0
        memmove(rx_buf, rx_buf + total_len, rx_len - total_len);
        rx_len -= total_len;
      } else {
        // Not enough data for a complete packet, exit loop and wait for next read
        break;
      }
    } else {
      // Magic number mismatch, search for next 0x88 0xAA sync header (little-endian)
      bool found = false;
      for (size_t i = 1; i < rx_len - 1; i++) {
        if (rx_buf[i] == 0x88 && rx_buf[i + 1] == 0xAA) {
          memmove(rx_buf, rx_buf + i, rx_len - i);
          rx_len -= i;
          found = true;
          break;
        }
      }
      if (!found) {
        // If the last byte is 0x88, keep it as it may combine with next 0xAA to form a header
        if (rx_buf[rx_len - 1] == 0x88) {
            rx_buf[0] = 0x88;
            rx_len = 1;
        } else {
            rx_len = 0; // All garbage data, clear directly
        }
      }
    }
  }
}

bool SenRayVarVision::awaitResult(uint32_t timeout_ms) {
  _new_data_ready = false;
  uint32_t start_time = millis();
  while (millis() - start_time < timeout_ms) {
    process();
    if (_new_data_ready) {
      _new_data_ready = false; 
      return true;
    }
    delay(2); 
  }
  return false;
}

JsonArray SenRayVarVision::getLatestResult() { return _latest_result; }
uint32_t SenRayVarVision::getLatestResultTs() { return _latest_result_ts_ms; }

int SenRayVarVision::getItemSize() {
  return _latest_result.size();
}

//object get
float SenRayVarVision::getScore(JsonObject obj) { 
  return obj["score"] | 0.0f; 
}

String SenRayVarVision::getLabel(JsonObject obj) { 
  return obj["label"] | ""; 
}

int SenRayVarVision::getBox(JsonObject obj, BoxCoord coord) {
  JsonArray box = obj["box"]; 
  if (box.isNull() || box.size() < 4) return 0;
  return box[(int)coord].as<int>();
}

int SenRayVarVision::getBoxCenter(JsonObject obj, PointCoord coord) {
  JsonArray box = obj["box"];
  if (box.isNull() || box.size() < 4) return 0;
  if (coord == POINT_X) return (box[0].as<int>() + box[2].as<int>()) / 2; // (Left + Right) / 2
  if (coord == POINT_Y) return (box[1].as<int>() + box[3].as<int>()) / 2; // (Top + Bottom) / 2
  return 0;
}

int SenRayVarVision::getPoint(JsonObject obj, int point, PointCoord coord) {
  JsonArray key_points = obj["key_points"]; 
  if (key_points.isNull() || point >= key_points.size()) return 0;
  JsonArray sub_point = key_points[point];
  if (sub_point.isNull() || sub_point.size() < 2) return 0;
  return sub_point[(int)coord].as<int>();
}

//id get
String SenRayVarVision::getLabelById(int id) {
  if (_latest_result.isNull() || id >= _latest_result.size()) return "";
  return getLabel(_latest_result[id]);
}

float SenRayVarVision::getScoreById(int id) {
  if (_latest_result.isNull() || id >= _latest_result.size()) return 0.0f;
  return getScore(_latest_result[id]);
}

int SenRayVarVision::getBoxById(int id, BoxCoord coord) {
  if (_latest_result.isNull() || id >= _latest_result.size()) return 0;
  return getBox(_latest_result[id], coord);
}

int SenRayVarVision::getBoxCenterById(int id, PointCoord coord) {
  if (_latest_result.isNull() || id >= _latest_result.size()) return 0;
  return getBoxCenter(_latest_result[id], coord);
}

int SenRayVarVision::getPointById(int id, int point, PointCoord coord) {
  if (_latest_result.isNull() || id >= _latest_result.size()) return 0;
  return getPoint(_latest_result[id], point, coord);
}

//label get 
bool SenRayVarVision::hasLabel(const char* label) {
  if (_latest_result.isNull()) return false;
  for (JsonObject obj : _latest_result) {
    if (getLabel(obj) == label) return true;
  }
  return false;
}

float SenRayVarVision::getScoreByLabel(const char* label) {
  if (_latest_result.isNull()) return 0.0f;
  for (JsonObject obj : _latest_result) {
    if (getLabel(obj) == label) return getScore(obj);
  }
  return 0.0f;
}

int SenRayVarVision::getBoxByLabel(const char* label, BoxCoord coord) {
  if (_latest_result.isNull()) return 0;
  for (JsonObject obj : _latest_result) {
    if (getLabel(obj) == label) return getBox(obj, coord);
  }
  return 0;
}

int SenRayVarVision::getBoxCenterByLabel(const char* label, PointCoord coord) {
  if (_latest_result.isNull()) return 0;
  for (JsonObject obj : _latest_result) {
    if (getLabel(obj) == label) return getBoxCenter(obj, coord);
  }
  return 0;
}

int SenRayVarVision::getPointByLabel(const char* label, int point, PointCoord coord) {
  if (_latest_result.isNull()) return 0;
  for (JsonObject obj : _latest_result) {
    if (getLabel(obj) == label) return getPoint(obj, point, coord);
  }
  return 0;
}