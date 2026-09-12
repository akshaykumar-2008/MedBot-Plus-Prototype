# MedBot+ Hardware Wiring Guide

## 🔌 Complete Wiring Diagram

### Overview
You have **2 ESP8266 microcontrollers**:
1. **Bot Controller** - Controls motors, sensors, on the robot
2. **Checkpoint Node** - Broadcasts RSSI signal from patient location (stationary)

---

## 📍 CHECKPOINT NODE (Stationary)

The checkpoint node is **very simple** - it only needs power!

```
┌─────────────────────────────────────────┐
│      CHECKPOINT NODE (ESP8266)          │
│      (Placed near patient)              │
└─────────────────────────────────────────┘
         │
         │ USB Power (5V) or Battery
         ▼
    ┌─────────────┐
    │ Buck Convert│
    │  7.4V → 5V │
    └──────┬──────┘
           │
           ▼ 5V, GND
    ┌─────────────────────┐
    │    ESP8266          │
    │ (NodeMCU/D1 Mini)   │
    │                     │
    │ VCC ────────── 5V   │
    │ GND ────────── GND  │
    └─────────────────────┘

** That's it! The checkpoint just needs power.
** It broadcasts WiFi signals automatically.
```

**Checkpoint Connections:**
- **VCC** → 5V (from buck converter)
- **GND** → GND (from buck converter)
- **That's ALL!**

---

## 🤖 BOT CONTROLLER (On the Robot)

The bot controller is more complex - it controls motors and reads sensors.

### Complete Pin Mapping

```
┌─────────────────────────────────────────────────────────────────┐
│              BOT CONTROLLER PINOUT (ESP8266)                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  POWER PINS:                                                    │
│  ├─ VCC (3.3V) ──────→ 3.3V Rail                               │
│  ├─ GND (Multiple) ──→ GND Rail (Ground)                       │
│                                                                 │
│  MOTOR CONTROL PINS:                                            │
│  ├─ D1 (GPIO5)   ──→ LEFT Motor Direction (IN1)                │
│  ├─ D2 (GPIO4)   ──→ LEFT Motor Direction (IN2)                │
│  ├─ D3 (GPIO0)   ──→ RIGHT Motor Direction (IN1)               │
│  ├─ D4 (GPIO2)   ──→ RIGHT Motor Direction (IN2)               │
│  ├─ D5 (GPIO14)  ──→ LEFT Motor Speed (PWM Enable)             │
│  ├─ D6 (GPIO12)  ──→ RIGHT Motor Speed (PWM Enable)            │
│                                                                 │
│  SENSOR PINS:                                                   │
│  ├─ A0 (Analog)  ──→ IR Proximity Sensor (Analog Input)        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔌 DETAILED CONNECTIONS

### Section 1: POWER DISTRIBUTION

```
┌──────────────────────────────────────┐
│     18650 Battery Holder             │
│     (2x 18650 = 7.4V)                │
└────────────┬─────────────────────────┘
             │ 7.4V, GND
             │
             ▼
       ┌──────────────┐
       │Buck Converter│
       │ 7.4V → 5V    │
       └──────┬───────┘
              │ 5V, GND
              ▼
    ┌─────────────────────┐
    │   Power Rails       │
    │ ┌─────────────────┐ │
    │ │ 5V Rail    ╔═══╝ │
    │ │ GND Rail   ║     │
    │ └─────────────┨─────┤
    │               ║     │
    └───────────────╨─────┘
```

**Power Rails Distribution:**
- **5V Rail** connects to:
  - Motor Driver VCC
  - IR Sensor VCC
  - ESP8266 VIN (or through voltage regulator)
  
- **GND Rail** connects to:
  - Motor Driver GND
  - IR Sensor GND
  - ESP8266 GND (multiple pins)
  - Motors GND

---

### Section 2: MOTOR DRIVER (TB6612FNG)

```
┌────────────────────────────────────────────┐
│        TB6612FNG Motor Driver              │
│                                            │
│  POWER:                                    │
│  ├─ VCC   ────────→ 5V Rail               │
│  ├─ GND   ────────→ GND Rail (both pins)  │
│  ├─ STBY  ────────→ 5V (Enable, tie high) │
│                                            │
│  LEFT MOTOR (OUT1 & OUT2):                │
│  ├─ IN1   ←────── D1 (GPIO5)              │
│  ├─ IN2   ←────── D2 (GPIO4)              │
│  ├─ PWM   ←────── D5 (GPIO14)             │
│  ├─ OUT1  ───────→ LEFT Motor Pin 1       │
│  └─ OUT2  ───────→ LEFT Motor Pin 2       │
│                                            │
│  RIGHT MOTOR (OUT3 & OUT4):               │
│  ├─ IN3   ←────── D3 (GPIO0)              │
│  ├─ IN4   ←────── D4 (GPIO2)              │
│  ├─ PWM   ←────── D6 (GPIO12)             │
│  ├─ OUT3  ───────→ RIGHT Motor Pin 1      │
│  └─ OUT4  ───────→ RIGHT Motor Pin 2      │
│                                            │
└────────────────────────────────────────────┘
```

**Motor Driver to ESP8266:**
```
TB6612FNG Pin    →    ESP8266 Pin
─────────────────────────────────
IN1              →    D1 (GPIO5)
IN2              →    D2 (GPIO4)
PWM_A            →    D5 (GPIO14)
IN3              →    D3 (GPIO0)
IN4              →    D4 (GPIO2)
PWM_B            →    D6 (GPIO12)
VCC              →    5V
GND              →    GND
STBY             →    5V (Standby Enable)
```

**Motor Driver to Motors:**
```
TB6612FNG        →    Motor Connections
─────────────────────────────────────
OUT1 + OUT2      →    LEFT DC Motor (2 pins)
OUT3 + OUT4      →    RIGHT DC Motor (2 pins)
GND              →    Motor GND (if separate supply)
```

---

### Section 3: IR PROXIMITY SENSOR

```
┌────────────────────────────────┐
│    IR Proximity Sensor         │
│   (FC-51 or KY-033)            │
│                                │
│  VCC (Red)   ──→ 5V Rail      │
│  GND (Black) ──→ GND Rail     │
│  OUT (White) ──→ A0 (Analog)  │
│                                │
└────────────────────────────────┘
```

**IR Sensor to ESP8266:**
```
IR Sensor        →    ESP8266
─────────────────────────────
VCC              →    5V
GND              →    GND
OUT              →    A0 (Analog Input)
```

---

## 🎯 QUICK REFERENCE: Pin Assignment Table

| Component | Function | ESP8266 Pin | Pin Name | Notes |
|-----------|----------|------------|----------|-------|
| **Motor Driver** |  |  |  |  |
| | Left Dir 1 | D1 | GPIO5 | Digital Output |
| | Left Dir 2 | D2 | GPIO4 | Digital Output |
| | Left Speed | D5 | GPIO14 | PWM Output |
| | Right Dir 1 | D3 | GPIO0 | Digital Output |
| | Right Dir 2 | D4 | GPIO2 | Digital Output |
| | Right Speed | D6 | GPIO12 | PWM Output |
| **IR Sensor** |  |  |  |  |
| | Pickup Detect | A0 | ADC0 | Analog Input |
| **Power** |  |  |  |  |
| | Power | VIN | VIN | 5V Input |
| | Ground | GND | GND | Ground (connect multiple) |

---

## 🔗 Connection Summary Checklist

### Power Section
- [ ] Battery Holder positive → Buck Converter Input
- [ ] Battery Holder negative → Buck Converter Input (GND)
- [ ] Buck Converter 5V output → 5V Rail
- [ ] Buck Converter GND output → GND Rail
- [ ] 5V Rail → Motor Driver VCC
- [ ] 5V Rail → IR Sensor VCC
- [ ] 5V Rail → ESP8266 VIN
- [ ] GND Rail → Motor Driver GND (both pins)
- [ ] GND Rail → IR Sensor GND
- [ ] GND Rail → ESP8266 GND (all pins)

### Motor Control
- [ ] D1 (GPIO5) → Motor Driver IN1
- [ ] D2 (GPIO4) → Motor Driver IN2
- [ ] D5 (GPIO14) → Motor Driver PWM_A
- [ ] D3 (GPIO0) → Motor Driver IN3
- [ ] D4 (GPIO2) → Motor Driver IN4
- [ ] D6 (GPIO12) → Motor Driver PWM_B
- [ ] Motor Driver OUT1 → Left Motor Terminal 1
- [ ] Motor Driver OUT2 → Left Motor Terminal 2
- [ ] Motor Driver OUT3 → Right Motor Terminal 1
- [ ] Motor Driver OUT4 → Right Motor Terminal 2
- [ ] Motor Driver STBY → 5V (or D7 for software control)

### Sensor
- [ ] A0 (Analog) → IR Sensor OUT
- [ ] 5V Rail → IR Sensor VCC
- [ ] GND Rail → IR Sensor GND

### Checkpoint Node
- [ ] Checkpoint VCC → 5V
- [ ] Checkpoint GND → GND
- [ ] **No other connections needed!**

---

## 🧪 Testing Before Full Assembly

1. **Power Test**: Apply power, check all lights on motor driver
2. **Motor Test**: Manually send forward command, motors should spin
3. **Sensor Test**: Serial monitor should show IR values changing
4. **RSSI Test**: Bot should detect checkpoint MAC address and show RSSI

---

## ⚠️ Common Wiring Mistakes

| Mistake | Impact | Fix |
|---------|--------|-----|
| Reversed motor polarity | Motors spin wrong direction | Swap motor terminals |
| Missing GND connection | Erratic behavior | Connect ALL GNDs together |
| Wrong RSSI threshold | Bot won't stop near checkpoint | Adjust in code |
| IR sensor inverted logic | Pickup detection fails | Adjust threshold in code |
| Buck converter not stable | ESP8266 resets | Check converter output with multimeter |

---

## 📊 Wiring Summary Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    COMPLETE SYSTEM DIAGRAM                      │
│                                                                 │
│  ┌──────────────┐         ┌─────────────┐                      │
│  │   Battery    │ ───────→│   Buck      │                      │
│  │   7.4V       │         │ Converter   │                      │
│  └──────────────┘         │  5V Output  │                      │
│                           └──────┬──────┘                       │
│                                  │                              │
│                    ┌─────────────┴─────────────┐                │
│                    ▼                           ▼                │
│            ┌──────────────┐         ┌──────────────────┐       │
│            │  ESP8266     │         │  Motor Driver    │       │
│            │  Bot Control │         │  TB6612FNG       │       │
│            │              │         │                  │       │
│            │ D1,D2 (L Dir)├────────→│ IN1,IN2          │       │
│            │ D5 (L Speed) ├────────→│ PWM_A            │       │
│            │ D3,D4 (R Dir)├────────→│ IN3,IN4          │       │
│            │ D6 (R Speed) ├────────→│ PWM_B            │       │
│            │              │         │                  │       │
│            │ A0 ┌────────→│ (Sensor)│                  │       │
│            │    │         └────┬────┘                  │       │
│            │    │              │                       │       │
│            │ GND├──────────────┴─────→ GND Rail        │       │
│            │ VIN├──────────────────→ 5V Rail          │       │
│            └────┘                   │                  │       │
│                                     │                  │       │
│                                     ▼                  ▼       │
│                              ┌────────────────────────────┐    │
│                              │  Left Motor                │    │
│                              │  Right Motor               │    │
│                              │  (Connected to OUT1-OUT4)  │    │
│                              └────────────────────────────┘    │
│                                                                 │
│                           ┌──────────────────┐                 │
│                           │  IR Sensor       │                 │
│                           │  VCC ← 5V        │                 │
│                           │  GND ← GND       │                 │
│                           │  OUT → A0        │                 │
│                           └──────────────────┘                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

CHECKPOINT (Separate Location):
┌──────────────────┐
│  ESP8266 Node    │
│  VCC ← 5V        │
│  GND ← GND       │
│  (Broadcasts)    │
└──────────────────┘
```

---

## 📞 Need Help?

- Check all GND connections are solid
- Verify 5V is stable with multimeter
- Test motors directly with power before connecting driver
- Ensure Motor Driver STBY pin is pulled HIGH (to 5V)

**Happy Building! 🔧**
