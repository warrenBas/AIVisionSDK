#include "SenRayVarVision.h"



SenRayVarVision senrayvar(&Serial1);

void onResult(JsonArray results) {
  for (JsonObject obj : results) {
    serializeJson(obj, Serial);
    Serial.println();
    Serial.print("Label: ");
    Serial.print(SenRayVarVision::getLabel(obj));
    Serial.print(", Score: ");
    Serial.println(SenRayVarVision::getScore(obj));
    Serial.print("Box Left: ");
    Serial.println(SenRayVarVision::getBox(obj, SenRayVarVision::BOX_LEFT));
    Serial.print("point: ");
    Serial.println(SenRayVarVision::getPoint(obj, 0, SenRayVarVision::POINT_X));
  }
}

void setup() {
  Serial.begin(115200);
  //rxPin=4, txPin=5
  Serial1.begin(115200, 12, 13);

  senrayvar.setCallback(onResult);
  
  senrayvar.setModel(SenRayVarVision::MODEL_DETECT);
  senrayvar.setModelParam(65, 0, 10, 33, 33); 
  senrayvar.setZoom(1.1);
  senrayvar.setDetectTrack(1);

  Serial.println("Starting AI Sensor...");
  
  if(senrayvar.start(false, 3000)) {
    Serial.println("Sensor started successfully!");
  } else {
    Serial.println("Sensor start timeout/failed.");
  }
}


void loop() {
  senrayvar.process();
  delay(10);
}



// void setup() {
//   Serial.begin(115200);
//   //rxPin=4, txPin=5
//   Serial1.begin(115200, SERIAL_8N1, 4, 5);
  
//   senrayvar.setModel(SenRayVarVision::MODEL_DETECT);
//   senrayvar.setModelParam(65); 
//   senrayvar.setZoom(1.0);

//   Serial.println("Starting AI Sensor...");
  
//   if(senrayvar.start(false)) {
//     Serial.println("Sensor started successfully!");
//   } else {
//     Serial.println("Sensor start timeout/failed.");
//   }
// }

// int cnt=0;
// void loop() {
//   senrayvar.process(); 
//   delay(10);
//   if (senrayvar.hasNewData()) {
//     Serial.print("getItemSize: ");
//     Serial.print(senrayvar.getItemSize());
//     Serial.print("label: ");
//     Serial.print(senrayvar.getLabelById(0));
//     Serial.print(", Score: ");
//     Serial.println(senrayvar.getScoreById(0));
//     Serial.print("Box Left: ");
//     Serial.println(senrayvar.getBoxById(0, SenRayVarVision::BOX_LEFT));
//     Serial.print("point: ");
//     Serial.println(senrayvar.getBoxCenterById(0, SenRayVarVision::POINT_X));
//   }
    // if (cnt++ > 1000) {
    //   senrayvar.setModel(SenRayVarVision::MODEL_FACEREC);
    //   senrayvar.start(false);
    // }

// }


