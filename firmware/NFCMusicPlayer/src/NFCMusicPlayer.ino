#include "NFCMusicPlayer.h"

void setup() {

  Serial.begin(115200);
  Serial.println("NFCMusicPlayer starting...");
  Serial.println();

  // Keep brownout enabled to detect power issues
  Serial.println("- Brownout detector active (protects ESP32)");

  // PINs
  pinMode(STATUS_LED, OUTPUT);
  pinMode(PLAY_LED, OUTPUT);
  pinMode(VOLUME_PIN, INPUT);
  pinMode(FORWARD_BTN, INPUT_PULLUP);
  pinMode(BACKWARD_BTN, INPUT_PULLUP);

  // Initialize power control
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);  // Power ON (HIGH)
  lastActivityTime = millis();    // Initialize activity timer
  Serial.println("- PINs configured (including navigation buttons on GPIO16/17)");
  Serial.println("- Power control initialized (GPIO22 = HIGH)");

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
  Serial.printf("- PN532 board OK, firmware: v%d.%d\n", (versiondata>>16) & 0xFF, (versiondata>>8) & 0xFF);

  // Configure PN532 to read RFID tags
  nfc.SAMConfig();

  // Optimize RF power for battery operation
  uint8_t rfConfig[] = {0x32, 0x01, 0x00, 0x0B, 0x0A};
  nfc.sendCommandCheckAck(rfConfig, sizeof(rfConfig));
  delay(50);

  nfc.setPassiveActivationRetries(0x05);

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
  // audio.setVolumeSteps(20); // Method not available in ESP32-audioI2S 2.0.6+
  audio.setVolume(currentVolume);

  // Load and apply EQ settings
  loadSettings();
  audio.setTone(eqBass, eqMid, eqTreble);
  Serial.printf("- Audio library OK (EQ: Bass=%d, Mid=%d, Treble=%d)\n", eqBass, eqMid, eqTreble);

  // Start NFC task
  xTaskCreate(nfcTaskCode, "nfcTask", 10000, NULL, tskIDLE_PRIORITY, &nfcTaskHandler);
  Serial.println("- NFC task created");

  // Ready to play!
  digitalWrite(STATUS_LED, HIGH);
  Serial.println();
  Serial.println("...NFCMusicPlayer READY :)");
}

void loop() {

  // Auto-shutdown check
  unsigned long currentTime = millis();
  if (playing) {
    lastActivityTime = currentTime;  // Music playing = activity
  }

  unsigned long elapsed = currentTime - lastActivityTime;

  // Trigger shutdown if timeout exceeded and not playing
  if (elapsed >= SHUTDOWN_TIMEOUT_MS && !playing) {
    digitalWrite(POWER_PIN, LOW);   // Power OFF (LOW)
    Serial.println("Auto-shutdown: 2 minutes of inactivity (GPIO22 = LOW)");
    // External circuit will cut power shortly
  }

  // Audio library required loop
  audio.loop();

  // EMA filter for fast but smoothed volume control
  int potValue = analogRead(VOLUME_PIN);
  smoothedVolume = (VOLUME_SMOOTHING * potValue) + ((1.0f - VOLUME_SMOOTHING) * smoothedVolume);

  if(currentTime - lastVolumeUpdate >= VOLUME_UPDATE_MS) {
    lastVolumeUpdate = currentTime;
    // Logarithmic volume curve for natural perception (exponent 2.5)
    float normalized = smoothedVolume / 4095.0f;
    int newVolume = constrain((int)(20.0f * pow(normalized, 2.5f) + 0.5f), 0, 20);
    if(newVolume != currentVolume) {
      currentVolume = newVolume;
      audio.setVolume(currentVolume);
    }
  }

  // Check navigation buttons (only active while playing)
  checkNavigationButtons();
}

// NFC Task: check if a NFC tag is present
void nfcTaskCode(void * pvParameters) {

  for(;;) {
    long currentTime = millis();
    if(currentTime - lastReadTime > NFC_READ_INTERVAL) {

      uint8_t success;
      uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
      uint8_t uidLength;
      success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000);

      if (success) {

        nfcPresent = true;
        // Save the nfcUID in the form AA:BB:CC:DD...
        tagID = "";
        for(int i = 0; i < uidLength; i++) {
          if(uid[i] <= 0xF) tagID += "0";
          tagID += String(uid[i] & 0xFF, HEX);
          if(i < (uidLength - 1)) tagID += ":";

        }
        Serial.printf("Tag found: %s\n", tagID.c_str());

        if(!playing) {

          // Find in the mappings the song to be played
          for(int i = 0; i < mappings.getSize(); i++) {
            if(tagID.equals(mappings[i].tagid)) {
              char songPath[255];
              sprintf(songPath, "%s/%s", MUSIC_FOLDER,mappings[i].song.c_str());
              Serial.printf("Playing: %s\n", songPath);
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
          Serial.println("Tag removed, stop playing");
          audio.stopSong();
          playing = false;
          digitalWrite(PLAY_LED, LOW);
        }
      }
      lastReadTime = currentTime;
    }

    // Small delay to prevent task from hogging CPU
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// Callback at the end of the song
void audio_eof_mp3(const char *info){  //end of file
    Serial.println("End of song");
    playing = false;
    digitalWrite(PLAY_LED, LOW);
}

// Load EQ settings from SD card
void loadSettings() {
  if (!SD.exists(SETTINGS_FILE)) return;

  File file = SD.open(SETTINGS_FILE, FILE_READ);
  if (!file) return;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  if (!error) {
    eqBass = doc["bass"] | 0;
    eqMid = doc["mid"] | 0;
    eqTreble = doc["treble"] | 0;
  }
  file.close();
}

// Save EQ settings to SD card
void saveSettings() {
  File file = SD.open(SETTINGS_FILE, FILE_WRITE);
  if (!file) return;

  JsonDocument doc;
  doc["bass"] = eqBass;
  doc["mid"] = eqMid;
  doc["treble"] = eqTreble;
  serializeJson(doc, file);
  file.close();
}

// Check navigation buttons with debouncing
void checkNavigationButtons() {
  unsigned long currentTime = millis();

  // Read current button states
  bool forwardReading = digitalRead(FORWARD_BTN);
  bool backwardReading = digitalRead(BACKWARD_BTN);

  // --- FORWARD BUTTON ---
  if (forwardReading != lastForwardState) {
    lastForwardDebounce = currentTime;
  }

  if ((currentTime - lastForwardDebounce) > DEBOUNCE_DELAY) {
    if (forwardReading == LOW && playing &&
        (currentTime - lastForwardTrigger) > REPEAT_DELAY) {

      Serial.println("Forward button pressed - seeking +30 seconds");
      audio.setTimeOffset(FORWARD_SEEK_SEC);
      lastForwardTrigger = currentTime;
      lastActivityTime = currentTime;  // Reset auto-shutdown timer
    }
  }
  lastForwardState = forwardReading;

  // --- BACKWARD BUTTON ---
  if (backwardReading != lastBackwardState) {
    lastBackwardDebounce = currentTime;
  }

  if ((currentTime - lastBackwardDebounce) > DEBOUNCE_DELAY) {
    if (backwardReading == LOW && playing &&
        (currentTime - lastBackwardTrigger) > REPEAT_DELAY) {

      Serial.println("Backward button pressed - seeking -10 seconds");
      audio.setTimeOffset(BACKWARD_SEEK_SEC);
      lastBackwardTrigger = currentTime;
      lastActivityTime = currentTime;  // Reset auto-shutdown timer
    }
  }
  lastBackwardState = backwardReading;
}
