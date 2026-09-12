#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <Arduino.h>

// Motor Pins (adjust based on your motor driver)
#define MOTOR_LEFT_PIN1 D1   // GPIO5
#define MOTOR_LEFT_PIN2 D2   // GPIO4
#define MOTOR_RIGHT_PIN1 D3  // GPIO0
#define MOTOR_RIGHT_PIN2 D4  // GPIO2
#define MOTOR_ENABLE_LEFT D5 // GPIO14
#define MOTOR_ENABLE_RIGHT D6 // GPIO12

// Sensor Pins
#define IR_SENSOR_PIN A0     // Analog pin for IR proximity sensor
#define RSSI_CHECK_INTERVAL 500 // ms between RSSI samples

// Navigation Constants
#define MOTOR_SPEED 200      // 0-255 PWM value
#define RSSI_PROXIMITY_THRESHOLD -50  // RSSI value indicating arrival (~1-2 meters)
#define RSSI_SAMPLE_SIZE 5   // Number of RSSI samples to average
#define MAX_DISTANCE_STEPS 10000 // Max motor steps to record

// Dead Reckoning Structure
struct DeadReckoningData {
  int leftSteps = 0;
  int rightSteps = 0;
  bool isNavigating = false;
};

class NavigationController {
private:
  int rssiReadings[RSSI_SAMPLE_SIZE] = {0};
  int rssiIndex = 0;
  int lastRSSI = -100;
  int irThreshold = 100;  // IR sensor threshold (adjust based on your sensor)
  
public:
  DeadReckoningData deadReckoning;
  
  // Motor Control Functions
  void initMotors() {
    pinMode(MOTOR_LEFT_PIN1, OUTPUT);
    pinMode(MOTOR_LEFT_PIN2, OUTPUT);
    pinMode(MOTOR_RIGHT_PIN1, OUTPUT);
    pinMode(MOTOR_RIGHT_PIN2, OUTPUT);
    pinMode(MOTOR_ENABLE_LEFT, OUTPUT);
    pinMode(MOTOR_ENABLE_RIGHT, OUTPUT);
    
    // Initialize all pins to LOW
    digitalWrite(MOTOR_LEFT_PIN1, LOW);
    digitalWrite(MOTOR_LEFT_PIN2, LOW);
    digitalWrite(MOTOR_RIGHT_PIN1, LOW);
    digitalWrite(MOTOR_RIGHT_PIN2, LOW);
    analogWrite(MOTOR_ENABLE_LEFT, 0);
    analogWrite(MOTOR_ENABLE_RIGHT, 0);
  }
  
  // Move Forward
  void moveForward(int speed = MOTOR_SPEED) {
    digitalWrite(MOTOR_LEFT_PIN1, HIGH);
    digitalWrite(MOTOR_LEFT_PIN2, LOW);
    digitalWrite(MOTOR_RIGHT_PIN1, HIGH);
    digitalWrite(MOTOR_RIGHT_PIN2, LOW);
    analogWrite(MOTOR_ENABLE_LEFT, speed);
    analogWrite(MOTOR_ENABLE_RIGHT, speed);
  }
  
  // Move Backward
  void moveBackward(int speed = MOTOR_SPEED) {
    digitalWrite(MOTOR_LEFT_PIN1, LOW);
    digitalWrite(MOTOR_LEFT_PIN2, HIGH);
    digitalWrite(MOTOR_RIGHT_PIN1, LOW);
    digitalWrite(MOTOR_RIGHT_PIN2, HIGH);
    analogWrite(MOTOR_ENABLE_LEFT, speed);
    analogWrite(MOTOR_ENABLE_RIGHT, speed);
  }
  
  // Turn Left (in place)
  void turnLeft(int speed = MOTOR_SPEED) {
    digitalWrite(MOTOR_LEFT_PIN1, LOW);
    digitalWrite(MOTOR_LEFT_PIN2, HIGH);
    digitalWrite(MOTOR_RIGHT_PIN1, HIGH);
    digitalWrite(MOTOR_RIGHT_PIN2, LOW);
    analogWrite(MOTOR_ENABLE_LEFT, speed);
    analogWrite(MOTOR_ENABLE_RIGHT, speed);
  }
  
  // Turn Right (in place)
  void turnRight(int speed = MOTOR_SPEED) {
    digitalWrite(MOTOR_LEFT_PIN1, HIGH);
    digitalWrite(MOTOR_LEFT_PIN2, LOW);
    digitalWrite(MOTOR_RIGHT_PIN1, LOW);
    digitalWrite(MOTOR_RIGHT_PIN2, HIGH);
    analogWrite(MOTOR_ENABLE_LEFT, speed);
    analogWrite(MOTOR_ENABLE_RIGHT, speed);
  }
  
  // Stop
  void stop() {
    digitalWrite(MOTOR_LEFT_PIN1, LOW);
    digitalWrite(MOTOR_LEFT_PIN2, LOW);
    digitalWrite(MOTOR_RIGHT_PIN1, LOW);
    digitalWrite(MOTOR_RIGHT_PIN2, LOW);
    analogWrite(MOTOR_ENABLE_LEFT, 0);
    analogWrite(MOTOR_ENABLE_RIGHT, 0);
  }
  
  // RSSI Processing Functions
  void addRSSIReading(int rssi) {
    rssiReadings[rssiIndex] = rssi;
    rssiIndex = (rssiIndex + 1) % RSSI_SAMPLE_SIZE;
  }
  
  int getAverageRSSI() {
    int sum = 0;
    for (int i = 0; i < RSSI_SAMPLE_SIZE; i++) {
      sum += rssiReadings[i];
    }
    return sum / RSSI_SAMPLE_SIZE;
  }
  
  // RSSI-based Navigation Decision
  // Returns: 1=Forward, 0=Stop/Arrived
  int getNavigationDirection(int currentRSSI) {
    lastRSSI = currentRSSI;
    
    // If RSSI is strong enough, we've arrived
    if (currentRSSI > RSSI_PROXIMITY_THRESHOLD) {
      return 0; // Stop - arrived at target
    }
    
    // Otherwise, continue forward
    return 1; // Move forward
  }
  
  // IR Sensor - Detect Medication Pickup
  bool isMedicinePickedUp() {
    int irValue = analogRead(IR_SENSOR_PIN);
    return (irValue < irThreshold); // Adjust logic based on your sensor behavior
  }
  
  // Get last recorded RSSI
  int getLastRSSI() {
    return lastRSSI;
  }
  
  // Dead Reckoning - Record steps
  void recordMotorStep(bool isLeftMotor) {
    if (isLeftMotor) {
      deadReckoning.leftSteps++;
    } else {
      deadReckoning.rightSteps++;
    }
  }
  
  // Return to origin using dead reckoning
  void returnToOrigin() {
    stop();
    delay(500);
    
    // Reverse the recorded steps
    moveBackward();
    delay((deadReckoning.leftSteps + deadReckoning.rightSteps) * 10); // Rough timing
    stop();
    
    // Reset dead reckoning
    deadReckoning.leftSteps = 0;
    deadReckoning.rightSteps = 0;
  }
};

#endif // NAVIGATION_H
