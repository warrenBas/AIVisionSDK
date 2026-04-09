#ifndef SENRAYVAR_VISION_H
#define SENRAYVAR_VISION_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <ArduinoJson.h>

#define VERSION       "1.0.0"


// =================================================================
// 🚀 Memory configuration area (automatically adapted based on board)
// =================================================================
#if defined(__AVR__) || defined(ARDUINO_ARCH_NRF51) || defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_MICROBIT)
  #define VISION_RX_BUF_SIZE   512   
  #define VISION_JSON_DOC_SIZE 256   
  #define VISION_EXEC_DOC_SIZE 256   
#else
  #define VISION_RX_BUF_SIZE   2048  
  #define VISION_JSON_DOC_SIZE 1024  
  #define VISION_EXEC_DOC_SIZE 2048  
#endif
// =================================================================

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

#define U_CMD_PYTHON_CODE 7
#define U_CMD_INTERFACE_OUTPUT 40

typedef void (*ResultCallback)(JsonArray results);

class SenRayVarVision {
public:
  enum ModelType {
    MODEL_DETECT = 1, MODEL_CLASSFY = 2, MODEL_PPOCR = 3,
    MODEL_FACEREC = 4, MODEL_TRACK = 5, MODEL_POSE = 6,
    MODEL_QRCODE = 7, MODEL_COLOR_DETECT = 8, MODEL_LINE_TRACK = 9,
    MODEL_VOICE_ASR = 10
  };

  enum BoxCoord { BOX_LEFT = 0, BOX_TOP = 1, BOX_RIGHT = 2, BOX_BOTTOM = 3 };
  enum PointCoord { POINT_X = 0, POINT_Y = 1 };

public:
  SenRayVarVision(Stream* uart);

  void setModel(int model_type);
  void setModelParam(int probability, int probability2 = 65, int color_h = 10, int color_s = 40, int color_v = 0);
  void setZoom(float zoom_factor);
  void setDetectTrack(int track);

  bool start(bool is_async = true, uint32_t timeout_ms = 2000);
  bool stop(bool is_async = true, uint32_t timeout_ms = 2000);
  bool save(bool is_async = true, uint32_t timeout_ms = 2000);

  void process(); 
  void setCallback(ResultCallback cb);
  bool hasNewData();

  bool awaitResult(uint32_t timeout_ms = 200);

  JsonArray getLatestResult();
  //ms
  uint32_t getLatestResultTs();
  // Get the number of recognized items
  int getItemSize();

  // Get from JsonObject
  static float getScore(JsonObject obj);
  static String getLabel(JsonObject obj);
  static int getBox(JsonObject obj, BoxCoord coord);
  static int getBoxCenter(JsonObject obj, PointCoord coord);
  static int getPoint(JsonObject obj, int point, PointCoord coord);

  // Get data by index in the recognition array
  String getLabelById(int id);
  float getScoreById(int id);
  int getBoxById(int id, BoxCoord coord); 
  int getBoxCenterById(int id, PointCoord coord); 
  int getPointById(int id, int point, PointCoord coord);

  // Get data by label
  bool hasLabel(const char* label);        
  float getScoreByLabel(const char* label);
  int getBoxByLabel(const char* label, BoxCoord coord); 
  int getBoxCenterByLabel(const char* label, PointCoord coord); 
  int getPointByLabel(const char* label, int point, PointCoord coord); 

  const char* getVersion() {return VERSION;}

private:
  Stream* _uart;
  int _model_type;
  int _probability;
  int _probability2;
  int _color_h;
  int _color_s;
  int _color_v;
  int _detect_track;
  float _zoom;
  uint16_t _msg_id;
  
  ResultCallback _callback;
  
  DynamicJsonDocument _doc;    
  JsonArray _latest_result;
  uint32_t _latest_result_ts_ms;
  bool _new_data_ready;        
  bool _exec_cmd_ack;      

  uint8_t rx_buf[VISION_RX_BUF_SIZE]; 
  size_t rx_len;

  String generateCode4Simple();
  String generateCode4Full();
  bool send(uint8_t cmd, String payload);
  uint8_t calc_crc(const uint8_t* data, size_t len);
};

#endif