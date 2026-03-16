#ifndef AI_SENSOR_H
#define AI_SENSOR_H

#include <Arduino.h>
#include <ArduinoJson.h>  //need install ArduinoJson

#pragma pack(push, 1)
typedef struct {
  uint16_t magic;
  uint16_t msg_id;
  uint16_t fragment_id;
  uint16_t data_len;
  uint8_t is_first : 1;
  uint8_t is_last : 1;
  uint8_t reserve_bits : 6;
  uint8_t cmd;
  uint8_t crc;
  uint8_t data[0];
} ble_packet_t;
#pragma pack(pop)

#define U_CMD_PYTHON_CODE 1
#define U_CMD_INTERFACE_OUTPUT 40

typedef void (*ResultCallback)(JsonArray results);

class AISensor {
 public:
  AISensor(Stream* uart);

  void setModel(int model_type);
  void setModelParam(int probability, int probability2 = 65, int color_h = 10, int color_s = 40, int color_v = 0);
  void setZoom(float zoom_factor);

  bool exec(uint32_t timeout_ms = 2000);
  bool stop(uint32_t timeout_ms = 2000);

  void poll();
  void setCallback(ResultCallback cb);

  // Helpers
  static float getScore(JsonObject obj);
  static String getLabel(JsonObject obj);
  static int getBox(JsonObject obj, int coord);

public:
  enum ModelType {
    MODEL_DETECT = 1,
    MODEL_CLASSFY = 2,
    MODEL_PPOCR = 3,
    MODEL_FACEREC = 4,
    MODEL_TRACK = 5,
    MODEL_POSE = 6,
    MODEL_QRCODE = 7,
    MODEL_COLOR_DETECT = 8,
    MODEL_LINE_TRACK = 9,
    MODEL_VOICE_ASR = 10
};

enum BoxCoord {
    BOX_LEFT = 1,
    BOX_TOP = 2,
    BOX_RIGHT = 3,
    BOX_BOTTOM = 4
};

 private:
  Stream* _uart;
  int _model_type;
  int _probability;
  int _probability2;
  int _color_h;
  int _color_s;
  int _color_v;
  float _zoom;
  uint16_t _msg_id;
  ResultCallback _callback;

  uint8_t rx_buf[5120];
  size_t rx_len;

  String generateCode();
  bool send(uint8_t cmd, String payload);
  uint8_t calc_crc(const uint8_t* data, size_t len);
};

#endif