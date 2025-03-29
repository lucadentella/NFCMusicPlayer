#include "NFCMusicPlayer.h"

void setup() {

  Serial.begin(115200);
  Serial.println("NFCMusicPlayer starting...");
  Serial.println();

  // Brownout
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.println("- Brownout disabled");

  // PINs
  pinMode(STATUS_LED, OUTPUT);
  pinMode(PLAY_LED, OUTPUT);
  pinMode(VOLUME_PIN, INPUT);
  Serial.println("- PINs configured");

  // Check SD card, blink in case of error
  spi.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  if (!SD.begin(SD_CS, spi, 80000000)) {
    Serial.println("! Unable to mount SD card");
    while(1) {
      digitalWrite(STATUS_LED, HIGH);
      delay(1000);
      digitalWrite(STATUS_LED, LOW);
      delay(1000);     
    }
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("- SD card OK, size: %lluMB\n", cardSize);

  // Load mappings from SD card
  readMappings();
  Serial.println("- Mappings loaded from SD card");

  // Check PN532 board, blink faster in case of error
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if(!versiondata) {
    Serial.println("! Unable to find PN532 board");
    while(1) {
      digitalWrite(STATUS_LED, HIGH);
      delay(200);
      digitalWrite(STATUS_LED, LOW);
      delay(200);     
    }
  }
  Serial.printf("- PN532 board OK, firmware: %d\n", (versiondata>>16) & 0xFF);

  // Run WiFi in AccessPoint MODE
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("- AccessPoint running: ");
  Serial.println(IP);

  // Configure and start webserver
  initWebserver();
  Serial.println("- Webserver running");

  // Configure Audio library
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolumeSteps(20);
  audio.setVolume(currentVolume);
  audio.setTone(10, 0, 0);
  Serial.println("- Audio library OK");

  // Start NFC task
  xTaskCreate(nfcTaskCode, "nfcTask", 10000, NULL, tskIDLE_PRIORITY, &nfcTaskHandler);
  Serial.println("- NFC task created");

  // Ready to play!
  digitalWrite(STATUS_LED, HIGH);
  Serial.println();
  Serial.println("...NFCMusicPlayer READY :)");
}

void loop() {

  // Audio library required loop
  audio.loop();

  // Check volume (average value every VOLUME_SAMPLES samples)
  int potValue = analogRead(VOLUME_PIN);
  volumeSamplesSum += potValue;
  volumeSamples++;
  if(volumeSamples == VOLUME_SAMPLES) {
    int newVolume = map(volumeSamplesSum / VOLUME_SAMPLES, 0, 4095, 0, 20);
    if(newVolume != currentVolume) {
      currentVolume = newVolume;
      audio.setVolume(currentVolume);
      Serial.printf("Volume changed to: %d\n", currentVolume);
    }
    volumeSamplesSum = 0;
    volumeSamples = 0;
  }
}

// NFC Task: check if a NFC tag is present
void nfcTaskCode(void * pvParameters) {

  for(;;) {
    long currentTime = millis();
    if(currentTime - lastReadTime > NFC_READ_INTERVAL) {
    
      uint8_t success;
      uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
      uint8_t uidLength;
      success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1500);
      
      if (success) {
        
        nfcPresent = true;
        // Save the nfcUID in the form AA:BB:CC:DD...
        tagID = "";
        for(int i = 0; i < uidLength; i++) {
          if(uid[i] <= 0xF) tagID += "0";
          tagID += String(uid[i] & 0xFF, HEX);
          if(i < (uidLength - 1)) tagID += ":";
          
        }
        Serial.printf("Tag found, %S\n", tagID.c_str());       

        if(!playing) {
          
          // Find in the mappings the song to be played
          for(int i = 0; i < mappings.getSize(); i++) {
            if(tagID.equals(mappings[i].tagid)) {
              char songPath[255];
              sprintf(songPath, "%s/%s", MUSIC_FOLDER,mappings[i].song.c_str());
              Serial.printf("Tag found, playing %s\n", songPath);
              audio.connecttoFS(SD, songPath);
              playing = true;
              digitalWrite(PLAY_LED, HIGH);
              break;
            }
          }
        }
      }
      
      else {
        
        nfcPresent = false;
        if(playing) {
          Serial.println("Tag not found, stop playing");
          audio.stopSong();
          playing = false;
          digitalWrite(PLAY_LED, LOW);
        }
      }
      lastReadTime = currentTime;
    }
  }
}

// Callback at the end of the song
void audio_eof_mp3(const char *info){  //end of file
    Serial.println("End of song");
    playing = false;
    digitalWrite(PLAY_LED, LOW);
}
