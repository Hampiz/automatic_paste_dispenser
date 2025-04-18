// Include libraries
#include <Wire.h>
#include <Stepper.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display settings
#define SCREEN_WIDTH 128              // OLED display width in pixels
#define SCREEN_HEIGHT 64              // OLED display height in pixels
#define OLED_RESET -1                 // No reset pin used
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// IR sensor pins
const int sensorDigitalPin = 9;       // Digital output from IR sensor
const int sensorAnalogPin = A0;       // Analog output from IR sensor

// Stepper motor configuration
const int stepsPerRevolution = 2048;  // Steps per full revolution
const int motorPin1 = 2;
const int motorPin2 = 3;
const int motorPin3 = 4;
const int motorPin4 = 5;
Stepper myStepper(stepsPerRevolution, motorPin1, motorPin3, motorPin2, motorPin4); // Initialize stepper motor

// Piezo buzzer and LED
const int piezoPin = 11;              // Pin for piezo buzzer
const int ledPin = 12;                // Pin for LED

// Sensor-related variables
int threshold = 500;                  // Initial threshold value
int detectionCount = 0;              // Counter for object detection
int releaseCount = 0;                // Counter for object absence
const int detectionThreshold = 3;    // Number of detections to confirm presence
const int releaseThreshold = 5;      // Number of releases to confirm absence
const int bufferSize = 5;            // Size of moving average buffer
int analogBuffer[bufferSize];        // Buffer to hold recent analog values
int bufferIndex = 0;                 // Index for circular buffer

bool obstacleDetected = false;       // Current obstacle detection status
bool motorRunning = false;           // Whether the motor is currently running

void setup() {
  Serial.begin(9600);                // Start serial communication

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED initialization failed"));
    while (true);                    // Halt program if display fails
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System starting...");
  display.display();

  // Set pin modes
  pinMode(sensorDigitalPin, INPUT);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
  pinMode(piezoPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  myStepper.setSpeed(10);           // Set motor speed

  // Initialize buffer with high values
  for (int i = 0; i < bufferSize; i++) {
    analogBuffer[i] = 1023;
  }

  delay(3000);                       // Wait for sensor stabilization

  // Calibrate sensor: take average value and calculate threshold
  int baselineSum = 0;
  for (int i = 0; i < 20; i++) {
    baselineSum += analogRead(sensorAnalogPin);
    delay(50);
  }
  int baseline = baselineSum / 20;
  threshold = baseline - 100;
  if (threshold < 100) threshold = 100; // Avoid too low threshold

  Serial.print("Threshold: ");
  Serial.println(threshold);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System ready!");
  display.display();
  delay(2000);
}

void loop() {
  // Read values from the IR sensor
  int digitalValue = digitalRead(sensorDigitalPin);
  int analogValue = analogRead(sensorAnalogPin);

  // Update moving average buffer
  analogBuffer[bufferIndex] = analogValue;
  bufferIndex = (bufferIndex + 1) % bufferSize;

  // Calculate average of recent analog values
  int analogAverage = 0;
  for (int i = 0; i < bufferSize; i++) {
    analogAverage += analogBuffer[i];
  }
  analogAverage /= bufferSize;

  // Determine if object is currently detected
  bool currentDetection = (digitalValue == LOW || analogAverage < threshold);

  // Update detection/release counters
  if (currentDetection) {
    detectionCount++;
    releaseCount = 0;
  } else {
    releaseCount++;
    detectionCount = 0;
  }

  // Object detected
  if (!obstacleDetected && detectionCount >= detectionThreshold) {
    obstacleDetected = true;
    Serial.println("Object detected!");
  } 
  // Object removed
  else if (obstacleDetected && releaseCount >= releaseThreshold) {
    obstacleDetected = false;
    Serial.println("Object gone.");
  }

  // --- OLED DISPLAY ---
  display.clearDisplay();
  display.setCursor(0, 0);

  if (obstacleDetected) {
    myStepper.step(stepsPerRevolution / 32);  // Run the motor
    digitalWrite(ledPin, HIGH);               // Turn on LED
    tone(piezoPin, 1000, 200);                // Play tone for 200ms
    display.println("Dispensing...");         // Show status
    motorRunning = true;
  } else {
    if (motorRunning) {
      turnOffStepper();                       // Stop motor
      digitalWrite(ledPin, LOW);              // Turn off LED
      noTone(piezoPin);                       // Stop tone
      motorRunning = false;
    }
    display.println("Waiting for object..."); // Show standby
  }

  // Show detection status
  display.setCursor(0, 30);
  display.print("Detected: ");
  display.println(obstacleDetected ? "YES" : "NO");
  display.display();

  delay(20);  // Small delay for stability
}

// Turns off all motor pins
void turnOffStepper() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
}