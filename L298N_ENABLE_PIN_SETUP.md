# L298N Enable Pin Configuration - CRITICAL!

## ⚠️ MOST COMMON REASON MOTORS DON'T MOVE

If your motors don't move at all, 90% of the time it's because the **L298N enable pin jumpers are still ON**.

---

## What Are Enable Pins?

L298N motor drivers have **enable pins (ENA and ENB)** that control motor speed:
- **ENA**: Controls speed of motors on OUT1/OUT2 (e.g., Front motors)
- **ENB**: Controls speed of motors on OUT3/OUT4 (e.g., Rear motors)

---

## The Problem: Enable Pin Jumpers

Many L298N boards come with **jumpers** (small plastic caps) on the enable pins.

### ❌ WRONG Configuration (Jumpers ON):
```
L298N Board:
┌─────────────┐
│  [ENA] ■    │  ← Jumper ON (shorted to 5V)
│  [ENB] ■    │  ← Jumper ON (shorted to 5V)
│             │
│   IN1  IN2  │
│   IN3  IN4  │
└─────────────┘
```

**Result**: Motors always at full speed OR not controllable by PWM
- You cannot control speed
- PCA9685 PWM signals are ignored
- Motors might not move at all or only at full speed

### ✅ CORRECT Configuration (Jumpers REMOVED):
```
L298N Board:
┌─────────────┐
│   ENA  ○    │  ← Jumper REMOVED! Pin exposed
│   ENB  ○    │  ← Jumper REMOVED! Pin exposed
│    ↑   ↑    │
│    │   │    │  ← Connect PCA9685 PWM here!
│    │   │    │
│   IN1  IN2  │
│   IN3  IN4  │
└─────────────┘
      ↓
PCA9685 Ch0 ──→ ENA
PCA9685 Ch1 ──→ ENB
```

**Result**: Motors controlled by PCA9685 PWM
- Speed is software-controllable
- PWM signals from PCA9685 control motor speed
- Line tracking works properly!

---

## Step-by-Step Fix

### 1. Remove the Jumpers

**Look at your L298N board:**
- Find the ENA and ENB pins (usually labeled)
- Are there small plastic jumper caps on them?
- **REMOVE them!** (Pull them off gently)

Save the jumpers for future use, but **DO NOT use them for this project**.

### 2. Connect PCA9685 PWM Outputs

After removing jumpers, connect:

```
PCA9685 → L298N Enable Pins:

Channel 0 → ENA (Front Left motor speed)
Channel 1 → ENB (Front Right motor speed)  
Channel 2 → ENA (Rear Left motor speed)
Channel 3 → ENB (Rear Right motor speed)
```

**Note**: You may have two L298N boards (one for front, one for rear) or one board driving all motors. Adjust connections accordingly.

### 3. Verify Connections

Your complete PCA9685 connections should be:

```
PCA9685 Channels:
┌──────────────────┐
│ CH0 ────────────────→ L298N ENA (Front Left speed)
│ CH1 ────────────────→ L298N ENB (Front Right speed)
│ CH2 ────────────────→ L298N ENA (Rear Left speed)
│ CH3 ────────────────→ L298N ENB (Rear Right speed)
│                  │
│ VCC ← Pi 3.3V   │
│ SDA ← Pi SDA    │
│ SCL ← Pi SCL    │
│ GND ← Pi GND    │
└──────────────────┘
```

---

## How to Check if Jumpers Are The Problem

### Quick Test:

1. **With jumpers ON** (wrong):
   - Run `sudo ./motor_test`
   - Motors either don't move OR move at full speed regardless of PWM

2. **With jumpers REMOVED** (correct):
   - Run `sudo ./motor_test`
   - Motors should spin at controlled speeds
   - Speed should be controllable by code

---

## Visual Identification

### What Does a Jumper Look Like?

```
Side view of jumper cap:
    ┌─┐
    │ │  ← Small plastic cap
    └┬┘
     │
   ──┴──  ← Two metal pins (shorted together)
```

### Where to Find Them on L298N:

Look for labels on the board:
- **ENA** (Enable A) 
- **ENB** (Enable B)

They're usually near the top of the board, often with small plastic jumper caps.

Some boards label them:
- `5V-ENA` or just `ENA`
- `5V-ENB` or just `ENB`

---

## Common Questions

### Q: Can I use jumpers for any configuration?
**A**: Only if you want motors at full speed all the time. For line tracking with speed control, **jumpers must be removed**.

### Q: What if my L298N doesn't have jumpers?
**A**: Some boards have solder bridges instead. You may need to desolder the bridge and add a wire from the enable pin to PCA9685.

### Q: Can I connect enable pins directly to Raspberry Pi GPIO?
**A**: Technically yes, but you'd need PWM-capable GPIO pins. Using PCA9685 is better because:
- 16 PWM channels (vs. limited RPi hardware PWM)
- Doesn't burden the CPU
- More consistent PWM frequency

### Q: What voltage should PCA9685 output to enable pins?
**A**: PCA9685 outputs 0-5V PWM signal. This is perfect for L298N enable pins which expect 0-5V for speed control.

---

## After Removing Jumpers

Once jumpers are removed and PCA9685 is connected:

```bash
# Test motors
make motor_test
sudo ./motor_test

# Motors should now spin!
# If they do, run line tracking:
make clean && make
sudo ./line_tracking
```

---

## Still Not Working?

If motors still don't move after removing jumpers:

1. **Check motor power supply**: 
   - Is 6-12V supply connected to L298N power input?
   - Try measuring voltage with multimeter

2. **Check PCA9685 wiring**:
   - `sudo i2cdetect -y 1` should show device at 0x40
   - Verify SDA, SCL, VCC, GND connections

3. **Test enable pins directly**:
   - Temporarily connect enable pin to 5V (use jumper wire)
   - Set IN1=HIGH, IN2=LOW (via GPIO)
   - Motor should spin at full speed
   - If it does, PCA9685→L298N wiring is the problem

4. **Check motor power voltage**:
   - Measure voltage at motor terminals while running
   - Should see voltage when motor is commanded to move
   - If 0V, check motor power supply or L298N

---

## Summary

**The #1 fix for "motors don't move":**
1. ✅ **REMOVE L298N enable pin jumpers** (ENA and ENB)
2. ✅ **Connect PCA9685 channels 0-3 to L298N enable pins**
3. ✅ **Ensure motor power supply is connected (6-12V)**
4. ✅ **Connect all grounds together**

**Then test:**
```bash
sudo ./motor_test
```

Motors should spin at full speed now!
