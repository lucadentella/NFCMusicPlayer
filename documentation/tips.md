# :mega: Tips

This page lists some tips to help building an NFCMusicPlayer

> Please refer to [Part list](https://github.com/lucadentella/NFCMusicPlayer/tree/main/documentation/partlist.md) page for the complete list of components.

### Fabric
![](https://github.com/lucadentella/NFCMusicPlayer/raw/main/images/fabric.jpg)

My suggestion is to add some speaker fabric to protect your speaker (and for aesthetic reasons, it's much nicer that way ;)).

I ordered [this cloth](https://www.aliexpress.com/item/1005005824294055.html) from Aliexpress... it will last you a few dozen of NFCMusicPlayers!

### Voltage regulator
I needed a *low-dropout* 3.3V voltage regulator that could provide up to **1A** of current. I considered some switching regulators, but to keep the project as simple as possible, I opted for a **linear** regulator and the [MCP1826S](https://ww1.microchip.com/downloads/en/DeviceDoc/22057B.pdf) from Microchip was the ideal choice:
![](https://github.com/lucadentella/NFCMusicPlayer/raw/main/images/1826s.png)

You probably can source it from Aliexpress or lcsc.com.

### NFC tags
![](https://github.com/lucadentella/NFCMusicPlayer/raw/main/images/tag.jpg)

NFCMusicPlayer is compatible with all the tags the PN532 chip can read.
I ordered some [cards](https://www.aliexpress.com/item/1005006428199332.html) but I also loved the [stickers](https://www.aliexpress.com/item/1005005883601752.html) because you can stick them to objects (*figurines*, *toys*...) and play the corresponding track if the idem is placed on top of your player!

### Battery
![](https://github.com/lucadentella/NFCMusicPlayer/raw/main/images/battery.jpg)

18650 lithium batteries are quite popular... with a capacity of 2000mAh I was able to play up to 10 hours of music before having to recharge it! 

### USB to serial adapter
It's very important to choose an USB to serial adapter with a working voltage of **3.3V**: you need it to program the firmware into the esp32 module and higher voltage will probably damage it!
Many adapters (like the [one I'm currently using](https://www.aliexpress.com/item/1005008182778961.html)) have a jumper or  microswitch to toggle between 5V and 3.3V... I suggest also to check with a multimeter the output voltage **before** connecting the adapter to the board to be safe!
![](https://github.com/lucadentella/NFCMusicPlayer/raw/main/images/usbserial.png)
