// Required libraries
#include <Audio.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
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
#define POWER_PIN     22  // Controls external power circuit (changed from 21 to 22 for testing)

// Navigation buttons
#define FORWARD_BTN   16
#define BACKWARD_BTN  17

// Configuration
#define NFC_READ_INTERVAL   2000
#define VOLUME_SMOOTHING    0.1f    // EMA smoothing factor (0.05-0.2 recommended)
#define VOLUME_UPDATE_MS    50      // Volume update interval in ms
#define MAX_SLOTS           20
#define MAPPINGS_FILE       "/mappings.txt"
#define MUSIC_FOLDER        "/music"
#define WEBUI_FOLDER        "/web/"
#define AP_SSID             "NFCMusicPlayer"
#define AP_PASSWORD         "MyMusicPlayer"
#define SHUTDOWN_TIMEOUT_MS  120000  // 2 minutes auto-shutdown


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
float smoothedVolume = 2048.0f;       // EMA value (start at midpoint)
unsigned long lastVolumeUpdate = 0;
int currentVolume = 10;
volatile unsigned long lastActivityTime = 0;  // Tracks last activity for auto-shutdown

// Button debouncing variables
unsigned long lastForwardDebounce = 0;
unsigned long lastBackwardDebounce = 0;
unsigned long lastForwardTrigger = 0;
unsigned long lastBackwardTrigger = 0;
bool lastForwardState = HIGH;
bool lastBackwardState = HIGH;

// EQ settings (range: -40 to +6 dB)
int eqBass = 0;
int eqMid = 0;
int eqTreble = 0;

// Button configuration constants
#define DEBOUNCE_DELAY    50   // milliseconds
#define REPEAT_DELAY      300  // milliseconds minimum between triggers
#define FORWARD_SEEK_SEC  30   // seconds to skip forward
#define BACKWARD_SEEK_SEC -10  // seconds to skip backward (negative)

// EQ settings file
#define SETTINGS_FILE     "/settings.json"