#include <Wire.h>
#include <LCD-I2C.h>  // LCD I2C library
#include "EmonLib.h"  // Energy monitoring library
#include <SoftwareSerial.h>  // Software serial for SIM800L

// Define SIM800L TX & RX pins
#define SIM800_TX 1  // Raspberry Pi Pico TX -> SIM800L RX
#define SIM800_RX 0  // Raspberry Pi Pico RX -> SIM800L TX

// Current sensor setup
EnergyMonitor emon1, emon2;
#define SENSOR_PIN1 26  // GP26 (ADC0)
#define SENSOR_PIN2 27  // GP27 (ADC1)
#define CALIBRATION_FACTOR 60  // Calibration factor for sensors

// LCD setup
LCD_I2C lcd(0x27, 16, 2);
SoftwareSerial sim800l(SIM800_TX, SIM800_RX);

void sendCommand(const char *command, int delayTime = 2000) {
    Serial.print("Sending command: ");
    Serial.println(command);

    sim800l.println(command);
    delay(delayTime);

    while (sim800l.available()) {
        Serial.write(sim800l.read());
    }
    Serial.println();
}

void sendSMS(const char* phoneNumber, const char* message) {
    Serial.println("Sending SMS...");
    sendCommand("AT+CMGF=1", 1000);

    sim800l.print("AT+CMGS=\"");
    sim800l.print(phoneNumber);
    sim800l.println("\"");
    delay(1000);

    Serial.println("Writing message...");
    sim800l.print(message);
    delay(1000);

    Serial.println("Sending CTRL+Z...");
    sim800l.write(26);
    delay(5000);
    Serial.println("SMS Sent!");
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    lcd.begin(&Wire);
    lcd.display();
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.print("Initializing...");
    delay(2000);

    // Initialize SIM800L
    sim800l.begin(9600);
    sendCommand("AT");
    sendCommand("AT+CMGF=1");
    sendCommand("AT+CSQ");
    sendCommand("AT+CREG?");
    sendCommand("AT+CMEE=2");

    // Initialize current sensors
    emon1.current(SENSOR_PIN1, CALIBRATION_FACTOR);
    emon2.current(SENSOR_PIN2, CALIBRATION_FACTOR);
}

void loop() {
    double totalCurrent1 = 0.0, totalCurrent2 = 0.0;
    int numReadings = 10;

    for (int i = 0; i < numReadings; i++) {
        totalCurrent1 += emon1.calcIrms(8000);
        totalCurrent2 += emon2.calcIrms(8000);
    }

    double current1 = ((totalCurrent1 / numReadings) - 0.155) / 9;
    double current2 = ((totalCurrent2 / numReadings) - 0.155) / 9;

    if (current1 < 0.08) current1 = 0.00;
    if (current2 < 0.08) current2 = 0.00;

    double difference = current1 - current2;

    Serial.println("----------------------");
    Serial.print("Current Sensor 1: ");
    Serial.print(current1, 3);
    Serial.println(" A");

    Serial.print("Current Sensor 2: ");
    Serial.print(current2, 3);
    Serial.println(" A");

    Serial.print("Difference: ");
    Serial.print(difference, 3);
    Serial.println(" A");
    Serial.println("----------------------");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("C1:");
    lcd.print(current1, 2);
    lcd.print("A ");
    lcd.setCursor(9, 0);
    lcd.print("C2:");
    lcd.print(current2, 2);
    lcd.print("A");

    lcd.setCursor(0, 1);
    lcd.print("Diff:");
    lcd.print(difference, 2);
    lcd.print("A");

    char smsMessage[50];
    snprintf(smsMessage, sizeof(smsMessage), "C1: %.2fA, C2: %.2fA, Diff: %.2fA", current1, current2, difference);
    sendSMS("+250780070415", smsMessage);

    delay(10000);
}