void initWebserver() {

  // Serve static files in the WEBUI_FOLDER
  server.serveStatic("/", SD, WEBUI_FOLDER).setDefaultFile("index.html");
  
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
    StaticJsonDocument<1000> songDoc;
    
    // return the list of the songs in the MUSIC_FOLDER
    File musicFolder = SD.open(MUSIC_FOLDER);
    if(musicFolder) {
      File musicFile = musicFolder.openNextFile();
      while(musicFile) {
        if(!musicFile.isDirectory()) {
          String songName(musicFile.name());
          songDoc.add(songName);
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

  // Start the webserver
  server.begin();
}