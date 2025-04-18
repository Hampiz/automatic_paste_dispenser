//Include libraries
#include <Wire.h>
#include <Stepper.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED inställningar
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// IR-sensor
const int sensorDigitalPin = 9;
const int sensorAnalogPin = A0;

// Stegmotor
const int stepsPerRevolution = 2048;
const int motorPin1 = 2;
const int motorPin2 = 3;
const int motorPin3 = 4;
const int motorPin4 = 5;
Stepper myStepper(stepsPerRevolution, motorPin1, motorPin3, motorPin2, motorPin4);

// Piezo & LED
const int piezoPin = 11;
const int ledPin = 12;

// Sensorvariabler
int threshold = 500;
int detectionCount = 0;
int releaseCount = 0;
const int detectionThreshold = 3;
const int releaseThreshold = 5;
const int bufferSize = 5;
int analogBuffer[bufferSize];
int bufferIndex = 0;

bool obstacleDetected = false;
bool motorRunning = false;

void setup() {
  Serial.begin(9600);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED initiering misslyckades"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System starting...");
  display.display();

  pinMode(sensorDigitalPin, INPUT);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
  pinMode(piezoPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  myStepper.setSpeed(10);

  // Init buffer
  for (int i = 0; i < bufferSize; i++) {
    analogBuffer[i] = 1023;
  }

  delay(3000); // Sensor stabilisering

  // Kalibrera sensor
  int baselineSum = 0;
  for (int i = 0; i < 20; i++) {
    baselineSum += analogRead(sensorAnalogPin);
    delay(50);
  }
  int baseline = baselineSum / 20;
  threshold = baseline - 100;
  if (threshold < 100) threshold = 100;

  Serial.print("Tröskelvärde: ");
  Serial.println(threshold);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System ready!");
  display.display();
  delay(2000);
}

void loop() {
  int digitalValue = digitalRead(sensorDigitalPin);
  int analogValue = analogRead(sensorAnalogPin);

  analogBuffer[bufferIndex] = analogValue;
  bufferIndex = (bufferIndex + 1) % bufferSize;

  int analogAverage = 0;
  for (int i = 0; i < bufferSize; i++) {
    analogAverage += analogBuffer[i];
  }
  analogAverage /= bufferSize;

  bool currentDetection = (digitalValue == LOW || analogAverage < threshold);

  if (currentDetection) {
    detectionCount++;
    releaseCount = 0;
  } else {
    releaseCount++;
    detectionCount = 0;
  }

  if (!obstacleDetected && detectionCount >= detectionThreshold) {
    obstacleDetected = true;
    Serial.println("Objekt detekterat!");
  } else if (obstacleDetected && releaseCount >= releaseThreshold) {
    obstacleDetected = false;
    Serial.println("Objekt borta.");
  }

  // --- DISPLAY ---
  display.clearDisplay();
  display.setCursor(0, 0);

  if (obstacleDetected) {
    myStepper.step(stepsPerRevolution / 32);  // Kör motorn
    digitalWrite(ledPin, HIGH);               // Tänd LED
    tone(piezoPin, 1000, 200);                // Spela ton i 200ms
    display.println("Dispensing...");
    motorRunning = true;
  } else {
    if (motorRunning) {
      turnOffStepper();
      digitalWrite(ledPin, LOW);              // Släck LED
      noTone(piezoPin);                       // Stoppa ton
      motorRunning = false;
    }
    display.println("Waiting for object...");
  }

  display.setCursor(0, 30);
  display.print("Detected: ");
  display.println(obstacleDetected ? "YES" : "NO");
  display.display();

  delay(20);
}

void turnOffStepper() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
}