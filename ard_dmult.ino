#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

const byte ANALOG_IN_PIN    = A0;
const byte SENSOR_PIN       = A1;
const byte CONTINUITY_PIN   = A2;
const byte BUTTON_PIN       = 3;
const byte SPEAKER_PIN      = 4;
const byte BACKLIGHT_PIN    = 13;

const float R1_VOLTAGE_DIVIDER = 330.0;
const float R2_VOLTAGE_DIVIDER = 220.0;
const float REFERENCE_VOLTAGE  = 5.0;
const float R_REF_RESISTANCE   = 10000.0;
const float ADC_MAX_VALUE      = 1023.0;

const int CONTINUITY_THRESHOLD = 1010; // Adjust based on your circuit (LOW value typically means continuity)
const unsigned int SPEAKER_FREQ = 1000;
const int CONTINUITY_READ_COUNT = 5;
const int CONTINUITY_READ_DELAY = 2;

const unsigned long DEBOUNCE_DELAY = 50;

const byte LCD_ADDR = 0x27;
const byte LCD_COLS = 16;
const byte LCD_ROWS = 2;
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

enum Mode {
  MODE_MENU,
  MODE_VOLTAGE,
  MODE_RESISTANCE,
  MODE_CONTINUITY,
  NUM_MODES
};
Mode currentMode = MODE_MENU;
Mode previousMode = MODE_MENU;

int currentButtonState = HIGH;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

void displayMenu();
void measureVoltage();
void measureResistance();
void checkContinuity();
int readContinuity();
void updateMode();

void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(BACKLIGHT_PIN, OUTPUT);

  noTone(SPEAKER_PIN);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(F("Digital"));
  lcd.setCursor(0, 1);
  lcd.print(F("Multimeter"));
  delay(2000);
  lcd.clear();
}

void loop() {
  updateMode();

  if (currentMode != previousMode) {
    lcd.clear();
    noTone(SPEAKER_PIN);
    previousMode = currentMode;
  }

  switch (currentMode) {
    case MODE_MENU:
      displayMenu();
      break;
    case MODE_VOLTAGE:
      measureVoltage();
      break;
    case MODE_RESISTANCE:
      measureResistance();
      break;
    case MODE_CONTINUITY:
      checkContinuity();
      break;
  }

  delay(150);
}


void updateMode() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
      if (currentButtonState == LOW) {
        currentMode = (Mode)((currentMode + 1) % NUM_MODES);
      }
    }
  }
  lastButtonState = reading;
}

void displayMenu() {
  lcd.setCursor(0, 0);
  lcd.print(F("Press button"));
  lcd.setCursor(0, 1);
  lcd.print(F("for V, R, Cont"));
}

void measureVoltage() {
  int adc_value = analogRead(ANALOG_IN_PIN);
  float adc_voltage = (adc_value / ADC_MAX_VALUE) * REFERENCE_VOLTAGE;
  float in_voltage = adc_voltage * (R1_VOLTAGE_DIVIDER + R2_VOLTAGE_DIVIDER) / R2_VOLTAGE_DIVIDER;

  if (in_voltage < 0.01) {
      in_voltage = 0.0;
  }

  Serial.print(in_voltage, 2);
  Serial.println(" V");

  lcd.setCursor(0, 0);
  lcd.print(F("Voltage:"));
  lcd.setCursor(0, 1);
  lcd.print(in_voltage, 2);
  lcd.print(F(" V       "));
}

void measureResistance() {
  int sensorValue = analogRead(SENSOR_PIN);
  float Vout = (sensorValue / ADC_MAX_VALUE) * REFERENCE_VOLTAGE;
  float R = 0.0;

  if (sensorValue <= 1) {
    R = 0.0;
  } else if (sensorValue >= (int)(ADC_MAX_VALUE - 1) ) {
    R = -1.0; // Flag for OL
  } else {
     R = R_REF_RESISTANCE * (1.0 / ((REFERENCE_VOLTAGE / Vout) - 1.0));
     if (R < 0) R = -1.0;
  }

  Serial.print("R: ");
  if (R < 0) {
      Serial.println("OL");
  } else {
      Serial.print(R, 1);
      Serial.println(" ohm");
  }

  lcd.setCursor(0, 0);
  lcd.print(F("Resistance:"));
  lcd.setCursor(0, 1);
  if (R < 0) {
    lcd.print(F("OL         "));
  } else if (R > 1000000) {
    lcd.print(R / 1000000.0, 2);
    lcd.print(F(" MOhm   "));
  } else if (R > 1000) {
    lcd.print(R / 1000.0, 2);
    lcd.print(F(" kOhm   "));
  } else {
    lcd.print(R, 1);
    lcd.print(F(" Ohm    "));
  }
}

void checkContinuity() {
  int sensorValue = readContinuity();

  Serial.print("Continuity ADC: ");
  Serial.println(sensorValue);

  lcd.setCursor(0, 0);
  lcd.print(F("Continuity:"));
  lcd.setCursor(0, 1);

  if (sensorValue > CONTINUITY_THRESHOLD) { // Low value assumed for continuity
    tone(SPEAKER_PIN, SPEAKER_FREQ);
    lcd.print(F("Connected  "));
  } else {
    noTone(SPEAKER_PIN);
    lcd.print(F("Open       "));
  }
}

int readContinuity() {
  long sum = 0;
  for (int i = 0; i < CONTINUITY_READ_COUNT; i++) {
    sum += analogRead(CONTINUITY_PIN);
    delay(CONTINUITY_READ_DELAY);
  }
  return sum / CONTINUITY_READ_COUNT;
}