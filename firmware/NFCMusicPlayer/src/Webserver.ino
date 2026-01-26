void initWebserver() {

  // Serve static files in the WEBUI_FOLDER
  server.serveStatic("/", SD, WEBUI_FOLDER).setDefaultFile("index.html");

  // Global middleware: track HTTP activity for auto-shutdown
  server.addMiddleware([](AsyncWebServerRequest *request, ArMiddlewareNext next) {
    lastActivityTime = millis();  // Any HTTP request = activity
    next();  // Continue to handler
  });

  // Serve /uid endpoint
  server.on("/tagid", HTTP_GET, [](AsyncWebServerRequest *request) {

    // Prepare JSON response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    StaticJsonDocument<100> doc;

    // return the current tagID (or empty if no tag is present)
    if(nfcPresent) doc["tagid"] = tagID;
    else doc["tagid"] = "";
    serializeJson(doc, *response);
    request->send(response);
  });

  // Serve /songs endpoint
  server.on("/songs", HTTP_GET, [](AsyncWebServerRequest *request) {

    // Prepare JSON response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    StaticJsonDocument<2000> songDoc;

    // return the list of the songs in the MUSIC_FOLDER with metadata
    File musicFolder = SD.open(MUSIC_FOLDER);
    if(musicFolder) {
      File musicFile = musicFolder.openNextFile();
      while(musicFile) {
        if(!musicFile.isDirectory()) {
          JsonObject fileObj = songDoc.createNestedObject();
          fileObj["name"] = String(musicFile.name());
          fileObj["size"] = musicFile.size();

          // Add file modification timestamp
          time_t lastWrite = musicFile.getLastWrite();
          fileObj["timestamp"] = (uint32_t)lastWrite;

          // Check if file is mapped
          bool isMapped = false;
          for(int i = 0; i < mappings.getSize(); i++) {
            if (String(musicFile.name()).equals(mappings[i].song)) {
              isMapped = true;
              break;
            }
          }
          fileObj["mapped"] = isMapped;
        }
        musicFile = musicFolder.openNextFile();
      }
    }
    serializeJson(songDoc, *response);
    request->send(response);
  });

    // Serve /mappings endpoint
  server.on("/mappings", HTTP_GET, [](AsyncWebServerRequest *request) {

    // Prepare JSON response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    StaticJsonDocument<1000> mappingDoc;
    
    for(int i = 0; i < mappings.getSize(); i++) {
      JsonObject mappingObject = mappingDoc.createNestedObject();
      mappingObject["tagid"] = mappings[i].tagid;
      mappingObject["song"] = mappings[i].song;
    }
    serializeJson(mappingDoc, *response);
    request->send(response);
  });

  // Serve /addmapping endpoint
  AsyncCallbackJsonWebHandler* addmappingHandler = new AsyncCallbackJsonWebHandler("/addmapping", [](AsyncWebServerRequest *request, JsonVariant &json) {

    // Prepare JSON response
    AsyncResponseStream *response = request->beginResponseStream("application/json");    
    StaticJsonDocument<100> doc; 

    // Get the new mapping
    JsonObject jsonObj = json.as<JsonObject>();
    String newTagID = jsonObj["tagid"];
    String newSong = jsonObj["song"];

    // Add new mapping if we still have space
    doc["result"] = "NOSPACE";
    if(mappings.getSize() < MAX_SLOTS) {
      mapping newMapping;
      newMapping.tagid = newTagID;
      newMapping.song = newSong;
      mappings.add(newMapping);
      saveMappings();
      doc["result"] = "OK";  
    }

    // Send response
    serializeJson(doc, *response);
    request->send(response);
  });
  server.addHandler(addmappingHandler);

  // Serve /delmapping endpoint
  AsyncCallbackJsonWebHandler* delmappingHandler = new AsyncCallbackJsonWebHandler("/delmapping", [](AsyncWebServerRequest *request, JsonVariant &json) {
    
    // Prepare JSON response
    AsyncResponseStream *response = request->beginResponseStream("application/json");    
    StaticJsonDocument<100> doc;  
    
    // Get TagID
    JsonObject jsonObj = json.as<JsonObject>();
    String delTagID = jsonObj["tagid"];

    // Check if TagID is present in the mappings list, if so remove the mapping
    doc["result"] = "NOTFOUND";
    for(int i = 0; i < mappings.getSize(); i++) {
      if(delTagID.equals(mappings[i].tagid)) {
        mappings.remove(i);
        saveMappings();
        doc["result"] = "OK";
      }
    }

    // Send response
    serializeJson(doc, *response);
    request->send(response);
  });
  server.addHandler(delmappingHandler);

  // Serve /upload endpoint
  server.on(
    "/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      // Called when upload completes
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      StaticJsonDocument<200> doc;

      // If we reach this handler, upload succeeded
      // (errors during chunking already sent response and returned)
      doc["result"] = "OK";
      doc["message"] = "File uploaded successfully";

      serializeJson(doc, *response);
      request->send(response);
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      // Called for each chunk
      if (!index) {
        // First chunk - validate and open file
        if (!filename.endsWith(".mp3") && !filename.endsWith(".MP3")) {
          request->send(400, "text/plain", "Only MP3 files allowed");
          return;
        }

        String filepath = String(MUSIC_FOLDER) + "/" + filename;
        request->_tempFile = SD.open(filepath.c_str(), FILE_WRITE);

        if (!request->_tempFile) {
          request->send(500, "text/plain", "Failed to open file for writing");
          return;
        }
      }

      if (len) {
        request->_tempFile.write(data, len);
      }

      if (final) {
        request->_tempFile.close();
      }
    }
  );

  // Serve /download endpoint
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Missing file parameter");
      return;
    }

    String filename = request->getParam("file")->value();

    // Security: prevent directory traversal
    if (filename.indexOf("..") >= 0 || filename.indexOf("/") >= 0) {
      request->send(403, "text/plain", "Invalid filename");
      return;
    }

    String filepath = String(MUSIC_FOLDER) + "/" + filename;

    if (!SD.exists(filepath.c_str())) {
      request->send(404, "text/plain", "File not found");
      return;
    }

    AsyncWebServerResponse *response = request->beginResponse(SD, filepath, "audio/mpeg");
    response->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    request->send(response);
  });

  // Serve /deletefile endpoint
  AsyncCallbackJsonWebHandler* deleteFileHandler = new AsyncCallbackJsonWebHandler(
    "/deletefile",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      StaticJsonDocument<200> doc;

      JsonObject jsonObj = json.as<JsonObject>();
      String filename = jsonObj["filename"];

      // Security validation
      if (filename.indexOf("..") >= 0 || filename.indexOf("/") >= 0) {
        doc["result"] = "ERROR";
        doc["message"] = "Invalid filename";
        serializeJson(doc, *response);
        request->send(response);
        return;
      }

      String filepath = String(MUSIC_FOLDER) + "/" + filename;

      if (!SD.exists(filepath.c_str())) {
        doc["result"] = "NOTFOUND";
        doc["message"] = "File does not exist";
        serializeJson(doc, *response);
        request->send(response);
        return;
      }

      // Check if file is referenced in mappings
      bool fileInUse = false;
      for(int i = 0; i < mappings.getSize(); i++) {
        if (mappings[i].song.equals(filename)) {
          fileInUse = true;
          break;
        }
      }

      if (fileInUse) {
        doc["result"] = "INUSE";
        doc["message"] = "Cannot delete: file is mapped to NFC tag(s)";
        serializeJson(doc, *response);
        request->send(response);
        return;
      }

      // Delete the file
      if (SD.remove(filepath.c_str())) {
        doc["result"] = "OK";
        doc["message"] = "File deleted successfully";
      } else {
        doc["result"] = "ERROR";
        doc["message"] = "Failed to delete file";
      }

      serializeJson(doc, *response);
      request->send(response);
    }
  );
  server.addHandler(deleteFileHandler);

  // Serve /renamefile endpoint
  AsyncCallbackJsonWebHandler* renameFileHandler = new AsyncCallbackJsonWebHandler(
    "/renamefile",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      StaticJsonDocument<200> doc;

      JsonObject jsonObj = json.as<JsonObject>();
      String oldFilename = jsonObj["oldname"];
      String newFilename = jsonObj["newname"];

      // Security validation
      if (oldFilename.indexOf("..") >= 0 || oldFilename.indexOf("/") >= 0 ||
          newFilename.indexOf("..") >= 0 || newFilename.indexOf("/") >= 0) {
        doc["result"] = "ERROR";
        doc["message"] = "Invalid filename";
        serializeJson(doc, *response);
        request->send(response);
        return;
      }

      // Validate .mp3 extension
      if (!newFilename.endsWith(".mp3") && !newFilename.endsWith(".MP3")) {
        doc["result"] = "ERROR";
        doc["message"] = "New filename must have .mp3 extension";
        serializeJson(doc, *response);
        request->send(response);
        return;
      }

      String oldPath = String(MUSIC_FOLDER) + "/" + oldFilename;
      String newPath = String(MUSIC_FOLDER) + "/" + newFilename;

      if (!SD.exists(oldPath.c_str())) {
        doc["result"] = "NOTFOUND";
        doc["message"] = "Source file does not exist";
        serializeJson(doc, *response);
        request->send(response);
        return;
      }

      if (SD.exists(newPath.c_str())) {
        doc["result"] = "EXISTS";
        doc["message"] = "A file with the new name already exists";
        serializeJson(doc, *response);
        request->send(response);
        return;
      }

      // Rename file
      if (SD.rename(oldPath.c_str(), newPath.c_str())) {
        // CRITICAL: Update all mappings
        bool mappingsUpdated = false;
        for(int i = 0; i < mappings.getSize(); i++) {
          if (mappings[i].song.equals(oldFilename)) {
            mappings[i].song = newFilename;
            mappingsUpdated = true;
          }
        }

        if (mappingsUpdated) {
          saveMappings();
        }

        doc["result"] = "OK";
        doc["message"] = "File renamed successfully";
        doc["mappingsUpdated"] = mappingsUpdated;
      } else {
        doc["result"] = "ERROR";
        doc["message"] = "Failed to rename file";
      }

      serializeJson(doc, *response);
      request->send(response);
    }
  );
  server.addHandler(renameFileHandler);

  // Serve /settings endpoint (GET - retrieve EQ settings)
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    StaticJsonDocument<100> doc;
    doc["bass"] = eqBass;
    doc["mid"] = eqMid;
    doc["treble"] = eqTreble;
    serializeJson(doc, *response);
    request->send(response);
  });

  // Serve /settings endpoint (POST - update EQ settings)
  AsyncCallbackJsonWebHandler* settingsHandler = new AsyncCallbackJsonWebHandler("/settings",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();

      eqBass = constrain((int)obj["bass"], -40, 6);
      eqMid = constrain((int)obj["mid"], -40, 6);
      eqTreble = constrain((int)obj["treble"], -40, 6);

      audio.setTone(eqBass, eqMid, eqTreble);
      saveSettings();

      AsyncResponseStream *response = request->beginResponseStream("application/json");
      StaticJsonDocument<100> doc;
      doc["result"] = "OK";
      serializeJson(doc, *response);
      request->send(response);
    });
  server.addHandler(settingsHandler);

  // Start the webserver
  server.begin();
}