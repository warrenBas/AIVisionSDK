#include "AISensor.h"

AISensor::AISensor(Stream* uart)
    : _uart(uart),
      _model_type(1), 
      _probability(65),
      _probability2(0),
      _color_h(0),
      _color_s(0),
      _color_v(0),
      _zoom(1.0),
      _msg_id(0),
      _callback(nullptr),
      rx_len(0) {}

void AISensor::setModel(int model_type) { _model_type = model_type; }

void AISensor::setModelParam(int probability, int probability2, int color_h,
                             int color_s, int color_v) {
  _probability = probability;
  _probability2 = probability2;
  _color_h = color_h;
  _color_s = color_s;
  _color_v = color_v;
}

void AISensor::setZoom(float zoom_factor) {
  if (zoom_factor < 1.0)
    _zoom = 1.0;
  else if (zoom_factor > 5.0)
    _zoom = 5.0;
  else
    _zoom = zoom_factor;
}

String AISensor::generateCode() {
  int final_prob = _probability;
  // 3. MODEL_TRACK Probability calculation logic
  if (_model_type == MODEL_TRACK) {
    final_prob = _probability * 100 + _probability2;
  }

  String code = "def program_main():\n";
  code += "    global __output__result\n";
  code += "    global __output__senrayvar_start\n";
  code += "    global __output__model_type\n";
  code +=
      "    global __output__model_probability_" + String(_model_type) + "\n";
  code += "    global __output__camera_zoom\n";
  code += "    __output__model_type = " + String(_model_type) + "\n";
  code += "    __output__model_probability_" + String(_model_type) + " = " +
          String(final_prob) + "\n";
  code += "    __output__color_detect_h = " + String(_color_h) + "\n";
  code += "    __output__color_detect_s = " + String(_color_s) + "\n";
  code += "    __output__color_detect_v = " + String(_color_v) + "\n";
  code += "    __output__camera_zoom = float(max(1.0, min(5.0, " +
          String(_zoom) + ")))\n";
  code += "    import json\n";
  code += "    __output__result = json.dumps(__input__model_result)\n";
  code += "    return\n\n";
  code += "if __name__ == '__main__':\n";
  code += "    program_main()\n";
  return code;
}

uint8_t AISensor::calc_crc(const uint8_t* data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; ++i) crc ^= data[i];
  return crc;
}

bool AISensor::send(uint8_t cmd, String payload) {
  _msg_id++;
  size_t header_len = sizeof(ble_packet_t);
  size_t data_len = payload.length();
  size_t total_len = header_len + data_len;

  uint8_t* buffer = (uint8_t*)malloc(total_len);
  if (!buffer) return false;

  ble_packet_t* pkt = (ble_packet_t*)buffer;
  pkt->magic = 0xAA88;
  pkt->msg_id = _msg_id;
  pkt->fragment_id = 0;
  pkt->data_len = data_len;
  pkt->is_first = 1;
  pkt->is_last = 1;
  pkt->reserve_bits = 0;
  pkt->cmd = cmd;
  pkt->crc = 0;

  memcpy(buffer + header_len, payload.c_str(), data_len);
  pkt->crc = calc_crc(buffer, total_len);

  _uart->write(buffer, total_len);
  free(buffer);

  return true;
}

bool AISensor::exec(uint32_t timeout_ms) {
  String pyCode = generateCode();
  DynamicJsonDocument doc(2048);
  doc["code"] = pyCode;
  doc["mode"] = 0;
  String payload;
  serializeJson(doc, payload);
  return send(U_CMD_PYTHON_CODE, payload);
}

bool AISensor::stop(uint32_t timeout_ms) {
  return send(U_CMD_PYTHON_CODE, "{\"code\":\"\",\"mode\":1}");
}

void AISensor::setCallback(ResultCallback cb) { _callback = cb; }

void AISensor::poll() {
  while (_uart->available()) {
    if (rx_len < sizeof(rx_buf)) {
      rx_buf[rx_len++] = _uart->read();
    } else {
      rx_len = 0;  // overflow
    }
  }

  if (rx_len >= sizeof(ble_packet_t)) {
    ble_packet_t* pkt = (ble_packet_t*)rx_buf;
    if (pkt->magic == 0xAA88) {
      size_t total_len = sizeof(ble_packet_t) + pkt->data_len;
      if (rx_len >= total_len) {
        uint8_t saved_crc = pkt->crc;
        pkt->crc = 0;
        if (calc_crc(rx_buf, total_len) == saved_crc) {
          if (pkt->cmd == U_CMD_INTERFACE_OUTPUT && _callback) {
            String payload((char*)(rx_buf + sizeof(ble_packet_t)),
                           pkt->data_len);
            DynamicJsonDocument doc(2048);
            if (!deserializeJson(doc, payload)) {
              _callback(doc.as<JsonArray>());
            }
          }
        }
        rx_len = 0;
      }
    } else {
      for (size_t i = 1; i < rx_len; i++) {
        if (rx_buf[i] == 0x88 && rx_buf[i + 1] == 0xAA) {
          memmove(rx_buf, rx_buf + i, rx_len - i);
          rx_len -= i;
          return;
        }
      }
      rx_len = 0;
    }
  }
}

float AISensor::getScore(JsonObject obj) { return obj["score"] | 0.0f; }
String AISensor::getLabel(JsonObject obj) { return obj["label"] | ""; }

int AISensor::getBox(JsonObject obj, int coord) {
  JsonObject box = obj["box"];
  if (coord == BOX_LEFT) return box["left"] | 0;
  if (coord == BOX_TOP) return box["top"] | 0;
  if (coord == BOX_RIGHT) return box["right"] | 0;
  if (coord == BOX_BOTTOM) return box["bottom"] | 0;
  return 0;
}