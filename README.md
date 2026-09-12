# MedBot+ Prototype - RSSI-Based Navigation System

## 📋 Project Overview

MedBot+ is an autonomous medical assistant robot that uses RSSI (Received Signal Strength Indicator) from an ESP8266 checkpoint node to navigate towards patients and deliver medication reminders.

This prototype demonstrates:
- **RSSI-based autonomous navigation** between two ESP8266 devices
- **Motor control** for differential drive locomotion
- **Dead reckoning** for return-to-origin functionality
- **IR-based pickup detection** for medication verification
- **Modular C++ architecture** for easy extension

## 🛠️ Hardware Requirements

### Microcontrollers & Communication
- **2x ESP8266** (NodeMCU recommended)
  - Node 1: Robot Controller
  - Node 2: Checkpoint/Beacon Node

### Motors & Chassis
- **1x 2-Wheel Drive Chassis** with front caster wheel
- **2x DC Gear Motors** with encoders (recommended for accurate dead reckoning)
- **1x Motor Driver** (TB6612FNG or L298N)

### Sensors & Peripherals
- **1x IR Proximity/Obstacle Avoidance Sensor** (for medication pickup detection)
- **1x DFPlayer Mini Module** (audio playback - optional for prototype)
- **1x Speaker** 8Ω, 1-3W (for audio alerts)
- **1x MicroSD Card** (for audio files)

### Power
- **2x 18650 Li-ion Batteries** + Battery Holder (~7.4V)
- **1x Buck Converter/Step-down Module** (7.4V → 5V for ESP8266 and sensors)

## 📁 File Structure

```
MedBot-Plus-Prototype/
├── Bot_Controller/
│   ├── Bot_Controller.ino          # Main robot control logic
│   └── navigation.h                # Motor control & RSSI processing
├── Checkpoint_Node/
│   └── Checkpoint_Node.ino         # RSSI beacon broadcaster
└── README.md
```

## 🚀 Quick Start

### Step 1: Flash the Checkpoint Node

1. Open `Checkpoint_Node/Checkpoint_Node.ino` in Arduino IDE
2. Select ESP8266 board and upload
3. Open Serial Monitor (115200 baud)
4. **Note down the MAC address displayed** (format: `AA:BB:CC:DD:EE:FF`)

Example output:
```
[INFO] Checkpoint MAC Address: A4:CF:12:34:56:78
*** Copy this MAC and paste into Bot_Controller.ino ***
```

### Step 2: Configure & Flash Bot Controller

1. Open `Bot_Controller/Bot_Controller.ino`
2. Find this line near the top:
   ```cpp
   uint8_t checkpointMAC[6] = {0xA4, 0xCF, 0x12, 0x34, 0x56, 0x78}; // Replace with actual checkpoint MAC
   ```
3. Replace the MAC address with the one from Step 1. Convert each hex pair:
   - `A4` → `0xA4`
   - `CF` → `0xCF`
   - etc.
4. Select ESP8266 board and upload

### Step 3: Wire the Hardware

#### Motor Driver Connections (TB6612FNG)
```
ESP8266          TB6612FNG
D1 (GPIO5)   →   IN1 (Left Motor)
D2 (GPIO4)   →   IN2 (Left Motor)
D3 (GPIO0)   →   IN1 (Right Motor)
D4 (GPIO2)   →   IN2 (Right Motor)
D5 (GPIO14)  →   PWM Left Motor
D6 (GPIO12)  →   PWM Right Motor
GND          →   GND
```

#### IR Sensor Connection
```
ESP8266          IR Sensor
A0 (Analog)  →   Analog Output
5V           →   VCC
GND          →   GND
```

### Step 4: Power & Test

1. Power on the **Checkpoint Node** first
2. Power on the **Bot Controller**
3. Open Serial Monitor on Bot Controller (115200 baud)
4. You should see:
   ```
   [INIT] Motors initialized
   [INFO] Bot MAC Address: ...
   [ESP-NOW] Checkpoint peer added: A4:CF:12:34:56:78
   [STATE] INITIALIZATION
   [STATE] SEARCHING
   [SEARCH] RSSI from checkpoint: -45 dBm
   [SEARCH->NAVIGATING] Checkpoint signal detected!
   ```

5. The robot should begin moving towards the checkpoint!

## 📊 State Machine

The robot operates through these states:

```
INITIALIZATION
    ↓
SEARCHING (wait for checkpoint signal)
    ↓
NAVIGATING (move towards checkpoint based on RSSI)
    ↓
ARRIVED (reached proximity threshold)
    ↓
WAITING_FOR_PICKUP (detect medication pickup via IR sensor)
    ↓
RETURNING_HOME (use dead reckoning to return to origin)
    ↓
HOME (mission complete)
```

## 🎯 RSSI Navigation Explained

### How It Works

1. **Checkpoint Broadcasting**: The checkpoint node continuously broadcasts ESP-NOW beacon packets
2. **Signal Scanning**: The bot periodically scans WiFi networks to measure RSSI from the checkpoint
3. **Navigation Decision**: 
   - RSSI > -50 dBm: Robot has arrived (close proximity)
   - RSSI < -50 dBm: Continue moving forward
4. **Arrival Detection**: When RSSI crosses threshold, robot stops and enters pickup detection mode

### RSSI Strength Reference
```
RSSI ≈ -30 dBm  → Very close (1-2 meters)
RSSI ≈ -50 dBm  → Close (2-5 meters)
RSSI ≈ -70 dBm  → Moderate distance (5-15 meters)
RSSI ≈ -90 dBm  → Far (20+ meters)
```

You can adjust `RSSI_PROXIMITY_THRESHOLD` in `navigation.h` based on your environment.

## 🔧 Customization

### Adjust Motor Speed
In `navigation.h`:
```cpp
#define MOTOR_SPEED 200  // 0-255 (higher = faster)
```

### Adjust Arrival Threshold
In `navigation.h`:
```cpp
#define RSSI_PROXIMITY_THRESHOLD -50  // Closer/further proximity
```

### Adjust IR Sensor Sensitivity
In `navigation.h`:
```cpp
int irThreshold = 100;  // Adjust based on your sensor
```

### Pin Configuration
All pins are defined at the top of `navigation.h`:
```cpp
#define MOTOR_LEFT_PIN1 D1
#define MOTOR_LEFT_PIN2 D2
// ... etc
```

## 🐛 Troubleshooting

### Robot doesn't move
- Check motor power connections
- Verify PWM pins are correct
- Test motor speed with lower `MOTOR_SPEED` value
- Check battery voltage (should be ~7.4V before buck converter)

### Robot doesn't find checkpoint
- Verify checkpoint MAC address is correct in `Bot_Controller.ino`
- Ensure both ESP8266s are powered and running
- Check Serial Monitor on checkpoint - should show broadcasts
- Move devices closer together
- Reduce `RSSI_CHECK_INTERVAL` to scan more frequently

### Motor control issues
- Verify motor driver connections
- Check if motor driver needs GND connection
- Test motors individually with fixed speed
- Ensure buck converter is outputting stable 5V

### Dead reckoning inaccuracy
- Motor speed must be consistent (check battery voltage)
- Wheel sizes should be similar
- Surface should be flat and smooth
- Future: Add motor encoders for precise step counting

## 📈 Future Enhancements

### Phase 2: Production Ready
- [ ] Weight sensors for pickup verification
- [ ] MQTT & Node.js backend
- [ ] React web dashboard
- [ ] IMU (MPU6050) for corridor navigation
- [ ] Auto-docking with charging
- [ ] DFPlayer Mini integration for audio
- [ ] Motor encoder feedback
- [ ] Obstacle avoidance
- [ ] Multi-checkpoint coordination

### Phase 3: Hospital Deployment
- [ ] Centralized scheduling system
- [ ] Nurse station alerts
- [ ] Real-time tracking
- [ ] Battery management
- [ ] Swarm coordination
- [ ] Cloud logging

## 📝 License

This project is provided as-is for educational and prototyping purposes.

## 🤝 Contributing

Feel free to submit issues, fork, and create pull requests!

---

**Happy Robotics! 🤖**
