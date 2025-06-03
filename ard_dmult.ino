#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

const byte VOLT_PIN = A0;
const byte RES_PIN = A1;
const byte CONT_PIN = A2;
const byte AMMETER_PIN = A3;

const byte BUTTON_PIN = 3;
const byte SPEAKER_PIN = 4;
const byte BACKLIGHT_PIN = 13;

const float R1V = 330.0;
const float R2V = 220.0;
const float Vref = 5.0;
const float Rref = 10000.0;

const int Cont_thres = 1010; // cont. sens 
const float acs712_sens = 0.185;
const float acs712_off = 2.5;
const float Iref = 5.0;
LiquidCrystal_I2C lcd(0x27, 16, 2); 

int currentMode = 0;
int previousMode = 0;
int currentButtonState = HIGH;
int lastButtonState = HIGH;

unsigned long lastDebounceTime = 0;

void displayMenu(); // foward decl
void measureVoltage();
void measureResistance();
void checkContinuity();
void measureCurrent();
void updateMode();

void setup() {
  // if you wish to debug uncomment this
  // Serial.begin(9600); 

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(BACKLIGHT_PIN, OUTPUT);

  noTone(SPEAKER_PIN);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  lcd.begin(16, 2);
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
    case 0:
        displayMenu();
      break;
    case 1:
      measureVoltage();
      break;
    case 2:
      measureResistance();
      break;
    case 3:
      checkContinuity();
      break;
    case 4:
      measureCurrent();
      break;
  }

  delay(150);
}

void updateMode() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > 50) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
      if (currentButtonState == LOW) {
        currentMode = (int)((currentMode + 1) % 5);
      }
    }
  }
  lastButtonState = reading;
}

void displayMenu() {
  lcd.setCursor(0, 0);
  lcd.print(F("Press button"));
  lcd.setCursor(0, 1);
  lcd.print(F("V, R, Cont, A"));
}

void measureVoltage() {
  int adc_value = analogRead(VOLT_PIN);
  float adc_voltage = (adc_value / 1023) * Vref;
  float in_voltage = adc_voltage * (R1V + R2V) / R2V;

  if (in_voltage < 0.03) {
      in_voltage = 0.0;
  }

  // Serial.print(in_voltage, 2);
  // Serial.println(F(" V"));
  lcd.setCursor(0,0);
  lcd.print(F("Voltage"));
  lcd.setCursor(0, 1);
  lcd.print(in_voltage, 2);
  lcd.print(F(" V       "));
}

void measureResistance() {
  int sensorValue = analogRead(RES_PIN);
  float Vout = (sensorValue / 1023) * Vref;
  float R = 0.0;

  if (sensorValue <= 1) {
    R = 0.0;
  } else if (sensorValue >= (1023 - 1) ) {
    R = -1.0;
  } else if (Vout >= Vref) {
    R = -1.0;
  }
  else {
     if ((Vref / Vout) <= 1.0) {
        R = -1.0;
     } else {
        R = Rref * (1.0 / ((Vref / Vout) - 1.0));
     }
     if (R < 0 && R != -1.0) R = -1.0;
  }

  /*Serial.print(F("R: "));
  if (R == -1.0) {
      Serial.println(F("OL"));
  } else {
      Serial.print(R, 1);
      Serial.println(F(" ohm"));
  }*/

  lcd.setCursor(0, 0);
  lcd.print(F("Resistance:"));
  lcd.setCursor(0, 1);
  if (R == -1.0) {
    lcd.print(F("OL         "));
  } else if (R > 999999.9) {
    lcd.print(R / 1000000.0, 2);
    lcd.print(F(" MOhm   "));
  } else if (R > 999.9) {
    lcd.print(R / 1000.0, 2);
    lcd.print(F(" kOhm   "));
  } else {
    lcd.print(R, 1);
    lcd.print(F(" Ohm    "));
  }
}

void checkContinuity() {
  long sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += analogRead(CONT_PIN);
    delay(2);
  }
  int sensorValue = sum / 5;
  // Serial.print(F("Continuity ADC: "));
  // Serial.println(sensorValue);

  lcd.setCursor(0, 0);
  lcd.print(F("Continuity:"));
  lcd.setCursor(0, 1);

  if (sensorValue > Cont_thres) {
    tone(SPEAKER_PIN, 1000);
    lcd.print(F("Connected  "));
  } else {
    noTone(SPEAKER_PIN);
    lcd.print(F("Open       "));
  }
}

void measureCurrent() {
  float adc_sum = 0;
  for (int i = 0; i < 20; i++) {
    adc_sum += analogRead(AMMETER_PIN);
  }
  float adc_average = adc_sum / 20;
  adc_average = adc_average > 516.20 ? adc_average : 511.5;
  // Serial.println(adc_average);
  float voltage_at_pin = (float)((adc_average * 5.0) / 1023) ;
  float I = (voltage_at_pin - 2.5)/(acs712_sens*10);
  // Serial.println(adc_average);
  lcd.setCursor(0, 0);
  lcd.print(F("Current:"));
  lcd.setCursor(0, 1);
  

  if (I < 1.0 && I != 0.0 ) {
    lcd.print(I * 1000.0, 1);
    lcd.print(F(" mA    "));
  } else {
    lcd.print(I, 2);
    lcd.print(F(" A     "));
  }
  if (I >= 10.0) {
      lcd.print(F("   "));
  } else if (I >= 1.0) {
      lcd.print(F("    "));
  } else if (I * 1000.0 >= 100.0){
      lcd.print(F("  "));
  } else if (I * 1000.0 >= 10.0){
      lcd.print(F("   "));
  }
}