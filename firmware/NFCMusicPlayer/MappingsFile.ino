// Read mappings from file
void readMappings() {

  File file = SD.open(MAPPINGS_FILE, FILE_READ);
  if(file) {
    while(file.available()) {
      String line = file.readStringUntil('\n');
      String loadedTagID = line.substring(0, line.indexOf('|'));
      String loadedSong = line.substring(line.indexOf('|') + 1);
      mapping loadedMapping;
      loadedMapping.tagid = loadedTagID;
      loadedMapping.song = loadedSong;
      mappings.add(loadedMapping);
    }
  }
}

// Save mappings to file
void saveMappings() {

  File file = SD.open(MAPPINGS_FILE, FILE_WRITE);
  if(file) {   
    for(int i = 0; i < mappings.getSize(); i++) {
      file.print(mappings[i].tagid);
      file.print('|');
      file.print(mappings[i].song);
      file.print('\n');
    }
    file.close();
  }
}