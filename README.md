# NFCMusicPlayer

![](https://github.com/lucadentella/NFCMusicPlayer/raw/main/images/cover.jpg)

Portable MP3 music player with NFC tags control

![](https://img.shields.io/badge/license-CC--BY--NC--SA-green)

## Features :trophy:

 - **SD card** reader
 - **Battery** powered with built-in **USB-C charger**
 - **Web based** configuration (map NFC tags to songs)
 - **Arduino** based firmware
 - **3D printed** enclosure

## :movie_camera: Tutorial
- [Youtube video - English](https://youtu.be/ck0IdwVWIIs)
- [Youtube video - Italiano](https://youtu.be/Wxny4k97sL8)

## :notebook: Documentation
 - [Part list](https://github.com/lucadentella/NFCMusicPlayer/tree/main/documentation/partlist.md)
 - [Some tips for building it](https://github.com/lucadentella/NFCMusicPlayer/tree/main/documentation/tips.md)
 - [Troubleshooting - Error codes](https://github.com/lucadentella/NFCMusicPlayer/tree/main/documentation/error-codes.md)

## :wrench: Board
- [Schematics, PDF](https://github.com/lucadentella/NFCMusicPlayer/raw/main/board/NFCMusicPlayer_12-sch.pdf)
- [KiCAD 8 files](https://github.com/lucadentella/NFCMusicPlayer/raw/main/board/)
- [SDcard content](https://github.com/lucadentella/NFCMusicPlayer/raw/main/sdcard/)
- [Project page for ready-made PCBs](https://www.pcbway.com/project/shareproject/NFCMusicPlayer_7cb0c6f8.html)
 
## :gift: Enclosure
- ["Thing" on Thingiverse](https://www.thingiverse.com/thing:6994529)

 ## :floppy_disk: Firmware
 - [Arduino sketch](https://github.com/lucadentella/NFCMusicPlayer/tree/main/firmware/NFCMusicPlayer)
 
**Required libraries**
- [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) 2.0.0
- [Adafruit_PN532](https://github.com/adafruit/Adafruit-PN532) 1.3.4
- [Async TCP](https://github.com/ESP32Async/AsyncTCP) 3.4.0
- [ESP Async WebServer](https://github.com/ESP32Async/ESPAsyncWebServer) 3.7.7
- [ArduinoJson](https://arduinojson.org/?utm_source=meta&utm_medium=library.properties) 7.4.1
- [Arduino List](https://github.com/nkaaf/Arduino-List) 3.0.1

