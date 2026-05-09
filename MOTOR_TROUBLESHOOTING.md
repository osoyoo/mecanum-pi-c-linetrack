# Motor Troubleshooting Guide

## Problem: Sensors Work But Motors Don't Move

If you see sensor readings and position calculations working, but the robot doesn't move:

```
Sensors: [1 0 1 1 1]  Position: -1
Slight left - Gentle turn left
```

...but the car stays stationary, this indicates a **motor control issue**, not a sensor issue.

---

## Step 1: Run Motor Diagnostic Test

We've created a special motor test program to diagnose the issue:

### Compile the motor test:
```bash
make motor_test
```

### Run the motor test:
```bash
sudo ./motor_test
```

### What the test does:
1. Checks GPIO and I2C initialization
2. Tests each motor individually (forward and backward)
3. Tests all motors together
4. Provides diagnostic information

### Expected results:
- ✅ **Motors spin**: Your hardware is working! Issue is with speed values or logic.
- ❌ **Motors don't spin**: Hardware connection problem (see Step 2)

---

## Step 2: Check Hardware Connections

If motors don't spin during the test, check these connections:

### A. PCA9685 PWM Controller

**I2C Connection (Raspberry Pi → PCA9685):**
```
Pi Pin 1 (3.3V)  → PCA9685 VCC
Pi Pin 3 (SDA)   → PCA9685 SDA
Pi Pin 5 (SCL)   → PCA9685 SCL
Pi Pin 9 (GND)   → PCA9685 GND
```

**Verify I2C is working:**
```bash
sudo i2cdetect -y 1
```

You should see `40` in the grid. If not, PCA9685 is not connected or I2C is not enabled.

**PWM Channels (PCA9685 → L298N Enable Pins):**
```
PCA9685 Channel 0 → L298N ENA (Front Left motor enable)
PCA9685 Channel 1 → L298N ENB (Front Right motor enable)
PCA9685 Channel 2 → L298N ENA (Rear Left motor enable)
PCA9685 Channel 3 → L298N ENB (Rear Right motor enable)
```

⚠️ **Common mistake**: Not connecting PCA9685 PWM outputs to L298N enable pins!

### B. L298N Motor Driver

**Direction Pins (Raspberry Pi GPIO → L298N):**
```
Front Motors:
GPIO 23 → IN1 (Front Right direction)
GPIO 24 → IN2 (Front Right direction)
GPIO 27 → IN3 (Front Left direction)
GPIO 22 → IN4 (Front Left direction)

Rear Motors:
GPIO 21 → IN1 (Rear Right direction)
GPIO 20 → IN2 (Rear Right direction)
GPIO 16 → IN3 (Rear Left direction)
GPIO 12 → IN4 (Rear Left direction)
```

**Motor Power:**
- L298N needs **separate power supply** (6-12V depending on motors)
- Connect battery/power supply to L298N power input
- Do NOT power motors from Raspberry Pi!

**Enable Pins:**
- Remove jumpers from L298N enable pins (ENA/ENB)
- Connect PCA9685 PWM outputs to these pins
- This allows software speed control

### C. Ground Connections

**CRITICAL: All grounds must be connected together:**
```
Raspberry Pi GND ──┐
                   ├── Common Ground
Motor Power GND ───┤
                   │
PCA9685 GND ───────┤
                   │
L298N GND ─────────┘
```

If grounds are not connected, nothing will work!

### D. Motors

**Motor Connections (L298N → Motors):**
```
L298N OUT1/OUT2 → Front Right Motor
L298N OUT3/OUT4 → Front Left Motor
L298N OUT1/OUT2 → Rear Right Motor
L298N OUT3/OUT4 → Rear Left Motor
```

---

## Step 3: Check Power Supply

### Raspberry Pi Power
- Use quality 5V/3A power adapter
- If Pi reboots during operation, power is insufficient

### Motor Power
- Motors need separate 6-12V supply (check motor specifications)
- Ensure battery is charged or power supply has enough current
- Test voltage with multimeter

---

## Step 4: Speed Settings

The speeds have been increased in the latest version:

```c
#define BASE_SPEED      0x8FFF  // 56% max
#define TURN_SPEED      0x6FFF  // 43% max
#define SHARP_TURN_SPEED 0x5FFF // 37% max
```

If motors barely move:
1. Increase these values in `line_tracking.c`
2. Recompile: `make`
3. Test again: `sudo ./line_tracking`

**Maximum value:** `0xFFFF` (100%, but may draw too much power)

---

## Step 5: Enable Pin Configuration

Some L298N boards have jumpers on enable pins:

### Option 1: Jumpers ON (Not recommended)
- Motors always at full speed
- No software speed control
- PWM from PCA9685 is ignored

### Option 2: Jumpers OFF (Recommended)
- Remove jumpers from ENA and ENB pins
- Connect PCA9685 PWM outputs to these pins
- Allows software speed control

**For line tracking, you MUST use Option 2!**

---

## Step 6: Verify Direction Control

If motors spin but all in wrong direction or not synchronized:

### Test individual GPIO pins:
```bash
# Set GPIO as output
gpio -g mode 23 out

# Set HIGH (motor should respond)
gpio -g write 23 1

# Set LOW
gpio -g write 23 0
```

If GPIO control doesn't work, check GPIO connections.

---

## Common Issues and Solutions

| Symptom | Likely Cause | Solution |
|---------|--------------|----------|
| No I2C device detected | PCA9685 not connected or I2C disabled | Check wiring, enable I2C in raspi-config |
| Motors don't spin at all | Enable pins not connected to PWM | Connect PCA9685 channels to L298N ENA/ENB |
| Motors spin but very weak | Speed values too low | Increase BASE_SPEED in code |
| Raspberry Pi reboots | Insufficient power | Use better power supply, separate motor power |
| Motors spin randomly | Common ground missing | Connect all grounds together |
| Only some motors work | Specific wiring issue | Check connections for non-working motors |

---

## Quick Diagnosis Checklist

Run through this checklist:

- [ ] `sudo i2cdetect -y 1` shows device at 0x40
- [ ] PCA9685 VCC, SDA, SCL, GND connected to Raspberry Pi
- [ ] PCA9685 channels 0-3 connected to L298N enable pins (jumpers removed!)
- [ ] L298N IN1-IN4 (front) connected to GPIO 23,24,27,22
- [ ] L298N IN1-IN4 (rear) connected to GPIO 21,20,16,12
- [ ] L298N has separate motor power supply (6-12V)
- [ ] All grounds connected together
- [ ] Motors connected to L298N OUT1-OUT4
- [ ] `sudo ./motor_test` runs and tests each motor
- [ ] Speed values in code are high enough (>0x5000)

---

## Still Not Working?

If motors still don't work after checking everything:

1. **Test L298N standalone:**
   - Connect motor directly
   - Apply power
   - Manually set IN1=HIGH, IN2=LOW, ENA=HIGH
   - Motor should spin

2. **Test PCA9685 standalone:**
   - Use `i2cdump -y 1 0x40` to check registers
   - Values should change when running program

3. **Swap components:**
   - Try different motor
   - Try different L298N module
   - Try different wiring

4. **Check for damaged components:**
   - Burned smell?
   - Visibly damaged chips?
   - Voltage drops under load?

---

## After Motors Work

Once motors are spinning with `motor_test`, recompile and run line tracking:

```bash
make clean
make
sudo ./line_tracking
```

The robot should now follow the line!

If it moves but tracking is erratic:
- Adjust speed values (lower if too fast)
- Calibrate sensor positions
- Improve track contrast (darker line, whiter background)
- See TUTORIAL.md for advanced tuning

---

## Getting Help

If you're still stuck, gather this information:

1. Output of `sudo i2cdetect -y 1`
2. Output of `sudo ./motor_test`
3. Photo of your wiring
4. Motor and power supply specifications
5. Any error messages

Then open an issue on GitHub with this information.
