# MedBot+ RSSI Directional Navigation Guide

## 🎯 How Your Robot Knows Which Direction to Go

Your robot uses a **smart RSSI comparison steering system** to determine the best direction toward the checkpoint. Here's exactly how it works:

---

## 📡 The Problem: RSSI Alone Doesn't Tell Direction

RSSI (signal strength) tells you **how far away** the signal is, but NOT **which direction** it's coming from.

```
Without directional info:
┌─────────────────────────────────┐
│  Robot at center                │
│  RSSI = -60 dBm (signal detected)│
│  Question: Which way to go? 🤔  │
│  ↑ ↓ ← → ???                     │
└─────────────────────────────────┘
```

## ✅ The Solution: Compare RSSI from Multiple Directions

The robot:
1. **Stops and measures** RSSI in the current forward direction
2. **Turns left 45°** and measures RSSI again
3. **Turns right 90°** (now facing right from original) and measures again
4. **Compares all three readings** and picks the direction with strongest signal
5. **Turns toward that direction** and continues moving

```
Steering Cycle:

Step 1: Forward      Step 2: Left         Step 3: Right        Step 4: Compare
┌────────────┐      ┌────────────┐      ┌────────────┐       RSSI Readings:
│    🔵→     │      │   →→🔵     │      │     🔵←    │       Forward: -55 dBm
│ RSSI: -55  │      │ RSSI: -60  │      │ RSSI: -50  │  →    Right:   -50 dBm ✓ BEST
│    dBm     │      │    dBm     │      │    dBm     │       Left:    -60 dBm
└────────────┘      └────────────┘      └────────────┘
                                                              Turn RIGHT!
```

---

## 🔄 Step-by-Step Navigation Process

### Cycle Overview (Repeats Every 3 Seconds)

```
START CYCLE
    ↓
[1] STOP Robot
    ↓
[2] SCAN FORWARD
    - Robot measures RSSI while facing forward
    - Records: Forward RSSI = -55 dBm
    ↓
[3] TURN & SCAN LEFT
    - Robot turns 45° left
    - Measures RSSI from this angle
    - Records: Left RSSI = -60 dBm
    ↓
[4] TURN & SCAN RIGHT
    - Robot turns another 45° right (now 90° from original)
    - Measures RSSI from this angle
    - Records: Right RSSI = -50 dBm
    ↓
[5] DECISION
    - Compare: -55 vs -60 vs -50
    - Winner: Right (-50 dBm) ← STRONGEST
    ↓
[6] EXECUTE
    - Turn right
    - Move forward
    ↓
[7] CHECK ARRIVAL
    - Is average RSSI > threshold?
    - NO → Go back to step 1 (repeat cycle)
    - YES → Robot has arrived!
```

---

## 📊 RSSI Comparison Logic

```cpp
Forward RSSI: -55 dBm
Left RSSI:    -60 dBm
Right RSSI:   -50 dBm

Maximum = -50 dBm (Right direction)

Decision: TURN RIGHT because it has the strongest signal!
```

### Code Logic
```cpp
// Determine best direction based on RSSI strength
// Returns: 0=Forward, 1=Turn Left, 2=Turn Right

int getBestDirection() {
    // Forward is strongest
    if (lastForwardRSSI >= lastLeftRSSI && lastForwardRSSI >= lastRightRSSI) {
        return 0;  // Keep going forward
    }
    
    // Left is strongest
    if (lastLeftRSSI > lastForwardRSSI && lastLeftRSSI > lastRightRSSI) {
        return 1;  // Turn left
    }
    
    // Right is strongest
    if (lastRightRSSI > lastForwardRSSI && lastRightRSSI > lastLeftRSSI) {
        return 2;  // Turn right
    }
    
    return 0;  // Default to forward
}
```

---

## 🎮 Real-World Navigation Example

### Scenario: Robot needs to find checkpoint

```
Initial State:
┌─────────────────────────────────────┐
│                                     │
│     Checkpoint (RSSI source)        │
│            📍                       │
│                                     │
│         (10 meters away)            │
│                                     │
│                                     │
│                    🤖 Robot         │
│                                     │
└─────────────────────────────────────┘

Cycle 1:
- Forward RSSI: -70 dBm
- Left RSSI:    -72 dBm
- Right RSSI:   -65 dBm ✓ BEST
→ Turn right & move forward

Cycle 2:
- Forward RSSI: -63 dBm ✓ BEST
- Left RSSI:    -68 dBm
- Right RSSI:   -66 dBm
→ Continue straight

Cycle 3:
- Forward RSSI: -58 dBm ✓ BEST
- Left RSSI:    -65 dBm
- Right RSSI:   -62 dBm
→ Continue straight

... (repeat)

Final Cycle:
- Forward RSSI: -45 dBm (within threshold!)
→ ARRIVED! Stop moving
```

---

## 🔧 Configuration Parameters

### In `Bot_Controller.ino`:

```cpp
// How often to run a steering cycle (milliseconds)
const unsigned long STEERING_INTERVAL = 3000;  // Every 3 seconds

// How long to turn when changing direction
const unsigned long TURN_DURATION = 500;  // 500ms turn

// How long to move forward between checks
const unsigned long FORWARD_DURATION = 2500;  // 2.5 seconds
```

### In `navigation.h`:

```cpp
// Signal threshold for arrival
#define RSSI_PROXIMITY_THRESHOLD -50  // dBm

// How many RSSI samples to average
#define RSSI_SAMPLE_SIZE 5

// Motor speed (0-255)
#define MOTOR_SPEED 200
```

---

## 📈 Two Navigation Modes

### Mode 1: SIMPLE_FORWARD (Basic)
- Just move forward continuously
- Check RSSI every 500ms
- If threshold crossed, stop
- **Pros:** Simple, fast, less power
- **Cons:** Might not find target if it's off to the side

### Mode 2: RSSI_STEERING (Smart) ⭐ RECOMMENDED
- Periodically check all three directions
- Always turn toward strongest signal
- More accurate navigation
- **Pros:** Much better accuracy, handles curved paths
- **Cons:** Slightly more complex, uses more power

**To switch modes**, change this line in `Bot_Controller.ino`:
```cpp
NavigationMode navMode = RSSI_STEERING;  // Change to SIMPLE_FORWARD for basic mode
```

---

## 📊 Serial Monitor Output Example

When running with RSSI steering enabled, you'll see:

```
[STATE] INITIALIZATION
[STATE] SEARCHING
[SEARCH] RSSI from checkpoint: -45 dBm
[SEARCH->NAVIGATING] Checkpoint signal detected!
[NAVIGATE] Starting directional RSSI steering...

[STEERING CYCLE #0]
  1. Measuring FORWARD direction...
     RSSI: -55 dBm
  2. Measuring LEFT direction (turning 45°)...
     RSSI: -62 dBm
  3. Measuring RIGHT direction (turning 90° from original)...
     RSSI: -48 dBm
[RSSI Compare] Forward: -55 | Left: -62 | Right: -48
[DECISION] Best direction: RIGHT
[PROGRESS] Average RSSI: -52 dBm | Threshold: -50

[STEERING CYCLE #1]
  1. Measuring FORWARD direction...
     RSSI: -50 dBm
  2. Measuring LEFT direction (turning 45°)...
     RSSI: -58 dBm
  3. Measuring RIGHT direction (turning 90° from original)...
     RSSI: -54 dBm
[RSSI Compare] Forward: -50 | Left: -58 | Right: -54
[DECISION] Best direction: FORWARD
[PROGRESS] Average RSSI: -51 dBm | Threshold: -50

[NAVIGATE->ARRIVED] Proximity threshold reached!

[STATE] ARRIVED at checkpoint!
```

---

## 🎯 Algorithm Flowchart

```
START NAVIGATION
        ↓
   ┌────────────────┐
   │ Every 3 Seconds│ (STEERING_INTERVAL)
   └────────┬───────┘
            ↓
      STOP Motors
            ↓
   Scan FORWARD RSSI
            ↓
   Turn LEFT 45°
   Scan LEFT RSSI
            ↓
   Turn RIGHT 90°
   Scan RIGHT RSSI
            ↓
   Compare All 3 Readings
            ↓
      ┌─────┴─────┬──────────┬──────────┐
      ↓           ↓          ↓          ↓
   Forward    Left wins  Right wins  All Equal?
   is Best?   (return 1) (return 2)   (use Forward)
      ↓
   ┌──┴──────────────────────────┐
   ↓                             ↓
Execute Direction      Check RSSI > Threshold?
(Turn if needed)        ↓           ↓
   ↓              YES (Arrived)   NO (Keep Going)
   ↓                  ↓           ↓
Move FORWARD     STOP Robot    Loop Back
   ↓             TRANSITION
Record Steps        to ARRIVED
   ↓
Loop until arrival
```

---

## 🐛 Troubleshooting Navigation

### Robot goes in circles
- **Cause:** RSSI readings are too similar in all directions
- **Fix:** Reduce `RSSI_PROXIMITY_THRESHOLD` to detect arrival sooner
- **Fix:** Increase `STEERING_INTERVAL` to make fewer direction changes

### Robot overshoots and passes checkpoint
- **Cause:** Threshold is too low (RSSI too weak at "arrival")
- **Fix:** Increase `RSSI_PROXIMITY_THRESHOLD` (e.g., from -50 to -45)

### Robot takes wrong direction
- **Cause:** Noise/interference giving false RSSI readings
- **Fix:** Average more samples: increase `RSSI_SAMPLE_SIZE` from 5 to 10
- **Fix:** Increase `STEERING_INTERVAL` between direction checks

### Robot moves too slow/fast
- **Cause:** Motor speed setting
- **Fix:** Adjust `MOTOR_SPEED` in `navigation.h` (0-255)

### Robot detection range too short
- **Cause:** Checkpoint broadcast power too low
- **Fix:** Check checkpoint node is properly powered
- **Fix:** Adjust antenna orientation on ESP8266

---

## 📚 Key Concepts

### RSSI (Received Signal Strength Indicator)
- Measured in dBm (decibels relative to 1 milliwatt)
- More negative = weaker signal (farther away)
- Less negative = stronger signal (closer)

### Dead Reckoning
- Robot records motor steps during navigation
- Uses this to reverse path and return home
- Not perfectly accurate (drift accumulates)

### Proximity Threshold
- RSSI value that indicates "we've arrived"
- Must be tuned for your environment
- Different buildings have different signal propagation

---

## 🚀 Next Steps for Improvement

1. **Add IMU (MPU6050)** - Track actual heading, not just RSSI
2. **Motor Encoders** - More accurate step counting for dead reckoning
3. **Kalman Filter** - Smooth noisy RSSI readings
4. **Obstacle Avoidance** - Detect walls/objects and navigate around
5. **Multi-checkpoint** - Navigate to different locations
6. **Web Dashboard** - Monitor robot path in real-time

---

**Your robot is now smart enough to find its way! 🤖✨**
