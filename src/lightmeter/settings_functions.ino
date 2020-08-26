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

void SaveSettings() {
  // Save lightmeter setting into EEPROM.
  // To save on write cycles, only update the changed data
  EEPROM.update(ndIndexAddr, ndIndex);
  EEPROM.update(ISOIndexAddr, ISOIndex);
  EEPROM.update(modeIndexAddr, modeIndex);
  EEPROM.update(apertureIndexAddr, apertureIndex);
  EEPROM.update(T_expIndexAddr, T_expIndex);
  EEPROM.update(meteringModeAddr, meteringMode);
}

void LoadSettings() {
  ISOIndex =      EEPROM.read(ISOIndexAddr);
  apertureIndex = EEPROM.read(apertureIndexAddr);
  T_expIndex =    EEPROM.read(T_expIndexAddr);
  modeIndex =     EEPROM.read(modeIndexAddr);
  meteringMode =  EEPROM.read(meteringModeAddr);
  ndIndex =       EEPROM.read(ndIndexAddr);

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
}
