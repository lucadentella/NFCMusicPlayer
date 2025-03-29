// Required libraries
#include <Audio.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <List.hpp>

// to disable brownout
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// PINs
#define SD_CS         13
#define PN532_CS      5
#define SPI_MOSI      23//15
#define SPI_MISO      19//2
#define SPI_SCK       18//14
#define I2S_DOUT      27//16
#define I2S_BCLK      26
#define I2S_LRC       25
#define PLAY_LED      2
#define STATUS_LED    4
#define VOLUME_PIN    34
#define BATTERY_PIN   35

// Configuration
#define NFC_READ_INTERVAL   2000
#define VOLUME_SAMPLES      200
#define MAX_SLOTS           20
#define MAPPINGS_FILE       "/mappings.txt"
#define MUSIC_FOLDER        "/music"
#define WEBUI_FOLDER        "/web/"
#define AP_SSID             "NFCMusicPlayer"
#define AP_PASSWORD         "MyMusicPlayer"


// Struct and array for mapping between tags and songs
typedef struct {
  String tagid;
  String song;
} mapping;
List<mapping> mappings;

// Global objects
SPIClass spi = SPIClass(VSPI);
Adafruit_PN532 nfc(PN532_CS, &spi);
Audio audio;
AsyncWebServer server(80);

// Global variables
TaskHandle_t nfcTaskHandler;
String tagID;
bool nfcPresent = false;
bool playing = false;
long lastReadTime = -1;
long volumeSamplesSum = 0;
int volumeSamples = 0;
int currentVolume = 10;