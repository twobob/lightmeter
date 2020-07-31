/* TODO
 *  - ISO value too long in the 128x32 display size
 *  - 3rd separator
 *  - T values increment strangely
 *  - buzzer moe
*/

#include <Wire.h>
#include <BH1750.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <U8g2lib.h>

//U8G2_SH1106_128X64_NONAME_2_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
U8G2_SSD1306_128X32_UNIVISION_2_HW_I2C display(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);
#define DISPLAY_WIDTH 128
//#define DISPLAY_HEIGHT 64
#define DISPLAY_HEIGHT 32

BH1750 lightMeter;

#define DomeMultiplier          2.17                    // Multiplier when using a white translucid Dome covering the lightmeter
#define MeteringButtonPin       3                       // Metering button pin
#define PlusButtonPin           8                       // Plus button pin
#define MinusButtonPin          9                       // Minus button pin
#define ModeButtonPin           7                       // Mode button pin
#define MenuButtonPin           2                       // ISO button pin
#define MeteringModeButtonPin   6                       // Metering Mode (Ambient / Flash)
#define BuzzerPin               5                       // Buzzer (speaker) pin
#define BatterVoltagePin        A0                      // Pin for measuring of the battery voltage
//#define PowerButtonPin          2

#define MaxISOIndex             57
#define MaxApertureIndex        70
#define MaxTimeIndex            80
#define MaxNDIndex              13
#define MaxFlashMeteringTime    5000                    // ms

float   lux;
boolean Overflow = false;                                   // Sensor got Saturated and Display "Overflow"
float   ISOND;
boolean ISOmode = false;
boolean NDmode = false;

boolean PlusButtonState;                // "+" button state
boolean MinusButtonState;               // "-" button state
boolean MeteringButtonState;            // Metering button state
boolean ModeButtonState;                // Mode button state
boolean MenuButtonState;                // ISO button state
boolean MeteringModeButtonState;        // Metering mode button state (Ambient / Flash)

boolean ISOMenu = false;
boolean NDMenu = false;
boolean mainScreen = false;

// EEPROM for memory recording
#define ISOIndexAddr        1
#define apertureIndexAddr   2
#define modeIndexAddr       3
#define T_expIndexAddr      4
#define meteringModeAddr    5
#define ndIndexAddr         6

#define defaultApertureIndex 12
#define defaultISOIndex      11
#define defaultModeIndex     0
#define defaultT_expIndex    19

uint8_t ISOIndex =          EEPROM.read(ISOIndexAddr);
uint8_t apertureIndex =     EEPROM.read(apertureIndexAddr);
uint8_t T_expIndex =        EEPROM.read(T_expIndexAddr);
uint8_t modeIndex =         EEPROM.read(modeIndexAddr);
uint8_t meteringMode =      EEPROM.read(meteringModeAddr);
uint8_t ndIndex =           EEPROM.read(ndIndexAddr);

uint8_t battVolts;
#define batteryInterval 10000
unsigned long lastBatteryTime = 0;

#define WHITE 1

#if DISPLAY_HEIGHT == 64
#include "ui_layout_128x64.h"
#elif DISPLAY_HEIGHT == 32
#include "ui_layout_128x32.h"
#endif

#define BATTERY_FULL_VALUE 90
#define BATTERY_EMPTY_VALUE 65

// 26634 bytes

void SaveSettings() {
  // Save lightmeter setting into EEPROM.
  EEPROM.write(ndIndexAddr, ndIndex);
  EEPROM.write(ISOIndexAddr, ISOIndex);
  EEPROM.write(modeIndexAddr, modeIndex);
  EEPROM.write(apertureIndexAddr, apertureIndex);
  EEPROM.write(T_expIndexAddr, T_expIndex);
  EEPROM.write(meteringModeAddr, meteringMode);
}

/*
  Read buttons state
*/
void readButtons() {
  PlusButtonState = digitalRead(PlusButtonPin);
  MinusButtonState = digitalRead(MinusButtonPin);
  MeteringButtonState = digitalRead(MeteringButtonPin);
  ModeButtonState = digitalRead(ModeButtonPin);
  MenuButtonState = digitalRead(MenuButtonPin);
  MeteringModeButtonState = digitalRead(MeteringModeButtonPin);
}

void setup() {  
  pinMode(PlusButtonPin, INPUT_PULLUP);
  pinMode(MinusButtonPin, INPUT_PULLUP);
  pinMode(MeteringButtonPin, INPUT_PULLUP);
  pinMode(ModeButtonPin, INPUT_PULLUP);
  pinMode(MenuButtonPin, INPUT_PULLUP);
  pinMode(MeteringModeButtonPin, INPUT_PULLUP);
  //pinMode(BuzzerPin, OUTPUT);

  //Serial.begin(115200);

  battVolts = readBatteryLevel();

  Wire.begin();
  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE_2);
  //lightMeter.begin(BH1750::ONE_TIME_LOW_RES_MODE); // for low resolution but 16ms light measurement time.

  display.begin();
  display.setFont(FONT_STANDARD);
  display.clear();


  // IF NO MEMORY WAS RECORDED BEFORE, START WITH THIS VALUES otherwise it will read "255"
  if (apertureIndex > MaxApertureIndex) {
    apertureIndex = defaultApertureIndex;
  }

  if (ISOIndex > MaxISOIndex) {
    ISOIndex = defaultISOIndex;
  }

  if (T_expIndex > MaxTimeIndex) {
    T_expIndex = defaultT_expIndex;
  }

  if (modeIndex < 0 || modeIndex > 1) {
    // Aperture priority. Calculating shutter speed.
    modeIndex = 0;
  }

  if (meteringMode > 1) {
    meteringMode = 0;
  }

  if (ndIndex > MaxNDIndex) {
    ndIndex = 0;
  }

  lux = getLux();
  refresh();
}

void loop() {  
    if (millis() >= lastBatteryTime + batteryInterval) {
      lastBatteryTime = millis();
      battVolts = readBatteryLevel();
    }
    
    readButtons();
  
    menu();
  
    if (MeteringButtonState == 0) {
      // Save setting if Metering button pressed.
      SaveSettings();
  
      lux = 0;
      refresh();
      
      if (meteringMode == 0) {
        // Ambient light meter mode.
        lightMeter.configure(BH1750::ONE_TIME_HIGH_RES_MODE_2);
  
        lux = getLux();
  
        if (Overflow == 1) {
          delay(10);
          getLux();
        }
  
        refresh();
        delay(200);
      } else if (meteringMode == 1) {
        // Flash light metering
        lightMeter.configure(BH1750::CONTINUOUS_LOW_RES_MODE);
  
        unsigned long startTime = millis();
        uint16_t currentLux = 0;
        lux = 0;
  
        while (true) {
          // check max flash metering time
          if (startTime + MaxFlashMeteringTime < millis()) {
            break;
          }
  
          currentLux = getLux();
          delay(16);
          
          if (currentLux > lux) {
            lux = currentLux;
          }
        }
  
        refresh();
      }
    }
}
