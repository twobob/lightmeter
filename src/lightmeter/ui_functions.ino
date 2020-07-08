void outOfrange() {
  display.print(F("--"));
}

// TODO is this still in use?
void footer() {
  display.setCursor(0, 55);
  display.print(F("press M"));
}

void DisplayMeteringMode(uint8_t meteringMode) {
  display.setFont(FONT_STANDARD);
  // Metering mode icon
  display.setCursor(METERING_MODE_X, METERING_MODE_Y);
  if (meteringMode == 0) {
    // Ambient light
    display.print(F("A"));
  } else if (meteringMode == 1) {
    // Flash light
    display.print(F("F"));
  }
}

/**
 * Displays the shutter speed based on the time value
 */
void DisplayTime(float T) {
  display.setFont(FONT_H1);
  display.setCursor(T_ICON_X, T_ICON_Y);
  display.print(F("T"));
  display.setCursor(T_VALUE_X, T_VALUE_Y);

  if (T > 0) {
    // there is place for 4 digits, or 3 digits and a letter
    if (T >= 60 && T <= 59940) {
      // minutes
      float val = T / 60.0;
      display.print(val, 0);
      display.print(F("m"));
      if (val < 10) {
        uint8_t mod = (uint8_t)T % 60;
        if (mod > 0) {
          display.print(mod);
          display.print(F("s"));
        }
      }
    }
    else if (T < 60 && T >= 0.5) {
      // seconds
      display.print(T, (T < 10)? 1 : 0);
      display.print(F("s"));
    }
    else if (T < 0.5) {
      // Exposure is in fractional form
      display.print(F("1/"));
      uint8_t val = round(1 / T);
      display.print(val);
    }
    else {
      outOfrange();
      //display.print(F("inf."));
    }
  }
  else {
    outOfrange();
  }
}

void DisplayAperture(float A) {
  display.setCursor(F_ICON_X, F_ICON_Y);
  display.setFont(FONT_H1);
  display.print(F("f"));
  display.setCursor(F_VALUE_X, F_VALUE_Y);
  if (A > 0) {
    if (A >= 100) {
      display.print(A, 0);
    } else {
      display.print(A, 1);
    }
  } else {
    outOfrange();
  }
}

void DisplayNDFilter() {
  uint8_t ndStop = getND(ndIndex);
  display.setFont(FONT_STANDARD);
  display.setCursor(ND_ICON_X, ND_ICON_Y);
  display.print(F("ND"));

  if (ndIndex > 0) {
    display.setCursor(ND_VALUE_X, ND_VALUE_Y);
    display.print(ndStop / 10.0, 1);
    
    // second line of ND data
    if (ND_VALUE_2_X > -1) {
      display.setCursor(ND_VALUE_2_X, ND_VALUE_2_Y);
      display.print(F("("));
      display.print(pow(2, ndIndex), 0);
      display.print(F(")"));
    }
  }
  else {
    // no nd filter selected
    outOfrange();
  }
}

void DisplayLux(float lux) {
  display.setCursor(LX_ICON_X, LX_ICON_Y);
  display.print(F("lx:"));
  display.setCursor(LX_VALUE_X, LX_VALUE_Y);
  display.print(lux, 0);
}

void DisplayBatteryIndicator(int battVolts) {
  // battery body
  display.drawFrame(BATTERY_X, BATTERY_Y + 1, 6, 8);
  // top of battery
  display.drawLine(BATTERY_X + 2, BATTERY_Y, BATTERY_X + 3, BATTERY_Y);

  // fill the battery indicator according to the measured battery level
  for (uint8_t i = 0; i <= map(battVolts, BATTERY_EMPTY_VALUE, BATTERY_FULL_VALUE, 0, 6); i++) {
    if (i > 6) {
      break;
    }
    display.drawHLine(BATTERY_X + 1, BATTERY_Y + 8 - i, 4);
  }
}

void DisplayEV(float lux, float EV) {
  display.setFont(FONT_STANDARD);
  display.setCursor(EV_ICON_X, EV_ICON_Y);
  display.print(F("EV: "));
  display.setCursor(EV_VALUE_X, EV_VALUE_Y);
  if (lux > 0) {
    display.print(EV, 0);
  } else {
    display.print(0, 0);
  }
}

void DisplayPriorityIndicator(uint8_t modeIndex) {
  display.setFont(FONT_STANDARD);
  if (modeIndex == 0) {
    display.setCursor(F_INDICATOR_X, F_INDICATOR_Y);
  }
  else {
    display.setCursor(T_INDICATOR_X, T_INDICATOR_Y);
  }
  display.print(F("*"));

}

void DisplayISO(long iso) {
  display.setCursor(ISO_ICON_X, ISO_ICON_Y);
  display.print(F("ISO"));
  display.setCursor(ISO_VALUE_X, ISO_VALUE_Y);
  if (iso > 999999) {
    display.print(iso / 1000000.0, 1);
    display.print(F("M"));
  }
  else if (iso > 9999) {
    display.print(iso / 1000.0, 0);
    display.print(F("K"));
  }
  else {
    display.print(iso);
  }  
}


// Calculate new exposure value and display it.
void refresh() {
  ISOMenu = false;
  mainScreen = true;
  NDMenu = false;

  float EV = lux2ev(lux);

  float T = getTimeByIndex(T_expIndex);
  float A = getApertureByIndex(apertureIndex);
  long  iso = getISOByIndex(ISOIndex);

  // if ND filter is configured then make corrections.
  // As ISO is a main operand in all EV calculations we can adjust ISO by ND filter factor.
  // if ND4 (ND 0.6) filter is configured then we need to adjust ISO to -2 full stops. Ex. 800 to 200
  if (ndIndex > 0) {
    ISOND = iso / (pow(2, ndIndex));
  } else {
    ISOND = iso;
  }

  if (lux > 0) {
    if (modeIndex == 0) {
      // Aperture priority. Calculating time.
      T = fixTime(100 * pow(A, 2) / ISOND / pow(2, EV)); //T = exposure time, in seconds

      // Calculating shutter speed index for correct menu navigation.
      for (int i = 0; i <= MaxTimeIndex; i++) {
        if (T == getTimeByIndex(i)) {
          T_expIndex = i;
          break;
        }
      }
    } else if (modeIndex == 1) {
      // Shutter speed priority. Calculating aperture.
      A = fixAperture(sqrt(pow(2, EV) * ISOND * T / 100));

      // Calculating aperture index for correct menu navigation.
      if (A > 0) {
        for (int i = 0; i <= MaxApertureIndex; i++) {
          if (A == getApertureByIndex(i)) {
            apertureIndex = i;
            break;
          }
        }
      }
    }
  } else {
    if (modeIndex == 0) {
      T = 0;
    } else {
      A = 0;
    }
  }

  display.firstPage();
  do {
    display.clearBuffer();
  
    // metering mode icon
    DisplayMeteringMode(meteringMode);
  
    // current iso value
    DisplayISO(iso);
  
    // current lux value
    DisplayLux(lux);
  
    // battery indicator
    DisplayBatteryIndicator(battVolts);
  
    display.drawLine(HEADER_SEPARATOR_START_X, HEADER_SEPARATOR_START_Y, HEADER_SEPARATOR_END_X, HEADER_SEPARATOR_END_Y); // LINE DIVISOR
  
    // f value
    DisplayAperture(A);
  
    // Time value
    DisplayTime(T);
  
    display.drawLine(EV_SEPARATOR_START_X, EV_SEPARATOR_START_Y, EV_SEPARATOR_END_X, EV_SEPARATOR_END_Y); // LINE DIVISOR
  
    // EV
    DisplayEV(lux, EV);
  
    display.drawLine(SEPARATOR_3_START_X, SEPARATOR_3_START_Y, SEPARATOR_3_END_X, SEPARATOR_3_END_Y); // LINE DIVISOR
  
    // ND filter indicator
    DisplayNDFilter();
  
    // priority marker (shutter or aperture priority indicator)
    DisplayPriorityIndicator(modeIndex);
  } while ( display.nextPage() );

  //display.sendBuffer();
}

void showISOMenu() {
  ISOMenu = true;
  NDMenu = false;
  mainScreen = false;

  display.firstPage();
  do {
    display.clearBuffer();
    display.setFont(FONT_H1);
    display.setCursor(MENU_ISO_TITLE_X, MENU_ISO_TITLE_Y);
    display.print(MENU_ISO_TITLE_TEXT);
    display.setFont(FONT_H2);
  
    display.setCursor(MENU_ISO_VALUE_X, MENU_ISO_VALUE_Y);
    // get the actual iso value based on the index
    display.print(getISOByIndex(ISOIndex));

  //display.sendBuffer();
  } while ( display.nextPage() );
  delay(200);
}

void showNDMenu() {
  ISOMenu = false;
  mainScreen = false;
  NDMenu = true;

  display.firstPage();
  do {
  display.clearBuffer();
  //display.clearDisplay();
  display.setFont(FONT_H1);
  display.setCursor(MENU_ND_TITLE_X, MENU_ND_TITLE_Y);
  display.print(MENU_ND_TITLE_TEXT);
  display.setFont(FONT_H2);

  display.setCursor(MENU_ND_VALUE_X, MENU_ND_VALUE_Y);

  if (ndIndex > 0) {
    display.print(F("ND"));
    display.print(pow(2, ndIndex), 0);
  } else {
    display.setFont(FONT_H1);
    display.print(F("No filter"));
  }

  //display.sendBuffer();
  } while ( display.nextPage() );
  delay(200);
}

// Navigation menu
void menu() {
  if (MenuButtonState == 0) {
    if (mainScreen) {
      showISOMenu();
    } else if (ISOMenu) {
      showNDMenu();
    } else {
      refresh();
      delay(200);
    }
  }

  if (NDMenu) {
    if (PlusButtonState == 0) {
      ndIndex++;

      if (ndIndex > MaxNDIndex) {
        ndIndex = 0;
      }
    } else if (MinusButtonState == 0) {
      if (ndIndex <= 0) {
        ndIndex = MaxNDIndex;
      } else {
        ndIndex--;
      }
    }

    if (PlusButtonState == 0 || MinusButtonState == 0) {
      showNDMenu();
    }
  }

  if (ISOMenu) {
    // ISO change mode
    if (PlusButtonState == 0) {
      // increase ISO
      ISOIndex++;

      if (ISOIndex > MaxISOIndex) {
        ISOIndex = 0;
      }
    } else if (MinusButtonState == 0) {
      if (ISOIndex > 0) {
        ISOIndex--;
      } else {
        ISOIndex = MaxISOIndex;
      }
    }

    if (PlusButtonState == 0 || MinusButtonState == 0) {
      showISOMenu();
    }
  }

  if (ModeButtonState == 0) {
    // switching between Aperture priority and Shutter Speed priority.
    if (mainScreen) {
      modeIndex++;

      if (modeIndex > 1) {
        modeIndex = 0;
      }
    }

    refresh();
    delay(200);
  }

  if (mainScreen && MeteringModeButtonState == 0) {
    // Switch between Ambient light and Flash light metering
    if (meteringMode == 0) {
      meteringMode = 1;
    } else {
      meteringMode = 0;
    }

    refresh();
    delay(200);
  }

  if (mainScreen && (PlusButtonState == 0 || MinusButtonState == 0)) {
    if (modeIndex == 0) {
      // Aperture priority mode
      if (PlusButtonState == 0) {
        // Increase aperture.
        apertureIndex++;

        if (apertureIndex > MaxApertureIndex) {
          apertureIndex = 0;
        }
      } else if (MinusButtonState == 0) {
        // Decrease aperture
        if (apertureIndex > 0) {
          apertureIndex--;
        } else {
          apertureIndex = MaxApertureIndex;
        }
      }
    } else if (modeIndex == 1) {
      // Time priority mode
      if (PlusButtonState == 0) {
        // increase time
        T_expIndex++;

        if (T_expIndex > MaxTimeIndex) {
          T_expIndex = 0;
        }
      } else if (MinusButtonState == 0) {
        // decrease time
        if (T_expIndex > 0) {
          T_expIndex--;
        } else {
          T_expIndex = MaxTimeIndex;
        }
      }
    }

    delay(200);

    refresh();
  }
}
