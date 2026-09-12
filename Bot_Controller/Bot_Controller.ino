#include <ESP8266WiFi.h>
#include <espnow.h>
#include "navigation.h"
#include "rssi_steering.h"

// ==================== CONFIGURATION ====================
// Checkpoint (Receiver) MAC Address - CHANGE THIS to your checkpoint's MAC
uint8_t checkpointMAC[6] = {0xA4, 0xCF, 0x12, 0x34, 0x56, 0x78}; // Replace with actual checkpoint MAC

// Robot states
enum RobotState {
  INITIALIZATION,
  SEARCHING,
  NAVIGATING,
  ARRIVED,
  WAITING_FOR_PICKUP,
  RETURNING_HOME,
  HOME
};

// Navigation modes
enum NavigationMode {
  SIMPLE_FORWARD,      // Just move forward (basic)
  RSSI_STEERING        // Compare RSSI from different directions (smart)
};

// ==================== GLOBAL VARIABLES ====================
NavigationController navController;
RSSSteering steering;
RobotState currentState = INITIALIZATION;
NavigationMode navMode = RSSI_STEERING; // Set to RSSI_STEERING for smart navigation
unsigned long lastRSSICheck = 0;
unsigned long lastSteering = 0;
int currentRSSI = -100;
unsigned long pickupWaitStart = 0;
const unsigned long PICKUP_TIMEOUT = 5 * 60 * 1000; // 5 minutes in milliseconds
int reminderCount = 0;
const int MAX_REMINDERS = 3;
int steeringCycleCount = 0;

// Steering parameters
const unsigned long STEERING_INTERVAL = 3000; // Check direction every 3 seconds
const unsigned long TURN_DURATION = 500; // How long to turn when changing direction
const unsigned long FORWARD_DURATION = 2500; // How long to move forward between checks

// ==================== ESP-NOW CALLBACK ====================
void onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  if (len > 0) {
    char message[len + 1];
    memcpy(message, incomingData, len);
    message[len] = '\0';
    
    Serial.print("[ESP-NOW] Message from checkpoint: ");
    Serial.println(message);
  }
}

void onDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("[ESP-NOW] Send status: ");
  Serial.println(sendStatus == 0 ? "Success" : "Fail");
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("=====================================");
  Serial.println("  MedBot+ Controller Started");
  Serial.println("  Navigation Mode: RSSI Steering");
  Serial.println("=====================================");
  
  // Initialize motors
  navController.initMotors();
  Serial.println("[INIT] Motors initialized");
  
  // Initialize WiFi in station mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Get and display this device's MAC address
  uint8_t botMAC[6];
  WiFi.macAddress(botMAC);
  Serial.print("[INFO] Bot MAC Address: ");
  printMacAddress(botMAC);
  
  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("[ERROR] ESP-NOW initialization failed!");
    return;
  }
  
  // Set roles
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  
  // Register send and receive callbacks
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  
  // Add checkpoint as peer
  esp_now_add_peer(checkpointMAC, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  Serial.print("[ESP-NOW] Checkpoint peer added: ");
  printMacAddress(checkpointMAC);
  
  Serial.println("[INIT] Setup complete - Starting in 3 seconds...");
  delay(3000);
  
  currentState = INITIALIZATION;
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long currentTime = millis();
  
  // State Machine
  switch (currentState) {
    case INITIALIZATION:
      handleInitialization();
      break;
      
    case SEARCHING:
      handleSearching(currentTime);
      break;
      
    case NAVIGATING:
      if (navMode == RSSI_STEERING) {
        handleNavigatingWithSteering(currentTime);
      } else {
        handleNavigating(currentTime);
      }
      break;
      
    case ARRIVED:
      handleArrived();
      break;
      
    case WAITING_FOR_PICKUP:
      handleWaitingForPickup(currentTime);
      break;
      
    case RETURNING_HOME:
      handleReturningHome();
      break;
      
    case HOME:
      handleHome();
      break;
  }
}

// ==================== STATE HANDLERS ====================

void handleInitialization() {
  Serial.println("\n[STATE] INITIALIZATION");
  Serial.println("  - Bot positioned at origin");
  Serial.println("  - Dead reckoning started");
  Serial.println("  - Transitioning to SEARCHING...");
  
  delay(2000);
  currentState = SEARCHING;
  navController.deadReckoning.leftSteps = 0;
  navController.deadReckoning.rightSteps = 0;
}

void handleSearching(unsigned long currentTime) {
  // Periodically scan for checkpoint signal
  if (currentTime - lastRSSICheck > RSSI_CHECK_INTERVAL) {
    lastRSSICheck = currentTime;
    
    // Scan networks to get RSSI from checkpoint
    int numNetworks = WiFi.scanNetworks(false, true);
    
    for (int i = 0; i < numNetworks; i++) {
      // Check if this is our checkpoint (by BSSID/MAC)
      uint8_t *scannedMAC = WiFi.BSSID(i);
      if (isSameMac(scannedMAC, checkpointMAC)) {
        currentRSSI = WiFi.RSSI(i);
        navController.addRSSIReading(currentRSSI);
        
        Serial.print("[SEARCH] RSSI from checkpoint: ");
        Serial.print(currentRSSI);
        Serial.println(" dBm");
        
        // Check if we should transition to NAVIGATING
        if (currentRSSI > -85) { // Checkpoint detected
          Serial.println("[SEARCH->NAVIGATING] Checkpoint signal detected!");
          Serial.println("[NAVIGATE] Starting directional RSSI steering...");
          currentState = NAVIGATING;
          steeringCycleCount = 0;
          lastSteering = currentTime;
          navController.moveForward();
          return;
        }
      }
    }
    
    WiFi.scanDelete();
  }
}

// ==================== SMART NAVIGATION WITH STEERING ====================

void handleNavigatingWithSteering(unsigned long currentTime) {
  // Every STEERING_INTERVAL, check which direction has strongest RSSI
  if (currentTime - lastSteering > STEERING_INTERVAL) {
    lastSteering = currentTime;
    
    Serial.print("\n[STEERING CYCLE #");
    Serial.print(steeringCycleCount++);
    Serial.println("]");
    
    navController.stop();
    delay(200);
    
    // Step 1: Scan forward (straight)
    Serial.println("  1. Measuring FORWARD direction...");
    int forwardRSSI = scanRSSI();
    steering.recordForwardRSSI(forwardRSSI);
    navController.stop();
    delay(200);
    
    // Step 2: Scan left
    Serial.println("  2. Measuring LEFT direction (turning 45°)...");
    navController.turnLeft(150);
    delay(500);
    int leftRSSI = scanRSSI();
    steering.recordLeftRSSI(leftRSSI);
    navController.stop();
    delay(200);
    
    // Step 3: Scan right
    Serial.println("  3. Measuring RIGHT direction (turning 90° from original)...");
    navController.turnRight(150);
    delay(1000);
    int rightRSSI = scanRSSI();
    steering.recordRightRSSI(rightRSSI);
    navController.stop();
    delay(200);
    
    // Step 4: Determine best direction
    int bestDirection = steering.getBestDirection();
    
    Serial.print("[DECISION] Best direction: ");
    if (bestDirection == 0) {
      Serial.println("FORWARD");
      navController.moveForward();
    } else if (bestDirection == 1) {
      Serial.println("LEFT");
      navController.turnLeft(150);
      delay(TURN_DURATION);
      navController.moveForward();
    } else if (bestDirection == 2) {
      Serial.println("RIGHT");
      navController.turnRight(150);
      delay(TURN_DURATION);
      navController.moveForward();
    }
    
    // Check if we've arrived
    int avgRSSI = navController.getAverageRSSI();
    Serial.print("[PROGRESS] Average RSSI: ");
    Serial.print(avgRSSI);
    Serial.print(" dBm | Threshold: ");
    Serial.println(RSSI_PROXIMITY_THRESHOLD);
    
    if (avgRSSI > RSSI_PROXIMITY_THRESHOLD) {
      Serial.println("[NAVIGATE->ARRIVED] Proximity threshold reached!");
      navController.stop();
      currentState = ARRIVED;
      return;
    }
  }
}

// Simple RSSI scan function
int scanRSSI() {
  int numNetworks = WiFi.scanNetworks(false, true);
  int rssi = -100;
  
  for (int i = 0; i < numNetworks; i++) {
    uint8_t *scannedMAC = WiFi.BSSID(i);
    if (isSameMac(scannedMAC, checkpointMAC)) {
      rssi = WiFi.RSSI(i);
      navController.addRSSIReading(rssi);
      break;
    }
  }
  
  WiFi.scanDelete();
  Serial.print("     RSSI: ");
  Serial.print(rssi);
  Serial.println(" dBm");
  
  return rssi;
}

// ==================== BASIC NAVIGATION (Fallback) ====================

void handleNavigating(unsigned long currentTime) {
  // Continuously scan for RSSI and navigate towards it
  if (millis() - lastRSSICheck > RSSI_CHECK_INTERVAL) {
    lastRSSICheck = millis();
    
    int numNetworks = WiFi.scanNetworks(false, true);
    
    for (int i = 0; i < numNetworks; i++) {
      uint8_t *scannedMAC = WiFi.BSSID(i);
      if (isSameMac(scannedMAC, checkpointMAC)) {
        currentRSSI = WiFi.RSSI(i);
        navController.addRSSIReading(currentRSSI);
        int avgRSSI = navController.getAverageRSSI();
        
        Serial.print("[NAVIGATE] RSSI: ");
        Serial.print(currentRSSI);
        Serial.print(" dBm | Avg: ");
        Serial.print(avgRSSI);
        Serial.println(" dBm");
        
        // Check if we've arrived
        if (currentRSSI > RSSI_PROXIMITY_THRESHOLD) {
          Serial.println("[NAVIGATE->ARRIVED] Proximity threshold reached!");
          navController.stop();
          currentState = ARRIVED;
          WiFi.scanDelete();
          return;
        }
        
        // Continue moving forward
        navController.moveForward();
        navController.recordMotorStep(true);
        navController.recordMotorStep(false);
      }
    }
    
    WiFi.scanDelete();
  }
}

// ==================== ARRIVAL & PICKUP ====================

void handleArrived() {
  Serial.println("\n[STATE] ARRIVED at checkpoint!");
  Serial.println("  - Motors stopped");
  Serial.println("  - Playing greeting audio");
  Serial.println("  - Waiting for medication pickup...");
  
  navController.stop();
  
  // TODO: Play audio greeting here
  // playAudio(1); // Play "Hello, it is time for your medicine"
  
  pickupWaitStart = millis();
  reminderCount = 0;
  currentState = WAITING_FOR_PICKUP;
  
  delay(2000);
}

void handleWaitingForPickup(unsigned long currentTime) {
  // Check if medicine was picked up
  if (navController.isMedicinePickedUp()) {
    Serial.println("[PICKUP] Medicine picked up! Transitioning to RETURNING_HOME");
    
    // TODO: Play success audio
    // playAudio(3); // Play "Thank you, returning to station"
    
    currentState = RETURNING_HOME;
    return;
  }
  
  // Check for timeout and replay reminder
  unsigned long elapsedTime = currentTime - pickupWaitStart;
  unsigned long reminderInterval = 60 * 1000; // 1 minute between reminders
  
  if (elapsedTime > reminderInterval * (reminderCount + 1) && reminderCount < MAX_REMINDERS) {
    reminderCount++;
    Serial.print("[PICKUP] Reminder #");
    Serial.println(reminderCount);
    
    // TODO: Replay audio reminder
    // playAudio(2); // Play reminder tone
  }
  
  // After 5 minutes, force return home
  if (elapsedTime > PICKUP_TIMEOUT) {
    Serial.println("[PICKUP] Timeout reached. Returning home without pickup confirmation.");
    currentState = RETURNING_HOME;
    return;
  }
}

void handleReturningHome() {
  Serial.println("\n[STATE] RETURNING HOME");
  Serial.print("  - Steps recorded: Left=");
  Serial.print(navController.deadReckoning.leftSteps);
  Serial.print(", Right=");
  Serial.println(navController.deadReckoning.rightSteps);
  
  // Use dead reckoning to return
  navController.returnToOrigin();
  
  Serial.println("  - Returned to origin");
  currentState = HOME;
  
  delay(2000);
}

void handleHome() {
  Serial.println("\n[STATE] HOME - Mission Complete!");
  Serial.println("  - Robot at rest at origin point");
  Serial.println("  - Waiting for next mission...");
  
  // Can add logic here to wait for next signal or loop back to INITIALIZATION
  delay(5000);
  
  // Reset and go back to search
  currentState = INITIALIZATION;
}

// ==================== HELPER FUNCTIONS ====================

bool isSameMac(uint8_t *mac1, uint8_t *mac2) {
  for (int i = 0; i < 6; i++) {
    if (mac1[i] != mac2[i]) {
      return false;
    }
  }
  return true;
}

void printMacAddress(uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}
