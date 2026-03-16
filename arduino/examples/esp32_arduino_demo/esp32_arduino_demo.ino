#include "AISensor.h"

AISensor sensor(&Serial1);

void onResult(JsonArray results) {
  for (JsonObject obj : results) {
    serializeJson(obj, Serial);
    Serial.println();
    Serial.print("Label: ");
    Serial.print(AISensor::getLabel(obj));
    Serial.print(", Score: ");
    Serial.println(AISensor::getScore(obj));
    Serial.print("Box Left: ");
    Serial.println(AISensor::getBox(obj, AISensor::BOX_LEFT));
  }
}

void setup() {
  Serial.begin(115200);
  //rxPin=16, txPin=17
  Serial1.begin(115200, SERIAL_8N1, 16, 17);

  sensor.setCallback(onResult);
  
  sensor.setModel(AISensor::MODEL_DETECT);
  sensor.setModelParam(65); 
  sensor.setZoom(1.0);

  Serial.println("Starting AI Sensor...");
  
  if(sensor.exec(2000)) {
    Serial.println("Sensor started successfully!");
  } else {
    Serial.println("Sensor start timeout/failed.");
  }
}

void loop() {
  sensor.poll(); 
}