#include <LiquidCrystal_I2C.h>

#define ANALOG_IN_PIN A0
#define BACKLIGHT_PIN 13
#define SENSOR_PIN A1
#define BUTTON_PIN 3
#define SPEAKER_PIN 4
#define CONTINUITY_PIN A2

float R1 = 330.0;
float R2 = 220.0;
float ref_voltage = 5.0;
float adc_voltage = 0.0;
float in_voltage = 0.0;
int adc_value = 0;

int relayPin = 2;
int TRIG = 1;

const int sensorPin = SENSOR_PIN;
int sensorValue = 0;
float Vin = 5;
float Vout = 0;
float Rref = 10000;
float R = 0;

const int continuityPin = CONTINUITY_PIN;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // dont change these numbers

int mode = 0;  // 0 = menu, 1 = voltage, 2 = resistance, 3 = continuity
int buttonState = 0;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 20;

void setup() {
  Serial.begin(9600);
  
  pinMode(relayPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  
  lcd.begin(16, 2);
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("Digital");
  lcd.setCursor(0, 1);
  lcd.print("Multimeter");
  delay(2000);
}

int readContinuity() {
  long sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += analogRead(continuityPin);
    delay(2);  
  }
  return sum / 5;
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        mode = (mode + 1) % 4;
        Serial.print("Button pressed, mode changed to: ");
        Serial.println(mode);
      }
    }
  }

  lastButtonState = reading;

  lcd.clear();
  
  if (mode == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Press button");
    lcd.setCursor(0, 1);
    lcd.print("to cycle V,R,C");
  }
  else if (mode == 1) {
    adc_value = analogRead(ANALOG_IN_PIN);
    adc_voltage = (adc_value * ref_voltage) / 1024.0;
    in_voltage = adc_voltage * (R1 + R2) / R2;
    
    Serial.print(in_voltage);
    Serial.println("V");
    
    lcd.setCursor(0, 0);
    lcd.print("Voltage:");
    lcd.setCursor(0, 1);
    lcd.print(in_voltage);
    lcd.print(" V");
  }
  else if (mode == 2) {
    sensorValue = analogRead(sensorPin);
    Vout = (Vin * sensorValue) / 1024;
    R = Rref * (1 / ((Vin / Vout) - 1));
    
    Serial.print("R: ");
    Serial.println(R);
    
    lcd.setCursor(0, 0);
    lcd.print("Resistance:");
    lcd.setCursor(0, 1);
    lcd.print(R);
    lcd.print(" ohm");
  }
  else if (mode == 3) {
    sensorValue = readContinuity();  // use avg, due to noise
    if (sensorValue > 1000) {  // cont. on 1023
      tone(SPEAKER_PIN, 1000);
      lcd.setCursor(0, 0);
      lcd.print("Continuity:");
      lcd.setCursor(0, 1);
      lcd.print("Connected");
    } else {
      noTone(SPEAKER_PIN);
      lcd.setCursor(0, 0);
      lcd.print("Continuity:");
      lcd.setCursor(0, 1);
      lcd.print("Open");
    }
    Serial.print("Continuity value: ");
    Serial.println(sensorValue);
  }
  
  delay(200);
}