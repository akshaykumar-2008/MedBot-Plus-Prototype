#ifndef RSSI_NAVIGATION_H
#define RSSI_NAVIGATION_H

#include <Arduino.h>

// ==================== RSSI COMPARISON NAVIGATION ====================
// This system uses RSSI comparison to steer the robot toward the checkpoint

class RSSSteering {
private:
  int lastForwardRSSI = -100;
  int lastLeftRSSI = -100;
  int lastRightRSSI = -100;
  int rssiDifference = 3; // Threshold for direction change (dBm)
  
public:
  // Scan RSSI in current direction and store it
  void recordForwardRSSI(int rssi) {
    lastForwardRSSI = rssi;
  }
  
  void recordLeftRSSI(int rssi) {
    lastLeftRSSI = rssi;
  }
  
  void recordRightRSSI(int rssi) {
    lastRightRSSI = rssi;
  }
  
  // Determine best direction based on RSSI strength
  // Returns: 0=Forward, 1=Turn Left, 2=Turn Right
  int getBestDirection() {
    Serial.print("[RSSI Compare] Forward: ");
    Serial.print(lastForwardRSSI);
    Serial.print(" | Left: ");
    Serial.print(lastLeftRSSI);
    Serial.print(" | Right: ");
    Serial.println(lastRightRSSI);
    
    // Forward is strongest
    if (lastForwardRSSI >= lastLeftRSSI && lastForwardRSSI >= lastRightRSSI) {
      return 0; // Keep going forward
    }
    
    // Left is strongest
    if (lastLeftRSSI > lastForwardRSSI && lastLeftRSSI > lastRightRSSI) {
      return 1; // Turn left
    }
    
    // Right is strongest
    if (lastRightRSSI > lastForwardRSSI && lastRightRSSI > lastLeftRSSI) {
      return 2; // Turn right
    }
    
    // Default to forward if all equal
    return 0;
  }
  
  // Check if significant difference exists to warrant turning
  bool shouldChangeCourse() {
    int maxRSSI = max(max(lastForwardRSSI, lastLeftRSSI), lastRightRSSI);
    return (maxRSSI - lastForwardRSSI) > rssiDifference;
  }
};

#endif // RSSI_NAVIGATION_H
