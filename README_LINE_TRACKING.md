# Mecanum Wheel Line Tracking Robot

This program enables a Mecanum wheel robot to autonomously follow a black line on white ground using 5 IR sensors.

## Features

- **5 IR Sensor Array**: Detects black line position with high precision
- **Weighted Line Detection**: Calculates line position using weighted sensor readings
- **Adaptive Speed Control**: Different speeds for straight, gentle turns, and sharp turns
- **Mecanum Wheel Control**: Smooth movement using all 4 mecanum wheels
- **Lost Line Detection**: Detects when the line is lost

## Hardware Requirements

### Motors and Control
- 4 Mecanum wheels with DC motors
- L298N motor driver (or similar dual H-bridge)
- PCA9685 PWM controller (I2C, 16-channel)
- Raspberry Pi (any model with GPIO and I2C)

### IR Sensors
- 5 IR line tracking sensors (returns 0 for black, 1 for white)
- Sensor arrangement: **S0 - S1 - S2 - S3 - S4** (left to right)
- Center sensor (S2) should be aligned with expected line position

## Wiring Configuration

### Motor Connections (L298N)
```
Front Motors:
  IN1_FRONT (GPIO 23) -> Front Right direction
  IN2_FRONT (GPIO 24) -> Front Right direction
  IN3_FRONT (GPIO 27) -> Front Left direction
  IN4_FRONT (GPIO 22) -> Front Left direction

Rear Motors:
  IN1_REAR (GPIO 21) -> Rear Right direction
  IN2_REAR (GPIO 20) -> Rear Right direction
  IN3_REAR (GPIO 16) -> Rear Left direction
  IN4_REAR (GPIO 12) -> Rear Left direction
```

### PWM Connections (PCA9685)
```
Channel 0 -> Front Left motor speed (ENA_FRONT)
Channel 1 -> Front Right motor speed (ENB_FRONT)
Channel 2 -> Rear Left motor speed (ENA_REAR)
Channel 3 -> Rear Right motor speed (ENB_REAR)

I2C Address: 0x40 (default)
I2C Bus: /dev/i2c-1
```

### IR Sensor Connections (GPIO Pins)
```
IR_SENSOR_0 (GPIO 5)  -> Left-most sensor
IR_SENSOR_1 (GPIO 6)  -> Left-center sensor
IR_SENSOR_2 (GPIO 13) -> Center sensor
IR_SENSOR_3 (GPIO 19) -> Right-center sensor
IR_SENSOR_4 (GPIO 26) -> Right-most sensor
```

These GPIO pins are pre-configured in `line_tracking.c`:
```c
#define IR_SENSOR_0  5   // Left-most sensor
#define IR_SENSOR_1  6   // Left-center sensor
#define IR_SENSOR_2  13  // Center sensor
#define IR_SENSOR_3  19  // Right-center sensor
#define IR_SENSOR_4  26  // Right-most sensor
```

## Installation

### 1. Install Dependencies
```bash
cd mecanum-pi-c
sudo chmod +x install_dependencies.sh
sudo ./install_dependencies.sh
```

This installs:
- `liblgpio-dev` - Modern GPIO library for Raspberry Pi
- `i2c-tools` - I2C utilities

### 2. Enable I2C
```bash
sudo raspi-config
# Navigate to: Interface Options -> I2C -> Enable
# Reboot if prompted
```

### 3. Compile the Program
```bash
make line_tracking
```

Or compile manually:
```bash
gcc -o line_tracking line_tracking.c -llgpio
```

## Usage

### Running the Line Tracking Program
```bash
sudo ./line_tracking
```

The program will:
1. Initialize GPIO pins for motors and sensors
2. Initialize PCA9685 PWM controller
3. Wait 2 seconds before starting
4. Enter continuous line tracking loop

### Stopping the Program
Press **Ctrl+C** to stop the program. The robot will stop immediately.

## Line Tracking Algorithm

### Sensor Position Calculation
The algorithm uses weighted sensor readings to determine line position:

| Sensor Pattern | Position | Action |
|----------------|----------|--------|
| `01111` | -2 (Far Left) | Sharp left turn |
| `10111` | -1 (Left) | Gentle left turn |
| `11011` | 0 (Center) | Go straight |
| `11101` | +1 (Right) | Gentle right turn |
| `11110` | +2 (Far Right) | Sharp right turn |
| `11111` | Lost | Stop/Search |
| `00000` | Wide/Intersection | Go straight |

### Movement Speeds
```c
BASE_SPEED       = 0x3FFF  // 25% - Forward movement
TURN_SPEED       = 0x2FFF  // 18% - Gentle turns
SHARP_TURN_SPEED = 0x2000  // 12% - Sharp turns
SEARCH_SPEED     = 0x1FFF  // 12% - Searching for line
```

You can adjust these speeds in `line_tracking.c` based on your track and power supply.

## Customization

### Adjusting Sensor Pins
The sensor pins are pre-configured in `line_tracking.c` as GPIO 5, 6, 13, 19, 26. If you need to change them:
```c
#define IR_SENSOR_0  5   // Left-most sensor
#define IR_SENSOR_1  6   // Left-center sensor
#define IR_SENSOR_2  13  // Center sensor
#define IR_SENSOR_3  19  // Right-center sensor
#define IR_SENSOR_4  26  // Right-most sensor
```

### Adjusting Speeds
Modify speed constants in `line_tracking.c`:
```c
#define BASE_SPEED      0x3FFF  // Increase for faster movement
#define TURN_SPEED      0x2FFF  // Adjust turn speed
#define SHARP_TURN_SPEED 0x2000 // Adjust sharp turn speed
```

**WARNING**: Higher speeds require better power supply. Insufficient power can cause Raspberry Pi to reboot.

### Changing Update Rate
Modify the delay in the main loop:
```c
usleep(50000);  // 50ms = 20Hz update rate
```

### Adding Search Pattern (When Line Lost)
In the `track_line()` function, you can add a search pattern when `position == 100`:
```c
if (position == 100) {
    // Custom search pattern
    turn_left(SEARCH_SPEED);  // Example: rotate to find line
    usleep(100000);
}
```

## Track Setup Recommendations

1. **Line Width**: 2-3 cm wide black tape works well
2. **Background**: White or light-colored surface
3. **Sensor Height**: 5-10mm above ground for best detection
4. **Lighting**: Consistent lighting (avoid shadows)
5. **Track Complexity**: Start with gentle curves, then add sharper turns
6. **Intersections**: The robot will go straight when detecting wide black areas

## Troubleshooting

### Robot Doesn't Move
- Check if PCA9685 is properly connected to I2C
- Verify I2C is enabled: `sudo i2cdetect -y 1` (should show 0x40)
- Ensure motor power supply is connected
- Check GPIO pin permissions (run with `sudo`)

### Sensors Not Reading Correctly
- Verify sensor wiring (VCC, GND, Signal)
- Test individual sensors: read GPIO values manually
- Check sensor height above ground (5-10mm optimal)
- Ensure proper pull-up/pull-down resistors if needed

### Robot Turns in Wrong Direction
- Check motor wiring (swap IN1/IN2 pairs if needed)
- Verify sensor order (S0 should be leftmost)
- Check if IR sensors are inverted (0=white, 1=black)

### Raspberry Pi Reboots During Operation
- **Power supply issue** - use 5V/3A+ adapter
- Motors need **separate power supply**
- Connect all grounds together
- Reduce speed constants in code

### Line Tracking is Erratic
- Lower the update rate (increase `usleep` value)
- Adjust speed constants for smoother movement
- Calibrate sensor spacing for your line width
- Check for electrical noise affecting sensors

## Files

- `line_tracking.c` - Main line tracking program
- `Makefile` - Build configuration
- `install_dependencies.sh` - Dependency installation script
- `README.md` - Project overview
- `README_LINE_TRACKING.md` - This file (detailed documentation)
- `TUTORIAL.md` - Complete step-by-step tutorial
- `QUICK_START_COMMANDS.md` - Quick command reference

## Example Output

```
=== Mecanum Wheel Line Tracking Robot ===
Press Ctrl+C to stop

Initialization complete!
Starting line tracking in 2 seconds...

Sensors: [1 1 0 1 1]  Position: 0
Center - Going straight
Sensors: [1 1 0 1 1]  Position: 0
Center - Going straight
Sensors: [1 0 1 1 1]  Position: -1
Slight left - Gentle turn left
Sensors: [0 1 1 1 1]  Position: -2
Far left - Sharp turn left
...
```

## License

This code is provided as-is for educational and hobbyist use.

## Credits

Based on the mecanum wheel controller using:
- lgpio library for GPIO control
- Linux I2C for PCA9685 PWM control
