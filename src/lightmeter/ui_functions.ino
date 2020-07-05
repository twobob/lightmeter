void outOfrange() {
  display.print(F("--"));
}

// Returns actual value of Vcc (x 100)
int getBandgap(void) {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  // For mega boards
  const long InternalReferenceVoltage = 1115L;  // Adjust this value to your boards specific internal BG voltage x1000
  // REFS1 REFS0          --> 0 1, AVcc internal ref. -Selects AVcc reference
  // MUX4 MUX3 MUX2 MUX1 MUX0  --> 11110 1.1V (VBG)         -Selects channel 30, bandgap voltage, to measure
  ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << ADLAR) | (0 << MUX5) | (1 << MUX4) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0);

#else
  // For 168/328 boards
  const long InternalReferenceVoltage = 1056L;  // Adjust this value to your boards specific internal BG voltage x1000
  // REFS1 REFS0          --> 0 1, AVcc internal ref. -Selects AVcc external reference
  // MUX3 MUX2 MUX1 MUX0  --> 1110 1.1V (VBG)         -Selects channel 14, bandgap voltage, to measure
  ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << ADLAR) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0);

#endif
  delay(50);  // Let mux settle a little to get a more stable A/D conversion
  // Start a conversion
  ADCSRA |= _BV( ADSC );
  // Wait for it to complete
  while ( ( (ADCSRA & (1 << ADSC)) != 0 ) );
  // Scale the value
  int results = (((InternalReferenceVoltage * 1024L) / ADC) + 5L) / 10L; // calculates for straight line value

  return results;
}

// TODO is this still in use?
void footer() {
  display.setCursor(0, 55);
  display.print(F("press M"));
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

  uint8_t ndStop = getND(ndIndex);

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

  uint8_t Tdisplay = 0; // Flag for shutter speed display style (fractional, seconds, minutes)
  double  Tfr = 0;
  float   Tmin = 0;

  if (T >= 60) {
    Tdisplay = 0;  // Exposure is in minutes
    Tmin = T / 60;

  } else if (T < 60 && T >= 0.5) {
    Tdisplay = 2;  // Exposure in in seconds

  } else if (T < 0.5) {
    Tdisplay = 1;  // Exposure is in fractional form
    Tfr = round(1 / T);
  }

  display.clear();

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
  // End of metering mode icon

  display.setCursor(ISO_VALUE_X, ISO_VALUE_Y);
  display.print(F("ISO:"));
  if (iso > 999999) {
    display.print(iso / 1000000.0, 2);
    display.print(F("M"));
  } else if (iso > 9999) {
    display.print(iso / 1000.0, 0);
    display.print(F("K"));
  } else {
    display.print(iso);
  }

  display.setCursor(LX_VALUE_X, LX_VALUE_Y);
  display.print(F("lx:"));
  display.print(lux, 0);

  // battery indicator
  // battery body
  display.drawFrame(BATTERY_X, BATTERY_Y + 1, 6, 8);
  // top of battery
  display.drawLine(BATTERY_X + 2, BATTERY_Y, BATTERY_X + 3, BATTERY_Y);

  // fill the battery indicator according to the measured battery level
  for (uint8_t i = 0; i <= map(battVolts, BATTERY_EMPTY_VALUE, BATTERY_FULL_VALUE, 0, 6); i++) {
    if(i > 6){
      break;
    }
    display.drawHLine(BATTERY_X + 1, 8 - i, 4);
  }

  display.drawLine(HEADER_SEPARATOR_START_X, HEADER_SEPARATOR_START_Y, HEADER_SEPARATOR_END_X, HEADER_SEPARATOR_END_Y); // LINE DIVISOR

  // f value
  display.setCursor(F_ICON_X, F_ICON_Y);
  display.setFont(FONT_H1);
  display.print(F("f/"));
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

  // Time value
  display.setFont(FONT_H1);
  display.setCursor(T_ICON_X, T_ICON_Y);
  display.print(F("T:"));
  display.setCursor(T_VALUE_X, T_VALUE_Y);
  if (Tdisplay == 0) {
    display.print(Tmin, 1);
    display.print(F("m"));
  } else if (Tdisplay == 1) {
    if (T > 0) {
      display.print(F("1/"));
      display.print(Tfr, 0);
    } else {
      outOfrange();
    }
  } else if (Tdisplay == 2) {
    display.print(T, 1);
    display.print(F("s"));
  } else if (Tdisplay == 3) {
    outOfrange();
  }

  // EV
  display.drawLine(EV_SEPARATOR_START_X, EV_SEPARATOR_START_Y, EV_SEPARATOR_END_X, EV_SEPARATOR_END_Y); // LINE DIVISOR
  display.setFont(FONT_STANDARD);
  display.setCursor(EV_ICON_X, EV_ICON_Y);
  display.print(F("EV: "));
  display.setCursor(EV_VALUE_X, EV_VALUE_Y);
  if (lux > 0) {
    display.print(EV, 0);
  } else {
    display.print(0, 0);
  }

  // ND filter indicator
  if (ndIndex > 0) {
    //display.setDrawColor(WHITE);
    //display.drawLine(0, 55, 128, 55); // LINE DIVISOR
    display.setFont(FONT_STANDARD);
    display.setCursor(0, 67);
    display.print(F("ND"));
    //display.setCursor(100, linePos[0] + 10);
    display.print(pow(2, ndIndex), 0);
    display.print(F("="));
    display.print(ndStop / 10.0, 1);
  }

  // priority marker (shutter or aperture priority indicator)
  display.setFont(FONT_STANDARD);
  if(modeIndex == 1) {
    display.setCursor(F_INDICATOR_X, F_INDICATOR_Y);
  }
  else {
    display.setCursor(T_INDICATOR_X, T_INDICATOR_Y);
  }
  display.print(F("*"));

  display.sendBuffer();
}

void showISOMenu() {
  ISOMenu = true;
  NDMenu = false;
  mainScreen = false;

  display.clear();
  display.setFont(FONT_H1);
  display.setCursor(MENU_ISO_TITLE_X, MENU_ISO_TITLE_Y);
  display.print(F("ISO"));
  display.setFont(FONT_H2);

  // get the actual iso value based on the index
  long iso = getISOByIndex(ISOIndex);
  // calculate based on the width of the text, where it should be positioned
  display.setCursor((DISPLAY_WIDTH / 2) - ((numDigits(iso) * 13) / 2), MENU_ISO_VALUE_Y);

  display.print(iso);

  display.sendBuffer();
  delay(200);
}

void showNDMenu() {
  ISOMenu = false;
  mainScreen = false;
  NDMenu = true;

  display.clear();
  //display.clearDisplay();
  display.setFont(FONT_H1);
  display.setCursor(MENU_ND_TITLE_X, MENU_ND_TITLE_Y);
  display.print(F("ND Filter"));
  display.setFont(FONT_H2);

  float ndValue = pow(2, ndIndex);
  // calculate based on the width of the text, where it should be positioned
  display.setCursor((DISPLAY_WIDTH / 2) - (((numDigits(ndValue) + 2) * 13) / 2), MENU_ND_VALUE_Y);


  if (ndIndex > 0) {
    display.print(F("ND"));
    display.print(ndValue, 0);
  } else {
    display.setFont(FONT_H1);
    display.setCursor(10, 50);
    display.print(F("No filter"));
  }

  display.sendBuffer();
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
