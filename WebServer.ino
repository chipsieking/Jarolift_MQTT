#include "html_api.h"

ESP8266WebServer server;

static bool uploadOTA = false;

void WebServer_init() {
  // configure webserver and start it
  server.on("/api", html_api);                       // command api
  server.on("/upload", HTTP_POST, handleUploadFinish, handleUpload);
  SPIFFS.begin();                                       // Start the SPI flash filesystem

  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri())) {                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
      Serial.println(" File not found: did you upload the data directory?");
    }
  });

  server.begin();
  WriteLog("[HTTP] - server started", true);
}

void WebServer_handle() {
  server.handleClient();
}

void WebServer_send(int code, const char* msgType, const char* msg) {
  server.send(code, msgType, msg);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Webserver functions group
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// void html_api() -> see html_api.h

//####################################################################
// convert the file extension to the MIME type
//####################################################################
String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

//####################################################################
// send the right file to the client (if it exists)
//####################################################################
bool handleFileRead(String path) {
  if (debug_webui) {
    Serial.println("handleFileRead '" + path + "'");
  }
  if (path.endsWith("/")) {
    path += "index.html";         // If a folder is requested, send the index file
  }
  if (SPIFFS.exists(path)) {                            // If the file exists
    String contentType = getContentType(path);          // Get the MIME type
    File file = SPIFFS.open(path, "r");                 // Open it
    server.streamFile(file, contentType);               // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  if (debug_webui) {
    Serial.println("\tFile Not Found");
  }
  return false;
}

void handleUpload() {
  static File fsUploadFile;
  HTTPUpload& upload = server.upload();

  switch (upload.status) {                                    // handle all states explicitely
    case UPLOAD_FILE_START:
      uploadOTA = upload.filename.endsWith(".bin");

      if (uploadOTA) {
        OTA_startFwUpload();
      } else {
        String filename = upload.filename;
        if (!filename.startsWith("/")) {
          filename = "/" + filename;
        }
        if (SPIFFS.exists(filename)) {
          SPIFFS.remove(filename);
          WriteLog("[HTTP] - existing file '" + filename + "' deleted", true);
        }
        WriteLog("[HTTP] - upload '" + filename + "'", true);
        fsUploadFile = SPIFFS.open(filename, "w");              // ignore file system errors
      }
    break;

    case UPLOAD_FILE_WRITE:
      if (uploadOTA) {
        OTA_writeFw(upload.buf, upload.currentSize);
      } else {
        if (fsUploadFile) {
          fsUploadFile.write(upload.buf, upload.currentSize);   // ignore write errors
        }
      }
    break;

    case UPLOAD_FILE_END:
      if (uploadOTA) {
        OTA_endFwUpload();
      } else {
        if (fsUploadFile) {
          fsUploadFile.close();
          server.sendHeader("Location", "/");                   // Redirect the client to the root page
          server.send(303);
        } else {
          server.send(500, "text/plain", "500: couldn't create file");
        }
      }
    break;

    case UPLOAD_FILE_ABORTED:
      Serial.printf("upload file aborted\n");
      if (uploadOTA) {
        // TODO
      } else {
        if (fsUploadFile) {
          fsUploadFile.close();
        }
      }
    break;
  }
}

void handleUploadFinish() {
  if (uploadOTA) {
    bool updateOk = OTA_finishFwUpdate();

    server.sendHeader("Connection", "close");
    String response = "<html><head><title>OTA</title><meta http-equiv='refresh' content='" + String(OTA_RESTART_DELAY / 100) + "; URL=/' /><script>";
    response.concat("setInterval(function(){document.getElementById('countdown').innerHTML -= 1;}, 1000);");
    response.concat("</script></head><body>");
    response.concat(updateOk ? "Update failed!" : "Update OK");
    response.concat(" - redirect in <label id='countdown'>" + String(OTA_RESTART_DELAY / 100) + "</label> Sec.</body></html>");
    server.send(updateOk ? 500 : 200, "text/html", response);

    OTA_restart();
  } else {
    server.send(200);
  }
  uploadOTA = false;
}
