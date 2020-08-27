/* TODO
 *  - T values increment strangely
 *  - buzzer mode
*/

#include <Wire.h>
#include <BH1750.h>
#include <EEPROM.h>
//#include <avr/sleep.h>
#include <U8g2lib.h>
#include <Bounce2.h>

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
#define TimerButtonPin          4                       // Timer button
#define BuzzerPin               5                       // Buzzer (speaker) pin
#define BatteryVoltagePin        A0                      // Pin for measuring of the battery voltage
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

Button button = Button();
Button PlusButton = Button();         // "+" button state
Button MinusButton = Button();        // "-" button state
Button MeteringButton = Button();     // Metering button state
Button ModeButton = Button();         // Mode button state
Button MenuButton = Button();         // ISO button state
Button MeteringModeButton = Button(); // Metering mode button state (Ambient / Flash)
Button TimerButton = Button();        // Timer button

//boolean ISOMenu = false;
//boolean NDMenu = false;
//boolean mainScreen = false;
enum MenuItem { main, iso, nd, debug, none };
enum MenuItem currentMenuItem = main;

uint8_t ISOIndex;
uint8_t apertureIndex;
uint8_t T_expIndex;
uint8_t modeIndex;
uint8_t meteringMode;
uint8_t ndIndex;

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

bool displayUpdateNeeded = true;

// 26634 bytes

void setup() {
  // set up the buttons
  PlusButton.attach(PlusButtonPin, INPUT_PULLUP);
  PlusButton.interval(5);
  PlusButton.setPressedState(LOW);
  
  MinusButton.attach(MinusButtonPin, INPUT_PULLUP);
  MinusButton.interval(5);
  MinusButton.setPressedState(LOW);
  
  MeteringButton.attach(MeteringButtonPin, INPUT_PULLUP);
  MeteringButton.interval(5);
  MeteringButton.setPressedState(LOW);
  
  ModeButton.attach(ModeButtonPin, INPUT_PULLUP);
  ModeButton.interval(5);
  ModeButton.setPressedState(LOW);
  
  MenuButton.attach(MenuButtonPin, INPUT_PULLUP);
  MenuButton.interval(5);
  MenuButton.setPressedState(LOW);
  
  MeteringModeButton.attach(MeteringModeButtonPin, INPUT_PULLUP);
  MeteringModeButton.interval(5);
  MeteringModeButton.setPressedState(LOW);
  
  TimerButton.attach(TimerButtonPin, INPUT_PULLUP);
  TimerButton.interval(5);
  TimerButton.setPressedState(LOW);

  
  //pinMode(BuzzerPin, OUTPUT);

  //Serial.begin(115200);

  battVolts = readBatteryLevel();

  Wire.begin();
  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE_2);
  //lightMeter.begin(BH1750::ONE_TIME_LOW_RES_MODE); // for low resolution but 16ms light measurement time.

  display.begin();
  display.setFont(FONT_STANDARD);
  display.clear();


  LoadSettings();

  lux = getLux();
  refresh();
}

void menuMain() {
  if (MeteringButton.pressed()) {
    displayUpdateNeeded = true;
    // Save setting if Metering button pressed.
    SaveSettings();

    lux = 0;
//      refresh();
    
    if (meteringMode == 0) {
      // Ambient light meter mode.
      lightMeter.configure(BH1750::ONE_TIME_HIGH_RES_MODE_2);

      lux = getLux();

      if (Overflow == 1) {
        delay(10);
        getLux();
      }
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

//        refresh();
    }
  }

  if (ModeButton.pressed()) {
    displayUpdateNeeded = true;
    // switching between Aperture priority and Shutter Speed priority.
    if (currentMenuItem == main) {
      modeIndex++;

      if (modeIndex > 1) {
        modeIndex = 0;
      }
    }

//    refresh();
//    delay(200);
  }

  if (MeteringModeButton.pressed()) {
     displayUpdateNeeded = true;
    // Switch between Ambient light and Flash light metering
    if (meteringMode == 0) {
      meteringMode = 1;
    } else {
      meteringMode = 0;
    }

//    refresh();
//    delay(200);
  }

  if (PlusButton.pressed() || MinusButton.pressed()) {
    displayUpdateNeeded = true;
    if (modeIndex == 0) {
      // Aperture priority mode
      if (PlusButton.pressed()) {
        // Increase aperture.
        apertureIndex++;

        if (apertureIndex > MaxApertureIndex) {
          apertureIndex = 0;
        }
      } else if (MinusButton.pressed()) {
        // Decrease aperture
        if (apertureIndex > 0) {
          apertureIndex--;
        } else {
          apertureIndex = MaxApertureIndex;
        }
      }
    } else if (modeIndex == 1) {
      // Time priority mode
      if (PlusButton.pressed()) {
        // increase time
        T_expIndex++;

        if (T_expIndex > MaxTimeIndex) {
          T_expIndex = 0;
        }
      } else if (MinusButton.pressed()) {
        // decrease time
        if (T_expIndex > 0) {
          T_expIndex--;
        } else {
          T_expIndex = MaxTimeIndex;
        }
      }
    }

//    delay(200);
//
//    refresh();
  }

//  refresh();
}

void menuISO() {
  // ISO change mode
  if (PlusButton.pressed()) {
    displayUpdateNeeded = true;
    // increase ISO
    ISOIndex++;

    if (ISOIndex > MaxISOIndex) {
      ISOIndex = 0;
    }
  } else if (MinusButton.pressed()) {
    displayUpdateNeeded = true;
    if (ISOIndex > 0) {
      ISOIndex--;
    } else {
      ISOIndex = MaxISOIndex;
    }
  }

//  showISOMenu();
}

void menuND() {
  if (PlusButton.pressed()) {
    displayUpdateNeeded = true;
    ndIndex++;

    if (ndIndex > MaxNDIndex) {
      ndIndex = 0;
    }
  } else if (MinusButton.pressed()) {
    displayUpdateNeeded = true;
    if (ndIndex <= 0) {
      ndIndex = MaxNDIndex;
    } else {
      ndIndex--;
    }
  }
//  showNDMenu();
}

void menuDebug() {
  showDebugMenu();
}

void loop() {
  // update all the buttons
  PlusButton.update();
  MinusButton.update();
  MeteringButton.update();
  ModeButton.update();
  MenuButton.update();
  MeteringModeButton.update();
  TimerButton.update();

  // read the battery level every "batteryInterval"s
  if (millis() >= lastBatteryTime + batteryInterval) {
    lastBatteryTime = millis();
    battVolts = readBatteryLevel();
  }

  if (MenuButton.pressed()) {
    displayUpdateNeeded = true;
    currentMenuItem = currentMenuItem + 1;
  }
  // display the current ui based on which menu item we are on
  switch (currentMenuItem) {
    case MenuItem::main:
      menuMain();
      break;
    case MenuItem::iso:
      menuISO();
      break;
    case MenuItem::nd:
      menuND();
      break;
    case MenuItem::debug:
      menuDebug();
      break;
    case MenuItem::none:
      currentMenuItem = 0;
      break;
  }

  if (displayUpdateNeeded) {
    displayUpdateNeeded = false;
    display.firstPage();
    do {
      display.clearBuffer();
      
      // display the current ui based on which menu item we are on
      switch (currentMenuItem) {
        case MenuItem::main:
          refresh();
          break;
        case MenuItem::iso:
          showISOMenu();
          break;
        case MenuItem::nd:
          showNDMenu();
          break;
        case MenuItem::debug:
          showDebugMenu();
          break;
      }
    } while ( display.nextPage() );
  }
}
