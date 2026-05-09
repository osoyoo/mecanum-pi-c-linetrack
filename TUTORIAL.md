# Complete Tutorial: Mecanum Wheel Line Tracking Robot

## Getting Started with Line Tracking on Raspberry Pi

This tutorial will guide you through the complete process of getting the line tracking code from GitHub onto your Raspberry Pi, installing dependencies, compiling, and running the program.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Step 1: Prepare Your Raspberry Pi](#step-1-prepare-your-raspberry-pi)
3. [Step 2: Download Code from GitHub](#step-2-download-code-from-github)
4. [Step 3: Install Dependencies](#step-3-install-dependencies)
5. [Step 4: Hardware Setup](#step-4-hardware-setup)
6. [Step 5: Configure GPIO Pins](#step-5-configure-gpio-pins)
7. [Step 6: Compile the Program](#step-6-compile-the-program)
8. [Step 7: Test Your Setup](#step-7-test-your-setup)
9. [Step 8: Run Line Tracking](#step-8-run-line-tracking)
10. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Hardware Requirements

#### Essential Components:
1. **Raspberry Pi** (any model with GPIO, tested on Pi 4/5)
   - 5V/3A power supply (minimum)
   - MicroSD card (16GB+, Class 10 recommended)
   - Raspberry Pi OS installed (Bookworm or newer)

2. **Motors and Wheels**
   - 4x Mecanum wheels
   - 4x DC motors (compatible with mecanum wheels)

3. **Motor Control**
   - L298N motor driver module (or equivalent dual H-bridge)
   - PCA9685 16-channel PWM controller (I2C)

4. **Line Tracking Sensors**
   - 5x IR line tracking sensor modules
   - Should output digital signal: 0 for black, 1 for white

5. **Power Supply**
   - Separate power supply for motors (6-12V, 2A+)
   - Battery pack or wall adapter

6. **Wiring**
   - Jumper wires (male-to-male, male-to-female)
   - Breadboard (optional, for testing)

#### Optional but Recommended:
- Robot chassis to mount components
- Voltage regulator for stable 5V supply
- Capacitors for motor noise filtering
- Cable ties for wire management

### Software Requirements

- **Raspberry Pi OS** (Bookworm 64-bit recommended)
- **Internet connection** (for downloading code and dependencies)
- **SSH access** (optional, for headless setup)

---

## Step 1: Prepare Your Raspberry Pi

### 1.1 Update System

Open a terminal on your Raspberry Pi (or connect via SSH) and run:

```bash
sudo apt-get update
sudo apt-get upgrade -y
```

This ensures you have the latest packages.

### 1.2 Enable I2C Interface

The PCA9685 PWM controller communicates via I2C, which must be enabled:

```bash
sudo raspi-config
```

Navigate through the menu:
- Select **3 Interface Options**
- Select **I5 I2C**
- Select **Yes** to enable
- Select **Finish**

If prompted to reboot, do so now:

```bash
sudo reboot
```

### 1.3 Verify I2C is Working

After reboot, verify I2C is enabled:

```bash
ls /dev/i2c*
```

You should see `/dev/i2c-1` (on most Raspberry Pi models).

### 1.4 Install Git

Install git if not already installed:

```bash
sudo apt-get install git -y
```

Verify installation:

```bash
git --version
```

---

## Step 2: Download Code from GitHub

### 2.1 Navigate to Your Workspace

Create a workspace directory (or use existing one):

```bash
cd ~
mkdir -p mecanum-projects
cd mecanum-projects
```

### 2.2 Clone the Repository

Clone the line tracking repository from GitHub:

```bash
git clone https://github.com/osoyoo/mecanum-pi-c-linetrack.git
```

### 2.3 Enter the Project Directory

```bash
cd mecanum-pi-c-linetrack
```

### 2.4 Verify Files

List the files to verify everything downloaded correctly:

```bash
ls -la
```

You should see:
- `line_tracking.c` - Main line tracking program
- `Makefile` - Build configuration
- `install_dependencies.sh` - Dependency installation script
- `README.md` - Project overview
- `README_LINE_TRACKING.md` - Line tracking documentation
- `TUTORIAL.md` - Complete tutorial (this file)
- `QUICK_START_COMMANDS.md` - Quick command reference
- `.gitignore`, `LICENSE` - Git and license files

---

## Step 3: Install Dependencies

### 3.1 Make Installation Script Executable

```bash
chmod +x install_dependencies.sh
```

### 3.2 Run Dependency Installation

```bash
sudo ./install_dependencies.sh
```

This script installs:
- **liblgpio-dev** - Modern GPIO library for Raspberry Pi
- **i2c-tools** - I2C utilities for testing and debugging
- **gcc** - C compiler (if not already installed)
- **make** - Build automation tool

### 3.3 Verify I2C Tools Installation

Test I2C detection (with PCA9685 connected):

```bash
sudo i2cdetect -y 1
```

You should see a grid with `40` (the PCA9685 default I2C address) marked if your PCA9685 is connected correctly.

**Example output:**
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: 40 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- --
```

If you don't see `40`, check your PCA9685 wiring (especially SDA, SCL, VCC, and GND).

---

## Step 4: Hardware Setup

### 4.1 Wiring Diagram Overview

Below is the complete wiring configuration:

#### Motor Control (L298N Driver)

Connect Raspberry Pi GPIO pins to L298N motor driver:

```
Raspberry Pi GPIO → L298N Motor Driver

Front Motors (L298N Output A & B):
GPIO 23 (IN1_FRONT) → IN1 (Front Right direction)
GPIO 24 (IN2_FRONT) → IN2 (Front Right direction)
GPIO 27 (IN3_FRONT) → IN3 (Front Left direction)
GPIO 22 (IN4_FRONT) → IN4 (Front Left direction)

Rear Motors (L298N Output A & B):
GPIO 21 (IN1_REAR) → IN1 (Rear Right direction)
GPIO 20 (IN2_REAR) → IN2 (Rear Right direction)
GPIO 16 (IN3_REAR) → IN3 (Rear Left direction)
GPIO 12 (IN4_REAR) → IN4 (Rear Left direction)
```

#### PWM Control (PCA9685)

Connect PCA9685 to Raspberry Pi via I2C:

```
Raspberry Pi → PCA9685
Pin 1 (3.3V)    → VCC
Pin 3 (SDA)     → SDA
Pin 5 (SCL)     → SCL
Pin 9 (GND)     → GND
```

Connect PCA9685 PWM outputs to L298N enable pins:

```
PCA9685 Channel → L298N Enable Pin
Channel 0       → ENA (Front Left motor)
Channel 1       → ENB (Front Right motor)
Channel 2       → ENA (Rear Left motor)
Channel 3       → ENB (Rear Right motor)
```

#### IR Sensor Connections

Connect 5 IR sensors to Raspberry Pi GPIO pins:

```
Raspberry Pi GPIO → IR Sensor (Pre-configured)
GPIO 5  → Sensor 0 (Left-most)
GPIO 6  → Sensor 1 (Left-center)
GPIO 13 → Sensor 2 (Center)
GPIO 19 → Sensor 3 (Right-center)
GPIO 26 → Sensor 4 (Right-most)

Each sensor also needs:
5V or 3.3V → VCC
GND → GND
```

**NOTE**: These GPIO pins are already configured in the code. Connect your sensors to these pins.

#### Power Supply

```
Raspberry Pi: 5V/3A USB-C power supply
Motors: 6-12V external battery/supply connected to L298N

CRITICAL: Connect all grounds together:
- Raspberry Pi GND
- L298N GND
- PCA9685 GND
- Motor power supply GND
```

### 4.2 Assembly Tips

1. **Mount sensors** in a straight line at the front of the robot
2. **Sensor spacing**: 2-3 cm apart works well for standard tracks
3. **Sensor height**: 5-10mm above ground for optimal detection
4. **Cable management**: Use cable ties to prevent wire tangles
5. **Test connections**: Double-check all connections before powering on

---

## Step 5: Configure GPIO Pins

### 5.1 Check Your IR Sensor Wiring

If you used different GPIO pins for your IR sensors, you need to update the code.

### 5.2 Edit line_tracking.c

Open the file for editing:

```bash
nano line_tracking.c
```

The IR sensor pins are already configured in the code (around line 59-65):

```c
// IR Sensor GPIO Pin Definitions (BCM numbering)
#define IR_SENSOR_0  5   // Left-most sensor
#define IR_SENSOR_1  6   // Left-center sensor
#define IR_SENSOR_2  13  // Center sensor
#define IR_SENSOR_3  19  // Right-center sensor
#define IR_SENSOR_4  26  // Right-most sensor
```

**These pins are pre-configured to GPIO 5, 6, 13, 19, 26.** Simply connect your sensors to these pins.

If you need to use different pins, change the numbers in the code above to match your wiring.

Save and exit:
- Press `Ctrl+O` to save
- Press `Enter` to confirm
- Press `Ctrl+X` to exit

### 5.3 Optional: Adjust Motor Speeds

You can also adjust speed settings (around line 69-72):

```c
#define BASE_SPEED      0x3FFF  // 25% - Forward movement
#define TURN_SPEED      0x2FFF  // 18% - Gentle turns
#define SHARP_TURN_SPEED 0x2000 // 12% - Sharp turns
```

Start with these conservative values. Increase only if you have a good power supply.

---

## Step 6: Compile the Program

### 6.1 Compile Using Makefile

The easiest method is using the included Makefile:

```bash
make line_tracking
```

You should see:

```
Compiling line_tracking...
gcc -Wall -O2 -o line_tracking line_tracking.c -llgpio
Build complete! Run with: sudo ./line_tracking
```

### 6.2 Alternative: Manual Compilation

If the Makefile doesn't work, compile manually:

```bash
gcc -Wall -O2 -o line_tracking line_tracking.c -llgpio
```

### 6.3 Verify Compilation

Check that the executable was created:

```bash
ls -lh line_tracking
```

You should see a file named `line_tracking` with execute permissions.

---

## Step 7: Test Your Setup

Before running the full line tracking program, test individual components.

### 7.1 Test I2C Connection

```bash
sudo i2cdetect -y 1
```

Verify PCA9685 appears at address `40`.

### 7.2 Test IR Sensors (Manual Method)

Create a simple test script to check sensors:

```bash
cat > test_sensors.sh << 'SCRIPT'
#!/bin/bash
echo "Reading IR sensors (Ctrl+C to stop)"
while true; do
    echo -n "S0: $(cat /sys/class/gpio/gpio5/value 2>/dev/null || echo '?') "
    echo -n "S1: $(cat /sys/class/gpio/gpio6/value 2>/dev/null || echo '?') "
    echo -n "S2: $(cat /sys/class/gpio/gpio13/value 2>/dev/null || echo '?') "
    echo -n "S3: $(cat /sys/class/gpio/gpio19/value 2>/dev/null || echo '?') "
    echo "S4: $(cat /sys/class/gpio/gpio26/value 2>/dev/null || echo '?')"
    sleep 0.5
done
SCRIPT

chmod +x test_sensors.sh
sudo ./test_sensors.sh
```

Move a black line under the sensors and verify values change from 1 to 0.

---

## Step 8: Run Line Tracking

### 8.1 Prepare Your Track

Create a test track:

1. **Surface**: Use white paper, poster board, or white floor
2. **Line**: Use black electrical tape (2-3 cm wide)
3. **Shape**: Start with a simple oval or straight line with gentle curves
4. **Lighting**: Ensure consistent lighting (avoid shadows)

### 8.2 Position the Robot

Place the robot on the track:
- **Center sensor (S2)** should be over the black line
- Sensors should be 5-10mm above the surface
- Robot should be facing forward along the track

### 8.3 Run the Line Tracking Program

```bash
sudo ./line_tracking
```

Expected output:

```
=== Mecanum Wheel Line Tracking Robot ===
Press Ctrl+C to stop

Initializing Mecanum Wheel Robot for Raspberry Pi...
Initialization complete!
Starting line tracking in 2 seconds...

Sensors: [1 1 0 1 1]  Position: 0
Center - Going straight
Sensors: [1 1 0 1 1]  Position: 0
Center - Going straight
Sensors: [1 0 1 1 1]  Position: -1
Slight left - Gentle turn left
...
```

### 8.4 Monitor Behavior

The robot should:
- Follow the black line smoothly
- Turn gently when line deviates slightly
- Turn sharply when line curves sharply
- Stop if it loses the line (all sensors see white)
- Continue straight on wide lines/intersections

### 8.5 Stop the Program

Press `Ctrl+C` to stop. The robot will halt immediately.

---

## Troubleshooting

### Problem 1: Compilation Errors

**Error: `lgpio.h: No such file or directory`**

Solution:
```bash
sudo apt-get install liblgpio-dev
```

**Error: `undefined reference to lgGpiochipOpen`**

Solution: Add `-llgpio` to compilation:
```bash
gcc -o line_tracking line_tracking.c -llgpio
```

### Problem 2: Robot Doesn't Move

**Check 1**: Is PCA9685 detected?
```bash
sudo i2cdetect -y 1
```

**Check 2**: Are motors getting power?
- Verify motor power supply is connected
- Check L298N LED indicators
- Measure voltage at motor terminals

**Check 3**: GPIO permissions?
- Always run with `sudo`
- Check GPIO pin ownership

**Check 4**: Enable pins connected?
- Verify PCA9685 channels 0-3 are connected to L298N enable pins

### Problem 3: Sensors Not Reading

**Check 1**: Sensor power
- Verify VCC and GND connections
- Check sensor LED indicators (if present)

**Check 2**: Sensor height
- Should be 5-10mm above surface
- Too high: won't detect line
- Too low: may scratch surface

**Check 3**: Test sensors manually
```bash
sudo gpio -g mode 5 in
sudo gpio -g read 5
```

Replace 5 with any of your sensor pin numbers (5, 6, 13, 19, or 26).

**Check 4**: Sensor polarity
- Some sensors output inverted signals (0=white, 1=black)
- If inverted, modify code to flip logic

### Problem 4: Raspberry Pi Reboots/Crashes

**Cause**: Power supply insufficient

**Solutions**:
1. Use high-quality 5V/3A+ power supply for Raspberry Pi
2. Use separate power for motors (6-12V)
3. Connect all grounds together
4. Add decoupling capacitors near motors
5. Reduce speed constants in code:

```c
#define BASE_SPEED      0x2FFF  // Reduce from 0x3FFF
#define TURN_SPEED      0x1FFF  // Reduce from 0x2FFF
```

### Problem 5: Erratic Line Tracking

**Solution 1**: Adjust speeds
- Lower speeds for better control
- Increase gentle turn threshold

**Solution 2**: Calibrate sensors
- Ensure sensors are evenly spaced
- Align center sensor with line
- Check sensor height consistency

**Solution 3**: Improve track
- Use wider black line (3cm+)
- Avoid sharp corners (start with gentle curves)
- Ensure consistent lighting
- Use matte black tape (glossy can reflect IR)

**Solution 4**: Adjust update rate

Edit `line_tracking.c` line ~426:
```c
usleep(100000);  // Increase from 50000 for smoother control
```

### Problem 6: Robot Turns Wrong Direction

**Solution 1**: Check motor wiring
- Swap IN1/IN2 pairs if motor spins backward
- Verify front/rear motors are correctly identified

**Solution 2**: Check sensor order
- S0 should be leftmost, S4 rightmost
- Verify sensor positions match code

**Solution 3**: Reverse turn logic

If left/right are swapped, modify turn functions in code.

### Problem 7: Line Lost Too Easily

**Solution 1**: Add search pattern

Edit `track_line()` function around line 377:

```c
if (position == 100) {
    // Add search: rotate slowly to find line
    turn_left(SEARCH_SPEED);
    usleep(100000);  // Search for 100ms
    return;  // Don't stop immediately
}
```

**Solution 2**: Increase sensor sensitivity
- Adjust sensor potentiometer (if available)
- Lower sensor height slightly
- Use better contrast (darker line, whiter surface)

---

## Advanced Customization

### Add Velocity Control

Modify `gentle_turn_left()` and `gentle_turn_right()` to vary speeds more:

```c
void gentle_turn_left(uint16_t left_speed, uint16_t right_speed) {
    // Slow down left side more for sharper gentle turns
    uint16_t adjusted_left = left_speed * 0.6;  // 60% speed
    uint16_t adjusted_right = right_speed;       // 100% speed
    
    rl_ahead();
    fl_ahead();
    rr_ahead();
    fr_ahead();
    
    pca9685_set_pwm(i2c_fd, ENA_FRONT, adjusted_left);
    pca9685_set_pwm(i2c_fd, ENA_REAR, adjusted_left);
    pca9685_set_pwm(i2c_fd, ENB_FRONT, adjusted_right);
    pca9685_set_pwm(i2c_fd, ENB_REAR, adjusted_right);
}
```

### PID Control

For smoother tracking, implement PID control:

```c
float kp = 1.0, ki = 0.0, kd = 0.1;
float last_error = 0, integral = 0;

float pid_control(int position) {
    float error = position;
    integral += error;
    float derivative = error - last_error;
    last_error = error;
    
    return kp * error + ki * integral + kd * derivative;
}
```

### Add Speed Display

Print current speed for debugging:

```c
printf("Base: %d, Turn: %d, Position: %d\n", 
       BASE_SPEED, TURN_SPEED, position);
```

---

## Next Steps

### Improve Your Track

1. Add sharper curves
2. Create intersections
3. Add loops and figure-8 patterns
4. Build ramps (test stability)

### Enhance the Code

1. Implement PID control for smoother tracking
2. Add speed ramping (gradual acceleration/deceleration)
3. Create waypoint navigation
4. Add obstacle detection with ultrasonic sensors
5. Implement data logging to SD card

### Learn More

- Study PID control theory
- Explore computer vision alternatives
- Learn about sensor fusion
- Experiment with different line colors/widths

---

## Additional Resources

### Documentation
- `README.md` - Project overview
- `README_LINE_TRACKING.md` - Detailed line tracking documentation
- `QUICKSTART.md` - Quick start guide

### GitHub Repository
- https://github.com/osoyoo/mecanum-pi-c-linetrack

### Helpful Commands

```bash
# Re-compile after code changes
make clean && make line_tracking

# View compile warnings in detail
gcc -Wall -Wextra -o line_tracking line_tracking.c -llgpio

# Monitor system logs while running
tail -f /var/log/syslog

# Check GPIO status
gpio readall

# Monitor I2C traffic (requires i2c-tools)
sudo i2cdetect -y 1
sudo i2cdump -y 1 0x40
```

---

## Safety Notes

1. **Power Supply**: Always use appropriate power supplies
   - Raspberry Pi: 5V/3A minimum
   - Motors: Check motor specifications

2. **Current Protection**: 
   - Use fuses on battery connections
   - Don't exceed L298N current ratings (2A per channel typically)

3. **Heat Management**:
   - L298N can get hot - add heatsinks if needed
   - Ensure adequate ventilation

4. **Testing**:
   - Test on a soft surface first (in case robot runs away)
   - Keep emergency power disconnect handy
   - Never leave robot running unattended

5. **Wiring**:
   - Double-check connections before powering on
   - Avoid short circuits
   - Use appropriate wire gauges

---

## Conclusion

You now have a fully functional line tracking mecanum wheel robot! 

**Key Takeaways:**
- GPIO control with lgpio library
- I2C communication with PCA9685
- Weighted sensor algorithm for line detection
- Differential motor control for smooth turns

Experiment, learn, and have fun building!

---

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Review code comments in `line_tracking.c`
3. Consult `README_LINE_TRACKING.md` for details
4. Open an issue on GitHub repository

Happy robot building! 🤖
