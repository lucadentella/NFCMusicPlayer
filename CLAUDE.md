# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

NFCMusicPlayer is an Arduino-based portable MP3 player that uses NFC tags to control playback. When an NFC tag is placed on the device, it plays the associated song from the SD card. The project includes custom PCB hardware, 3D-printed enclosure, ESP32 firmware, and a web-based configuration interface.

## Hardware Platform

- **Microcontroller**: ESP32 WROOM-32E Module (4MB)
- **NFC Reader**: PN532 module (SPI interface)
- **Audio**: MAX98357A I2S Class D amplifier
- **Storage**: SD card (SPI interface)
- **Power**: 18650 battery with TP4056 USB-C charger

## Firmware Architecture

The firmware is split across three Arduino (.ino) files that work together:

### NFCMusicPlayer.ino (main)
- **setup()**: Initializes all hardware components in sequence (SD card, PN532, WiFi AP, webserver, audio library)
- **loop()**: Continuously runs audio processing and monitors volume potentiometer
- **nfcTaskCode()**: Runs as FreeRTOS task, polls for NFC tags every 2 seconds, starts/stops playback based on tag presence
- **Error signaling**: Uses STATUS_LED to indicate errors (1s blink = SD card failure, 200ms blink = PN532 failure)

### MappingsFile.ino
Handles persistent storage of NFC tag-to-song mappings:
- **readMappings()**: Loads mappings from `/mappings.txt` on SD card at startup
- **saveMappings()**: Writes current mappings back to SD card
- **Format**: Plain text, one mapping per line: `AA:BB:CC:DD|songname.mp3`

### Webserver.ino
Provides REST API and web UI for configuration:
- Serves static web UI files from `/web/` folder on SD card

**REST API Endpoints:**
- **GET /tagid**: Returns currently detected NFC tag UID
- **GET /songs**: Lists all MP3 files in `/music/` folder with metadata (name, size, timestamp, mapped status)
- **GET /mappings**: Returns all configured tag-to-song mappings
- **GET /download?file=**: Downloads/streams an MP3 file
- **POST /addmapping**: Adds new mapping (max 20 slots)
- **POST /delmapping**: Removes mapping by tag ID
- **POST /upload**: Handles chunked file upload for MP3 files
- **POST /deletefile**: Deletes an MP3 file (checks if mapped first)
- **POST /renamefile**: Renames an MP3 file (updates mappings automatically)

### NFCMusicPlayer.h
Central header file containing:
- All `#include` statements for required libraries
- Pin definitions for SPI buses, I2S audio, LEDs, and analog inputs
- Configuration constants (intervals, file paths, WiFi credentials)
- Global objects (SPI, NFC, Audio, WebServer)
- Global variables (playback state, tag ID, volume control)
- `mapping` struct definition and `List<mapping>` for tag storage

## File System Structure (SD Card)

The SD card must contain:
- `/mappings.txt` - NFC tag to song mappings (pipe-delimited: `AA:BB:CC:DD|songname.mp3`)
- `/music/` - Directory containing MP3 files
- `/web/` - Web UI files:
  - `index.html` - Main web interface (Tailwind CSS dark theme)
  - `index.js` - JavaScript application logic
  - `tailwind.css` - Tailwind CSS utilities (offline-capable, ~12KB)
  - `index.css` - Minimal custom CSS (scrollbar, smooth scrolling)
  - `jquery.js` - jQuery library for AJAX and DOM manipulation

## Development Commands

### Building and Uploading Firmware with PlatformIO

Navigate to the firmware directory:
```bash
cd firmware/NFCMusicPlayer
```

**Build the firmware:**
```bash
pio run
```

**Upload to ESP32:**
```bash
pio run --target upload
```

**Monitor serial output:**
```bash
pio device monitor
```

**Build, upload, and monitor in one command:**
```bash
pio run --target upload && pio device monitor
```

**Clean build files:**
```bash
pio run --target clean
```

### Programming the ESP32
- Requires 3.3V USB-to-serial adapter (never use 5V - it will damage the ESP32)
- To enter programming mode: Press BOOT button while pressing RESET
- PlatformIO will automatically handle upload timing in most cases

## Required Libraries

PlatformIO automatically installs these dependencies from platformio.ini:
- ESP32-audioI2S 2.0.0
- Adafruit_PN532 1.3.4
- AsyncTCP 3.4.0
- ESP Async WebServer 3.7.7
- ArduinoJson 7.4.1
- Arduino List 3.0.1

No manual library installation required when using PlatformIO.

## Key Design Patterns

### NFC Polling Strategy
- NFC reading runs in separate FreeRTOS task to avoid blocking audio playback
- Polls every 2 seconds (NFC_READ_INTERVAL)
- Tag presence triggers playback start, tag removal triggers stop
- Tag UID stored as colon-separated hex string: "AA:BB:CC:DD"

### Volume Control
- Uses analog potentiometer on GPIO 34
- Averages 200 samples (VOLUME_SAMPLES) before updating
- Maps 0-4095 ADC range to 0-20 volume steps
- Continuous monitoring in main loop

### Audio Playback
- Uses ESP32-audioI2S library with I2S interface
- Streams MP3 files directly from SD card
- Callback `audio_eof_mp3()` handles end-of-song events
- Song continues looping while NFC tag remains present

### WiFi Configuration
- Always runs as Access Point (no station mode)
- SSID: "NFCMusicPlayer"
- Password: "MyMusicPlayer"
- Static IP assigned by ESP32 softAP

## Web UI Architecture

### Design Philosophy
- **Dark Theme**: Minimalist slate-based color scheme optimized for readability
- **Offline-First**: All assets served locally from SD card (no CDN dependencies)
- **Mobile-Responsive**: Tailwind CSS utilities for adaptive layouts
- **Modern UX**: Toast notifications, smooth transitions, visual feedback

### Technology Stack
- **Tailwind CSS**: Custom minimal build (~12KB) with only used utility classes
- **jQuery**: AJAX requests and DOM manipulation
- **HTML5 Audio API**: In-browser MP3 preview playback
- **Vanilla JavaScript**: Toast notifications, file upload, table sorting

### UI Sections

**File Management**
- Unified table showing all MP3 files with integrated NFC tag management
- **Columns**: Play preview | Filename | Size | Upload Date | NFC Tag | Actions
- **Features**:
  - Sortable columns (name, size, date) - click headers to toggle
  - Default sort: newest files first
  - Audio preview with HTML5 Audio API (play/pause toggle)
  - **Inline NFC tag management**:
    - Unmapped files: "🔍 Scan & Assign Tag" button - one-click scan and assign
    - Mapped files: Tag ID badge with remove (✕) button
    - Automatic duplicate tag detection
  - File actions: Download, Rename, Delete
- **Upload Section**:
  - Visual file selection feedback
  - Real-time progress bar with percentage
  - Sequential file upload for reliability
  - Validates MP3 format before upload

### NFC Tag Workflow
1. User places NFC tag on the reader
2. User clicks "🔍 Scan & Assign Tag" button in the desired file's row
3. Button shows "Scanning..." loading state
4. System automatically:
   - Scans for tag via `/tagid` endpoint
   - Checks if tag is already mapped to another file
   - Assigns tag to the selected file via `/addmapping` endpoint
5. Table updates to show tag badge with remove button
6. Toast notification confirms success or reports errors

### Toast Notification System
Replaces browser `alert()` dialogs with modern, non-intrusive notifications:
- **Success** (green checkmark): File uploaded, mapping added, etc.
- **Error** (red X): Upload failed, invalid file, etc.
- **Warning** (amber triangle): No tag detected, validation issues
- **Info** (blue circle): General information
- Auto-dismiss after 3 seconds with slide-in animation

### Client-Side File Management
- **Upload**: Sequential chunked upload with progress tracking
- **Download**: Direct download via `/download?file=` endpoint
- **Delete**: Server validates file is not mapped before deletion
- **Rename**: Server automatically updates all mappings with new filename
- **Preview**: Streams file via `/download` endpoint to HTML5 Audio element

### Responsive Breakpoints
- **Mobile (<640px)**: Single column, size/date columns hidden, larger touch targets
- **Tablet (640-1024px)**: Size column visible, optimized spacing
- **Desktop (>1024px)**: All columns visible, full feature set

### State Management
- **Current Sort**: `{ column: 'date', ascending: false }` (newest first)
- **Current Audio**: Tracks playing preview, ensures only one plays at a time
- File list refreshes after: upload, delete, rename, mapping changes

## Pin Assignments

Critical SPI pin configuration (non-standard):
- MOSI: GPIO 23
- MISO: GPIO 19
- SCK: GPIO 18
- SD_CS: GPIO 13
- PN532_CS: GPIO 5

I2S audio pins:
- DOUT: GPIO 27
- BCLK: GPIO 26
- LRC: GPIO 25

## Hardware Constraints

- Maximum 20 tag mappings (MAX_SLOTS)
- MCP1826S regulator provides up to 1A at 3.3V
- Brownout detector disabled in firmware to prevent spurious resets
- 18650 battery provides approximately 10 hours playback

## Common Modifications

When modifying pin assignments:
- Update defines in NFCMusicPlayer.h
- Ensure SPI pins don't conflict with I2S
- Verify pin capabilities (ADC, output, etc.)

When adding new web endpoints:
- Add handler in Webserver.ino `initWebserver()`
- Use AsyncResponseStream with ArduinoJson for responses
- Keep JSON document sizes reasonable for ESP32 memory limits
- Update corresponding JavaScript in index.js to consume the endpoint
- Add toast notifications for user feedback

When modifying the web UI:
- **Styling**: Add utility classes to `tailwind.css` if needed (keep file minimal)
- **Custom CSS**: Use `index.css` only for browser-specific overrides (scrollbar, etc.)
- **Colors**: Dark theme uses slate-* color palette (slate-900 background, slate-100 text)
- **Responsive**: Use Tailwind breakpoints: sm (640px), md (768px), lg (1024px)
- **User Feedback**: Always use `showToast()` instead of `alert()` for notifications
- **File Operations**: Refresh file list and dropdowns after modifications
- **Offline Requirement**: Never use CDN links - all assets must be local on SD card
