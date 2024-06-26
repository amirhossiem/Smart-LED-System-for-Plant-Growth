#include <Wire.h>
#include <DmxSimple.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

// Define the pins for DS1302
#define RST_PIN A2
#define DAT_PIN A1
#define CLK_PIN A0

ThreeWire myWire(DAT_PIN, CLK_PIN, RST_PIN); // DAT, CLK, RST
RtcDS1302<ThreeWire> rtc(myWire);

// Pin definitions for the TCS3200 color sensor
#define S0 10
#define S1 11
#define S2 12
#define S3 13
#define OUT 4

// Pin definitions for the LCD
const int rs = 2, en = 3, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // Initialize the LCD object

// Pin definition for LCD backlight
const int lcdBacklight = 5; // Adjust this pin based on your circuit

// DMX configuration
#define settingPin 5
uint8_t dmx_vals[4] = {0};

// RGB sensor readings
int R, G, B = 0;

// Plant growth stages
#define LEAFING 'l'
#define ROOTING 'r'
#define FLOWERING 'f'

// Pin definitions for the buttons
const int buttonEnter = 14;
const int buttonNext = 15;
const int buttonSetting = 16;
const int buttonExit = 17;
const int buttonPower = 18; // Pin for the rocker switch

// Menu states
#define MENU_MAIN 0
#define MENU_SETTING 1
#define MENU_SENSOR 2
#define MENU_TIME_SETTING 3

int menuState = MENU_MAIN;
int selectedStage = 0; // No stage selected initially
int currentSettingOption = 0; // Default setting option
bool systemPower = true; // System power state
bool stageSelected = false; // Flag to check if a stage is selected

// Variables for timekeeping
unsigned long previousMillis = 0;
unsigned long currentMillis;
unsigned long settingButtonPressMillis = 0;
bool settingButtonPressed = false;
int seconds = 0;
int minutes = 0;
int hours = 0;
int timeSettingStep = 0; // 0 for hours, 1 for minutes
int storedSeconds, storedMinutes, storedHours;
bool timeSet = false; // Flag to check if time is set


// Variables for debounce
unsigned long lastDebounceTimeEnter = 0;
unsigned long lastDebounceTimeNext = 0;
unsigned long lastDebounceTimeSetting = 0;
unsigned long lastDebounceTimeExit = 0;
const unsigned long debounceDelay = 50; // Debounce delay time in milliseconds


// Custom character for Rotting 1
byte Rooting1[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00011,
  B00100,
  B01010
};
// Custom character for Rotting 2
byte Rooting2[8] = {
  B00001,
  B00001,
  B00001,
  B00001,
  B00111,
  B11100,
  B10010,
  B10001
};
// Custom character for Rotting 3
byte Rooting3[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B11100,
  B00100,
  B01010,
  B10001
};
// Custom character for Rotting 1
byte Leafing1[8] = {
  B00000,
  B00000,
  B00001,
  B00101,
  B00111,
  B01111,
  B01011,
  B01000
};
// Custom character for Rotting 2
byte Leafing2[8] = {
  B00000,
  B01110,
  B11110,
  B11000,
  B11100,
  B10000,
  B11000,
  B00000
};
// Custom character for Rotting 3
byte Flowering1[8] = {
  B00000,
  B00000,
  B10001,
  B11101,
  B01111,
  B00011,
  B00001,
  B00000
};
// Custom character for Rotting 3
byte Flowering2[8] = {
  B00100,
  B01110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
// Custom character for Rotting 3
byte Flowering3[8] = {
  B00000,
  B00000,
  B10001,
  B10111,
  B11110,
  B11000,
  B10000,
  B00000
};
// Custom character for Heart
byte Heart[8] = {
  B00000,
  B01010,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000
};
// Custom character for flower 
byte flower[8] = {
  B01110,
  B11111,
  B01110,
  B00100,
  B10100,
  B01101,
  B00110,
  B00100
};
// Custom character for smile
byte a[8] = {
  B00001,
  B00011,
  B00111,
  B01110,
  B11110,
  B11111,
  B11111,
  B11111
};

byte b[8] = {
  B11111,
  B11111,
  B11111,
  B01110,
  B01110,
  B11111,
  B11111,
  B11111
};

byte c[8] = {
  B10000,
  B11000,
  B11100,
  B01110,
  B01111,
  B11111,
  B11111,
  B11111
};

byte d[8] = {
  B11011,
  B11000,
  B11100,
  B11110,
  B01111,
  B00111,
  B00011,
  B00001
};

byte e[8] = {
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};

byte f[8] = {
  B11011,
  B00011,
  B00111,
  B01111,
  B11110,
  B11100,
  B11000,
  B10000
};

// Function prototypes
void commandLights(char command);
void readRGBSensor();
void displayRGBValues();
void displayMainMenu();
void displaySettingMenu();
void displaySensorMenu();
void updateSelectedStage();
void updateLCD();
void powerControl();
void updateTime();
void saveTimeToEEPROM();
// void setTimeFromButtons(bool increase);
void saveSensorData();
void promptSaveData();
void saveDataToExcel();
bool debounce(int buttonPin, unsigned long& lastDebounceTime);
bool isSerialConnected();

// Global variables
bool savingData = false;
bool promptSave = false;
unsigned long buttonEnterPressTime = 0;
const unsigned long longPressDuration = 3000; // 3 seconds

void setup() {
  // Initialize RGB sensor pins
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);
  
  // Initialize RTC
  rtc.Begin();
  
  // Check if the RTC is running, if not set the time
  if (!rtc.GetIsRunning()) {
    rtc.SetIsRunning(true);
    // Set the RTC to the date & time this sketch was compiled
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    rtc.SetDateTime(compiled);
  }
  
  // Setting frequency-scaling to 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // Initialize serial communication and DMX
  Serial.begin(9600);
  pinMode(settingPin, OUTPUT);
  DmxSimple.usePin(5);
  DmxSimple.maxChannel(4);

  // Initialize the LCD
  lcd.begin(16, 2);

  // Initialize LCD backlight pin
  pinMode(lcdBacklight, OUTPUT);
  digitalWrite(lcdBacklight, HIGH); // Turn on backlight initially

  // Initialize button pins with pull-up resistors
  pinMode(buttonEnter, INPUT_PULLUP);
  pinMode(buttonNext, INPUT_PULLUP);
  pinMode(buttonSetting, INPUT_PULLUP);
  pinMode(buttonExit, INPUT_PULLUP);
  pinMode(buttonPower, INPUT_PULLUP); // Initialize power button pin with pull-up resistor

  // Display initial menu
  updateLCD();

}

void loop() {
  // Check power button state
  if (digitalRead(buttonPower) == HIGH) { // Invert the condition
    if (!systemPower) {
      systemPower = true;
      powerControl();
      delay(10); // Debounce delay
    }
  } else {
    if (systemPower) {
      systemPower = false;
      powerControl();
      delay(10); // Debounce delay
    }
  }

  if (systemPower) {
    // Update time
    updateTime();

    switch(menuState) {
      case MENU_MAIN:
        displayMainMenu();
        break;
      case MENU_SETTING:
        displaySettingMenu();
        break;
      case MENU_SENSOR:
        displaySensorMenu();
        break;
      case MENU_TIME_SETTING:
        // Ensure that the time is set from the RTC on the first press
        if (!timeSet) {
          updateLCD();
        }
        break;
    }
 
    // Check button states with debounce
    if (debounce(buttonEnter, lastDebounceTimeEnter)) {
      if (menuState == MENU_SETTING) {
        updateSelectedStage();
        menuState = MENU_MAIN;
        updateLCD();
      } else if (menuState == MENU_TIME_SETTING) {
        if (timeSettingStep == 0) {
          // Set hours
          timeSettingStep++;
        } else if (timeSettingStep == 1) {
          // Set minutes
          timeSettingStep = 0;
          timeSet = true;
          menuState = MENU_MAIN;
        }
        updateLCD();
      } else if (menuState == MENU_SENSOR && !promptSave) {
        savingData = true;
        promptSaveData();
      } else if (promptSave) {
        saveSensorData();
      }
    } else if (debounce(buttonNext, lastDebounceTimeNext)) {
      if (menuState == MENU_SETTING) {
        currentSettingOption = (currentSettingOption + 1) % 3;
        updateLCD();
      } else if (menuState == MENU_TIME_SETTING) {
        // setTimeFromButtons(true); // Adjust the time
        updateLCD();
      }
    } else if (debounce(buttonSetting, lastDebounceTimeSetting)) {
      menuState = (menuState + 1) % 4;
      if (menuState == MENU_MAIN) {
        menuState = MENU_SETTING; // Skip MENU_MAIN when cycling through settings
      }
      updateLCD();
    } else if (debounce(buttonExit, lastDebounceTimeExit)) {
      if (menuState == MENU_TIME_SETTING) {
        timeSettingStep = 0;
        timeSet = true;
        menuState = MENU_MAIN;
        updateLCD();
      } else if (menuState != MENU_MAIN) {
        menuState = MENU_MAIN;
        updateLCD();
      } else if (promptSave) {
        promptSave = false;
        displaySensorMenu();
      }
      
    }

    // Write the DMX values to the respective channels
    for (int i = 0; i < 4; i++) {
      DmxSimple.write(i + 1, dmx_vals[i]);
    }

    // Read RGB sensor values
    readRGBSensor();

    // Print RGB Sensor Values to Serial
    displayRGBValues();
  }
}

void displaySensorMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sensor Values");
  lcd.setCursor(0, 1);
  lcd.print("R:");
  lcd.print(R);
  lcd.print(" G:");
  lcd.print(G);
  lcd.print(" B:");
  lcd.print(B);

  if (savingData) {
    lcd.setCursor(0, 0);
    lcd.print("Save Data");
    lcd.setCursor(0, 1);
    lcd.print("Yes=Enter No=Exit");
  }
}

void promptSaveData() {
  promptSave = true;
  displaySensorMenu();
}

void saveSensorData() {
  if (isSerialConnected()) {
    saveDataToExcel();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Save Successful");
    delay(2500); // Display for 2 seconds
    savingData = false;
    promptSave = false;
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Serial Conn.");
    lcd.setCursor(0, 1);
    lcd.print("Plug & Retry");
    delay(2500); // Display for 2 seconds
  }
}

bool isSerialConnected() {
  return Serial;
}

void saveDataToExcel() {
  RtcDateTime now = rtc.GetDateTime();
  Serial.print(now.Year(), DEC);
  Serial.print('/');
  Serial.print(now.Month(), DEC);
  Serial.print('/');
  Serial.print(now.Day(), DEC);
  Serial.print(" ");
  Serial.print(now.Hour(), DEC);
  Serial.print(':');
  Serial.print(now.Minute(), DEC);
  Serial.print(':');
  Serial.print(now.Second(), DEC);
  Serial.print(", R=");
  Serial.print(R);
  Serial.print(", G=");
  Serial.print(G);
  Serial.print(", B=");
  Serial.println(B);
}

void commandLights(char command) {
  switch(command) {
    case ROOTING:
      dmx_vals[2] = 0;
      dmx_vals[0] = 46.5;
      dmx_vals[1] = 178.5;
      dmx_vals[3] = 0;
      digitalWrite(settingPin, HIGH);
      Serial.println("Command received: Rooting stage");
      break;
    case LEAFING:
      dmx_vals[2] = 0;
      dmx_vals[0] = 127.5;
      dmx_vals[1] = 127.5;
      dmx_vals[3] = 0;
      digitalWrite(settingPin, LOW);
      Serial.println("Command received: Leafing stage");
      break;
    case FLOWERING:
      dmx_vals[2] = 0;
      dmx_vals[0] = 204;
      dmx_vals[1] = 0;
      dmx_vals[3] = 51;
      digitalWrite(settingPin, LOW);
      Serial.println("Command received: Flowering stage");
      break;
    default:
      return;
  }
  stageSelected = true; // Stage is now selected
}

void readRGBSensor() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  R = pulseIn(OUT, LOW);
  delay(100);

  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  G = pulseIn(OUT, LOW);
  delay(100);

  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  B = pulseIn(OUT, LOW);
  delay(100);
}

void displayRGBValues() {
  RtcDateTime now = rtc.GetDateTime();
  Serial.print(now.Year(), DEC);
  Serial.print('/');
  Serial.print(now.Month(), DEC);
  Serial.print('/');
  Serial.print(now.Day(), DEC);
  Serial.print(" ");
  Serial.print(now.Hour(), DEC);
  Serial.print(':');
  Serial.print(now.Minute(), DEC);
  Serial.print(':');
  Serial.print(now.Second(), DEC);
  Serial.print(", R=");
  Serial.print(R);
  Serial.print(", G=");
  Serial.print(G);
  Serial.print(", B=");
  Serial.println(B);
}

void displayMainMenu() {

  RtcDateTime now = rtc.GetDateTime();  // Get the current time and date from the RTC
  
  // Retrieve and format the time
  hours = now.Hour();
  minutes = now.Minute();
  seconds = now.Second();
  
  // Retrieve and format the date
  // int year = now.Year() - 2000; // Extract the last two digits of the year
  int month = now.Month();
  int day = now.Day();
  
  // Clear the LCD and display the time
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(hours < 10 ? "0" : "");
  lcd.print(hours);
  lcd.print(":");
  lcd.print(minutes < 10 ? "0" : "");
  lcd.print(minutes);
  lcd.print(":");
  lcd.print(seconds < 10 ? "0" : "");
  lcd.print(seconds);

  lcd.createChar(7, flower);
  lcd.setCursor(9, 0);
  lcd.write(byte(7)); // Write custom character (Flower2)

  lcd.setCursor(11, 0);
  lcd.print(month < 10 ? "0" : "");
  lcd.print(month);
  lcd.print("/");
  lcd.print(day < 10 ? "0" : "");
  lcd.print(day);

  lcd.setCursor(0, 1);
  lcd.print("Stage:");
  if (systemPower && stageSelected) { // Check if the system is powered on and a stage is selected
    switch (selectedStage) {
      case ROOTING:
        lcd.print("Rooting");
        break;
      case LEAFING:
        lcd.print("Leafing");
        break;
      case FLOWERING:
        lcd.print("Flowering");
        break;
    }
  } else {
    lcd.print("Not selected"); // Display "Not selected" if no stage is selected or the system is powered off
  }
}

void displaySettingMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Growth Stage");
  lcd.setCursor(0, 1);
  switch(currentSettingOption) {
    case 0:
      lcd.print("> Rooting   ");
      lcd.createChar(8, Rooting1);
      lcd.setCursor(11, 1);
      lcd.write(byte(8)); // Write custom character (Rooting1)
      lcd.createChar(9, Rooting2);
      lcd.setCursor(12, 1);
      lcd.write(byte(9)); // Write custom character (Rooting2)
      lcd.createChar(10, Rooting3);
      lcd.setCursor(13, 1);
      lcd.write(byte(10)); // Write custom character (Rooting3)
      break;
    case 1:
      lcd.print("> Leafing   ");
      lcd.createChar(11, Leafing1);
      lcd.setCursor(12, 1);
      lcd.write(byte(11)); // Write custom character (Leafing1)
      lcd.createChar(12, Leafing2);
      lcd.setCursor(13, 1);
      lcd.write(byte(12)); // Write custom character (Leafing2)
      break;
    case 2:
      lcd.print("> Flowering ");
      lcd.createChar(13, Flowering1);
      lcd.setCursor(12, 1);
      lcd.write(byte(13)); // Write custom character (Flowering1)
      lcd.createChar(14, Flowering2);
      lcd.setCursor(13, 1);
      lcd.write(byte(14)); // Write custom character (Flowering2)
      lcd.createChar(15, Flowering3);
      lcd.setCursor(14, 1);
      lcd.write(byte(15)); // Write custom character (Flowering3)
      break;
  }
}

void updateSelectedStage() {
  switch(currentSettingOption) {
    case 0:
      selectedStage = ROOTING;
      break;
    case 1:
      selectedStage = LEAFING;
      break;
    case 2:
      selectedStage = FLOWERING;
      break;
  }
  commandLights(selectedStage);
}

void updateLCD() {
  switch(menuState) {
    case MENU_MAIN:
      displayMainMenu();
      break;
    case MENU_SETTING:
      displaySettingMenu();
      break;
    case MENU_SENSOR:
      displaySensorMenu();
      break;
  }
}

void powerControl() {
  if (systemPower) {
    Serial.println("System Powered ON");
    // Turn on LCD backlight
    digitalWrite(lcdBacklight, HIGH);

    // Display custom characters on LCD for 2 seconds
    lcd.createChar(1, a);
    lcd.createChar(2, b);
    lcd.createChar(3, c);
    lcd.createChar(4, d);
    lcd.createChar(5, e);
    lcd.createChar(6, f);

    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.write(byte(1));
    lcd.setCursor(7, 0);
    lcd.write(byte(2));
    lcd.setCursor(8, 0);
    lcd.write(byte(3));
    lcd.setCursor(6, 1);
    lcd.write(byte(4));
    lcd.setCursor(7, 1);
    lcd.write(byte(5));
    lcd.setCursor(8, 1);
    lcd.write(byte(6));
    delay(2000); // Display custom characters for 2 seconds

    // Display "Welcome" on LCD and scroll "Plant Growth DARE" in 5 seconds
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("Welcome ");
    lcd.createChar(0, Heart);
    lcd.setCursor(4, 0);
    lcd.write(byte(0)); // Write custom character (Heart)
    lcd.createChar(0, Heart);
    lcd.setCursor(12, 0);
    lcd.write(byte(0)); // Write custom character (Heart)

    String message = " Plant Growth DARE     ";
    unsigned long startTime = millis();
    while (millis() - startTime < 5000) { // Scroll for 5 seconds
      for (int position = 0; position < message.length() - 16; position++) {
        lcd.setCursor(0, 1);
        lcd.print(message.substring(position, position + 16));
        delay(500); // Scrolling speed
        if (millis() - startTime >= 5000) {
          break;
        }
      }
    }

    lcd.clear();
    updateLCD();
  } else {
    Serial.println("System Powered OFF");
    // Save current time to EEPROM
    saveTimeToEEPROM();
    // Turn off all devices
    for (int i = 0; i < 4; i++) {
      dmx_vals[i] = 0;
      DmxSimple.write(i + 1, dmx_vals[i]);
    }
    // Display "System OFF" on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("System OFF");
    delay(3500); // Display for 3.5 seconds
    // Turn off LCD backlight
    digitalWrite(lcdBacklight, LOW);
  }
}

void updateTime() {
  // Get the current time from the RTC
  RtcDateTime now = rtc.GetDateTime();
  hours = now.Hour();
  minutes = now.Minute();
  seconds = now.Second();
}

void saveTimeToEEPROM() {
  EEPROM.write(0, seconds);
  EEPROM.write(1, minutes);
  EEPROM.write(2, hours);
}

void loadTimeFromEEPROM() {
  seconds = EEPROM.read(0);
  minutes = EEPROM.read(1);
  hours = EEPROM.read(2);
}

bool debounce(int buttonPin, unsigned long& lastDebounceTime) {
  bool buttonState = digitalRead(buttonPin) == LOW;
  unsigned long currentTime = millis();
  if (buttonState && (currentTime - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = currentTime;
    return true;
  }
  return false;
}
