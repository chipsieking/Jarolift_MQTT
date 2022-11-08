void SetupOTA() {
  ArduinoOTA.setHostname(deviceNameShort);
  //ArduinoOTA.setPassword((const char *)"");
  //Default Port
  //ArduinoOTA.setPort(1337);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else { // U_SPIFFS
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
    char msgString[30];
    sprintf(msgString, "Progress: %u%%\r", (progress / (total / 100)));
    WriteLog("[OTA] - " + String(msgString), true);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    char msgString[20];
    sprintf(msgString, "Error[%u]: ", error);
    WriteLog("[OTA] - " + String(msgString), true);
    if (error == OTA_AUTH_ERROR) WriteLog("[OTA] -Auth Failed", true);
    else if (error == OTA_BEGIN_ERROR) WriteLog("[OTA] -Begin Failed", true);
    else if (error == OTA_CONNECT_ERROR) WriteLog("[OTA] -Connect Failed", true);
    else if (error == OTA_RECEIVE_ERROR) WriteLog("[OTA] -Receive Failed", true);
    else if (error == OTA_END_ERROR) WriteLog("[OTA] -End Failed", true);
  });
  ArduinoOTA.begin();
}

void handle_update(HTTPUpload& upload) {
  //HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    WriteLog("[OTA] - Upload: " + String(upload.filename.c_str()), true);
    if (!Update.begin(0XFFFFFFFF)) { //start with max available size
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    // flashing firmware to ESP
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { //true to set the size to the current progress
      //Kernel panic using upload.totalSize
      //PrintMessageLn("Update Success: " + upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void handle_update_finish() {
  uint16_t refreshdelay = 1000;
  WriteLog("[OTA] - Update finish", true);
  server.sendHeader("Connection", "close");
  String response = "<html><head><title>OTA</title><meta http-equiv='refresh' content='" + String(refreshdelay / 100) + "; URL=/' /><script>";
  response.concat("setInterval(function(){document.getElementById('countdown').innerHTML -= 1;}, 1000);");
  response.concat("</script></head><body>");
  response.concat(Update.hasError() ? "Update failed!" : "Update OK");
  response.concat(" - redirect in <label id='countdown'>" + String(refreshdelay / 100) + "</label> Sec.</body></html>");
  if (Update.hasError())
    server.send(500, "text/html", response);
  else
    server.send(200, "text/html", response);
  delay(refreshdelay);
  ESP.restart();
}

File fsUploadFile;
void handle_fileupload(HTTPUpload& upload) {
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
      filename = "/" + filename;

    WriteLog("[File] - Upload: handleFileUpload: " + filename, true);

    if (SPIFFS.exists(filename)) {
      WriteLog("[File] - File Deleted", true);
      SPIFFS.remove(filename);
    }

    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
      //Serial.print("handleFileUpload Size: ");
      //Serial.println(String(upload.totalSize));
      // Redirect the client to the root page
      server.sendHeader("Location", "/");
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void handle_upload_finish() {
  if (UploadIsOTA)
    handle_update_finish();
  else
    server.send(200);
}

void handle_upload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    UploadIsOTA = filename.endsWith(".bin");
  }
  if (UploadIsOTA)
    handle_update(upload);
  else
    handle_fileupload(upload);
}
void OTA_Handle() {
  ArduinoOTA.handle();
}
