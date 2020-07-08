# lightmeter
A Lightmeter/Flashmeter for photographers, based on Arduino.
TODO update this!

# Task list
 - [x] Rewrite the code to use u8g2 instead of the Adafruit lib
 - [ ] Investigate: why does it take 2 measures in ambient mode to get the actual result?
 - [x] Refactor the code - so it is easier to do the next step
 - [x] Modify the code, to work with 128x32 displays
 - ~~[ ] Find a solution to get rid of the arduino nano, and be more power efficient, and use rechargeable batteries (1.2V) ( + boost regulator)~~
 - [ ] Make the whole thing take up less space and RAM and program space
  - [x] Use the single/double buffer mode
  - [ ] Use custom fonts, and restrict them to the minimum number of characters needed (possibly numbers only)
  - [ ] Use bitmaps insted of text where applicable (this helps to use smaller font set)
 - [ ] New feature: fixed aperture size, fixed iso, fixed timing: indicate that you need a flash, or how dark will the picture be (for low end cameras)
 - [ ] update the readme with the updated description, components and feature list. Also add credits to the original author!
 - [ ] modify getBandgap() for the new board and schematic
 - [ ] modify the schematic to have the buttons take up less pins
 - [ ] New feature: kitchen timer functionality: add a buzzer, and add code to count down the measured time for bulb mode photography
 - [ ] Draw new schematic diagram

## Notes
 - I was aiming to use an attiny45/85 (or something along these lines), but even the u8x8 lib uses ~430bytes of RAM, and that is a bit too much for a device that has 512bytes. I could have used my own [SH1106Lib](https://github.com/notisrac/SH1106Lib) (that is the display I am using) as it uses much less mem, but then we would have lost the ability to use any display you wanted. So I'm now using an Arduino Pro Mini clone.
 - Currently using double buffer mode. Speed is pretty good, RAM used is less than 1k. There is no need to use full buffer mode.

## Components:
TODO update this list:
1. Arduino NANO v.3 https://www.banggood.com/custlink/K3Kvbdnnea
2. BH1750 light sensor https://www.banggood.com/custlink/mKDv2Ip1dr or https://www.banggood.com/custlink/GvGvnRNN0e
3. SSD1306 128*64 OLED SPI Display https://www.banggood.com/custlink/DDKmsdAQ6z
4. Buttons https://www.banggood.com/custlink/m3DGAYsnnY
5. 50x70 PCB https://www.banggood.com/custlink/KvvvnybQAP
6. AAA battery Holder https://www.banggood.com/custlink/vK3KsynANN

Thanks @morozgrafix https://github.com/morozgrafix for creating schematic diagram for this device.

The lightmeter based on Arduino as a main controller and BH1750 as a metering cell. Information is displayed on SSD1306 OLED display. The device is powered by 2 AAA batteries.

## Functions list:
TODO update this list:
* Ambient light metering
* Flash light metering
* ND filter correction
* Aperture priority
* Shutter speed priority
* ISO range 8 - 4 000 000
* Aperture range 1.0 - 3251
* Shutter speed range 1/10000 - 133 sec
* ND Filter range ND2 - ND8192
* Displaying amount of light in Lux.
* Displaying exposure value, EV
* Recalculating exposure pair while one of the parameter changing
* Battery information
* Power 2xAAA LR03 batteries

Detailed information on my site: https://www.pominchuk.com/lightmeter/
