#include <ArduinoOTA.h>

const uint16_t      OTA_RESTART_DELAY = 1500; // [ms]
static const char*  OTA_DEVICE_NAME   = "JaroliftESP8266";

void OTA_setup() {
  ArduinoOTA.setHostname(OTA_DEVICE_NAME);
  //ArduinoOTA.setPassword((const char *)"");
  //Default Port
  //ArduinoOTA.setPort(1337);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      SPIFFS.end();
    }
    WriteLog("[OTA] - Start updating " + type, true);
  });

  ArduinoOTA.onEnd([]() {
    WriteLog("[OTA] - End", true);
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    WriteLog("[OTA] - Progress: " + String(progress / (total / 100)) + "%", true);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    WriteLog("[OTA] - Error: " + String(error), true);
  });

  ArduinoOTA.begin();
}

void OTA_startFwUpload() {
  WriteLog("[OTA] - start firmware upload", true);
  SPIFFS.end();
  if (!Update.begin(0XFFFFFFFF)) { //start with max available size
    Update.printError(Serial);
  }
}

void OTA_writeFw(uint8_t* data, size_t len) {
  if (Update.write(data, len) != len) { // flash firmware to ESP
    Update.printError(Serial);
  }
}

void OTA_endFwUpload() {
  if (!Update.end(true)) { //true to set the size to the current progress
    Update.printError(Serial);
  }
}

bool OTA_finishFwUpdate() {  
  SPIFFS.begin();
  bool updateOk = !Update.hasError();
  WriteLog("[OTA] - firmware upload " + updateOk ? "failed" : "finished", true);
  return updateOk;
}

void OTA_restart() {  
  delay(OTA_RESTART_DELAY);
  ESP.restart();  
}

void OTA_handle() {
  ArduinoOTA.handle();
}
