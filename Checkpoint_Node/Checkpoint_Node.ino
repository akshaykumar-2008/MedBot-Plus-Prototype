#include <ESP8266WiFi.h>
#include <espnow.h>

// ==================== CONFIGURATION ====================
// This ESP8266 acts as the checkpoint (signal broadcaster)
// It sends RSSI beacon signals for the robot to locate

// ==================== GLOBAL VARIABLES ====================
uint8_t botMAC[6];  // Will store the bot's MAC address when it connects
bool isBotConnected = false;
unsigned long lastBroadcast = 0;
const unsigned long BROADCAST_INTERVAL = 1000; // Broadcast every 1 second

// ==================== ESP-NOW CALLBACK ====================
void onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  // Store the first device that connects as the bot
  if (!isBotConnected) {
    memcpy(botMAC, mac, 6);
    isBotConnected = true;
    
    Serial.print("[CHECKPOINT] Bot connected! MAC: ");
    printMacAddress(botMAC);
  }
  
  // Display received message
  if (len > 0) {
    char message[len + 1];
    memcpy(message, incomingData, len);
    message[len] = '\0';
    
    Serial.print("[CHECKPOINT] Received from bot: ");
    Serial.println(message);
  }
}

void onDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("[CHECKPOINT] Broadcast status: ");
  Serial.println(sendStatus == 0 ? "Success" : "Fail");
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("=====================================");
  Serial.println("  MedBot+ Checkpoint Node Started");
  Serial.println("=====================================");
  
  // Initialize WiFi in station mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Get and display this device's MAC address
  uint8_t checkpointMAC[6];
  WiFi.macAddress(checkpointMAC);
  Serial.print("[INFO] Checkpoint MAC Address: ");
  printMacAddress(checkpointMAC);
  Serial.println("\n*** Copy this MAC and paste into Bot_Controller.ino ***\n");
  
  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("[ERROR] ESP-NOW initialization failed!");
    return;
  }
  
  // Set roles
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  
  // Register callbacks
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  
  Serial.println("[INIT] ESP-NOW initialized - Waiting for bot connection...");
  Serial.println("[INIT] Broadcasting RSSI beacon every 1 second...");
  
  delay(2000);
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long currentTime = millis();
  
  // Broadcast beacon signal periodically
  if (currentTime - lastBroadcast > BROADCAST_INTERVAL) {
    lastBroadcast = currentTime;
    
    // Create beacon message
    const char *beaconMsg = "CHECKPOINT_BEACON";
    
    // Broadcast to all devices (0xFF:0xFF:0xFF:0xFF:0xFF:0xFF)
    uint8_t broadcastMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_send(broadcastMAC, (uint8_t *)beaconMsg, strlen(beaconMsg));
    
    Serial.print("[BROADCAST] Sent beacon at ");
    Serial.print(currentTime);
    Serial.println(" ms");
  }
  
  // Optional: Send status updates
  if (isBotConnected && currentTime % 5000 == 0) {
    Serial.println("[CHECKPOINT] Bot is connected and navigating...");
  }
  
  delay(100); // Small delay to prevent overwhelming the serial monitor
}

// ==================== HELPER FUNCTIONS ====================

void printMacAddress(uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}
