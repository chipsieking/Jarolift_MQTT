void InitWebServer()
{
  // configure webserver and start it
  server.on ( "/api", html_api );                       // command api
  server.on("/upload", HTTP_POST, handle_upload_finish, handle_upload);
  SPIFFS.begin();                                       // Start the SPI flash filesystem
  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri())) {                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
      Serial.println(" File not found: did you upload the data directory?");
    }
  });

  server.begin();
  WriteLog("[INFO] - HTTP server started", true);
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
} // String getContentType

//####################################################################
// send the right file to the client (if it exists)
//####################################################################
bool handleFileRead(String path) {
  if (debug_webui) Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    server.streamFile(file, contentType);               // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  if (debug_webui) Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
} // bool handleFileRead
