# Mecanum Wheel Robot Controller - C Version for Raspberry Pi

A high-performance C implementation for controlling a 4-wheeled mecanum robot on Raspberry Pi using GPIO and PCA9685 PWM controller.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi-red.svg)
![Language](https://img.shields.io/badge/language-C-green.svg)

## Table of Contents

- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Quick Start](#quick-start)
- [Detailed Installation](#detailed-installation)
- [Hardware Setup](#hardware-setup)
- [Compilation and Running](#compilation-and-running)
- [Movement Capabilities](#movement-capabilities)
- [Customization](#customization)
- [Troubleshooting](#troubleshooting)
- [API Reference](#api-reference)
- [Power Supply Requirements](#power-supply-requirements)
- [Contributing](#contributing)
- [License](#license)

## Features

✅ **Complete mecanum wheel control** - Forward, backward, lateral, diagonal, and rotation movements
✅ **Optimized C code** - Faster execution than Python implementations
✅ **Modern GPIO library** - Uses `lgpio` (not deprecated wiringPi)
✅ **I2C PWM control** - Direct control of PCA9685 via I2C
✅ **Power-safe defaults** - Conservative speeds to prevent Raspberry Pi brown-outs
✅ **Well-documented** - Extensive comments and documentation
✅ **Easy to customize** - Adjustable speeds, movement patterns, and pin configurations

## Hardware Requirements

### Required Components

| Component | Specification | Notes |
|-----------|--------------|-------|
| **Raspberry Pi** | Any model with 40-pin GPIO | Tested on Pi 4, Pi 3, Pi Zero 2W |
| **OS** | Raspberry Pi OS (Trixie or newer) | Debian 12+ recommended |
| **Motor Driver** | L298N or Model-Pi motor driver board | Dual H-bridge driver |
| **PWM Controller** | PCA9685 16-channel 12-bit PWM | I2C address 0x40 (default) |
| **Motors** | 4x DC motors with encoders (optional) | For mecanum wheels |
| **Power Supply** | 5V 3A+ for Pi, separate for motors | See [Power Requirements](#power-supply-requirements) |
| **Wiring** | Jumper wires, breadboard (optional) | For connections |

### Pin Connections

#### GPIO Pins (BCM Numbering)
```
Front Motors (L298N)
├── IN1_FRONT: GPIO 23  (Front Right direction)
├── IN2_FRONT: GPIO 24  (Front Right direction)
├── IN3_FRONT: GPIO 27  (Front Left direction)
└── IN4_FRONT: GPIO 22  (Front Left direction)

Rear Motors (L298N)
├── IN1_REAR:  GPIO 21  (Rear Right direction)
├── IN2_REAR:  GPIO 20  (Rear Right direction)
├── IN3_REAR:  GPIO 16  (Rear Left direction)
└── IN4_REAR:  GPIO 12  (Rear Left direction)
```

#### PCA9685 PWM Channels
```
Motor Speed Control
├── Channel 0: Front Left motor (ENA_FRONT)
├── Channel 1: Front Right motor (ENB_FRONT)
├── Channel 2: Rear Left motor (ENA_REAR)
└── Channel 3: Rear Right motor (ENB_REAR)
```

#### I2C Connection
```
PCA9685 to Raspberry Pi
├── SDA: GPIO 2 (Pin 3)
├── SCL: GPIO 3 (Pin 5)
├── VCC: 5V
└── GND: Ground
```

## Quick Start

For experienced users, here's the fastest way to get started:

```bash
# 1. Clone repository
git clone https://github.com/osoyoo/mecanum-pi-c.git
cd mecanum-pi-c

# 2. Install dependencies
chmod +x install_dependencies.sh
sudo ./install_dependencies.sh

# 3. Reboot if I2C was just enabled
sudo reboot

# 4. Compile
make

# 5. Run
sudo ./mecanum5
```

For detailed step-by-step instructions, see [Detailed Installation](#detailed-installation) below.

## Detailed Installation

### Step 1: Prepare Your Raspberry Pi

#### 1.1 Update System
```bash
sudo apt-get update
sudo apt-get upgrade -y
```

#### 1.2 Enable I2C Interface
```bash
sudo raspi-config
```

Navigate to:
- **Interface Options** → **I2C** → **Enable**
- Select **Finish** and reboot

Or enable via command line:
```bash
sudo raspi-config nonint do_i2c 0
sudo reboot
```

#### 1.3 Verify I2C is Working
After reboot:
```bash
# Check if I2C device exists
ls /dev/i2c-*
# Should show: /dev/i2c-1

# Install I2C tools (if not installed)
sudo apt-get install -y i2c-tools

# Scan I2C bus (with PCA9685 connected and powered)
sudo i2cdetect -y 1
```

You should see your PCA9685 at address `0x40`:
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: 40 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
...
```

### Step 2: Install Required Libraries

#### 2.1 Automated Installation (Recommended)

Download and run the installation script:

```bash
cd /home/pi
git clone https://github.com/osoyoo/mecanum-pi-c.git
cd mecanum-pi-c

chmod +x install_dependencies.sh
sudo ./install_dependencies.sh
```

The script will automatically:
- Install build tools (gcc, make)
- Install I2C libraries
- Install lgpio library
- Enable I2C interface
- Verify all installations

#### 2.2 Manual Installation (If Script Fails)

If the automated script doesn't work, install manually:

```bash
# Install build tools
sudo apt-get install -y build-essential

# Install I2C development libraries
sudo apt-get install -y i2c-tools libi2c-dev

# Install lgpio library
sudo apt-get install -y liblgpio-dev liblgpio1

# If lgpio package not available, build from source:
cd /tmp
git clone https://github.com/joan2937/lg.git
cd lg
make
sudo make install
sudo ldconfig
cd ~
```

#### 2.3 Verify Installation

```bash
# Check gcc
gcc --version

# Check if lgpio header exists
ls /usr/include/lgpio.h
# or
ls /usr/local/include/lgpio.h

# Check I2C tools
which i2cdetect
```

### Step 3: Download and Compile Code

#### 3.1 Get the Code

If you haven't already cloned the repository:

```bash
cd /home/pi
git clone https://github.com/osoyoo/mecanum-pi-c.git
cd mecanum-pi-c
```

#### 3.2 Compile the Program

Using Makefile (recommended):
```bash
make
```

Or compile manually:
```bash
gcc -o mecanum5 mecanum5.c -llgpio
```

You should see:
```
Compiling mecanum5...
Build complete! Run with: sudo ./mecanum5
```

### Step 4: Connect Hardware

⚠️ **IMPORTANT**: Before connecting, ensure both Raspberry Pi and motor power supply are OFF!

#### 4.1 Connect L298N Motor Driver to Raspberry Pi

| L298N Pin | Raspberry Pi GPIO | Description |
|-----------|-------------------|-------------|
| IN1 | GPIO 23 | Front Right motor direction |
| IN2 | GPIO 24 | Front Right motor direction |
| IN3 | GPIO 27 | Front Left motor direction |
| IN4 | GPIO 22 | Front Left motor direction |
| Additional L298N (or ports) for rear motors |
| IN1 | GPIO 21 | Rear Right motor direction |
| IN2 | GPIO 20 | Rear Right motor direction |
| IN3 | GPIO 16 | Rear Left motor direction |
| IN4 | GPIO 12 | Rear Left motor direction |
| GND | GND | Common ground |

#### 4.2 Connect PCA9685 to Raspberry Pi

| PCA9685 Pin | Raspberry Pi Pin | Description |
|-------------|------------------|-------------|
| VCC | 5V (Pin 2 or 4) | Power |
| GND | GND (Pin 6, 9, etc.) | Ground |
| SDA | GPIO 2 (Pin 3) | I2C Data |
| SCL | GPIO 3 (Pin 5) | I2C Clock |

#### 4.3 Connect PCA9685 to Motor Drivers

| PCA9685 Channel | Connection | Motor |
|----------------|------------|-------|
| PWM 0 | ENA (Front Left) | Front Left speed |
| PWM 1 | ENB (Front Right) | Front Right speed |
| PWM 2 | ENA (Rear Left) | Rear Left speed |
| PWM 3 | ENB (Rear Right) | Rear Right speed |

#### 4.4 Connect Motors and Power

1. Connect 4 motors to L298N driver outputs
2. Connect motor power supply to L298N (typically 6-12V depending on motors)
3. **Connect all grounds together** (Pi GND, PCA9685 GND, motor supply GND)
4. Power Raspberry Pi from separate 5V 3A+ power supply

⚠️ **NEVER** power motors from Raspberry Pi GPIO pins!

### Step 5: Run the Program

```bash
cd /home/pi/mecanum-pi-c
sudo ./mecanum5
```

Expected output:
```
Initializing Mecanum Wheel Robot for Raspberry Pi...
Initialization complete!

Moving ahead...
Moving backward...
Shifting left...
Shifting right...
Moving upper-left (diagonal)...
Moving lower-right (diagonal)...
Moving upper-right (diagonal)...
Moving lower-left (diagonal)...
Turning left (reduced speed)...
Turning right (reduced speed)...

Demo complete!
```

If motors don't move, see [Troubleshooting](#troubleshooting).

## Hardware Setup

### Complete Wiring Diagram

```
Raspberry Pi
    │
    ├─── GPIO Pins ─────────► L298N Motor Drivers
    │                         (Direction Control)
    │                              │
    │                              └─► 4 DC Motors
    │
    └─── I2C (SDA/SCL) ────► PCA9685 PWM Controller
                              (Speed Control)
                                   │
                                   └─► L298N Enable Pins
                                       (ENA/ENB)

Power Supplies:
- Raspberry Pi: 5V 3A+ USB-C/Micro USB
- Motors: 6-12V (depends on motor spec) → L298N
- PCA9685: 5V from Raspberry Pi

⚠️ Common Ground: All GND must be connected together!
```

## Compilation and Running

### Using Makefile (Recommended)

```bash
# Build
make

# Clean and rebuild
make clean
make

# Install dependencies (requires sudo)
sudo make install-deps

# Build and run
make run

# Help
make help
```

### Manual Compilation

```bash
# Compile
gcc -o mecanum5 mecanum5.c -llgpio

# With debug symbols
gcc -g -o mecanum5 mecanum5.c -llgpio

# With optimizations
gcc -O2 -o mecanum5 mecanum5.c -llgpio

# Run
sudo ./mecanum5
```

### Why sudo is Required

The program needs sudo for:
- GPIO access (lgpio requires permission)
- I2C device access (/dev/i2c-1)

## Movement Capabilities

The mecanum wheel design enables 10 different movement patterns:

### 1. Linear Movements

```c
go_ahead(MOVE_SPEED);     // Move forward
go_back(MOVE_SPEED);      // Move backward
shift_left(MOVE_SPEED);   // Slide left (parallel)
shift_right(MOVE_SPEED);  // Slide right (parallel)
```

### 2. Diagonal Movements

```c
upper_left(MOVE_SPEED);   // Move diagonally forward-left
upper_right(MOVE_SPEED);  // Move diagonally forward-right
lower_left(MOVE_SPEED);   // Move diagonally backward-left
lower_right(MOVE_SPEED);  // Move diagonally backward-right
```

### 3. Rotational Movements

```c
turn_left(TURN_SPEED);    // Rotate counter-clockwise in place
turn_right(TURN_SPEED);   // Rotate clockwise in place
```

### 4. Stop

```c
stop_car();               // Stop all motors
```

## Customization

### Adjusting Motor Speeds

Edit `mecanum5.c` lines 69-70:

```c
#define MOVE_SPEED  0x4FFF  // Normal movement (31% of max)
#define TURN_SPEED  0x2FFF  // Rotation (18% of max)
```

Speed values range from `0x0000` (stopped) to `0xFFFF` (maximum).

**Recommended values**:
- Conservative: `MOVE_SPEED = 0x3FFF`, `TURN_SPEED = 0x1FFF`
- Moderate: `MOVE_SPEED = 0x4FFF`, `TURN_SPEED = 0x2FFF` (default)
- Fast: `MOVE_SPEED = 0x7FFF`, `TURN_SPEED = 0x3FFF` (requires good power supply)

⚠️ **Warning**: Speeds above 50% may cause brown-outs if power supply is insufficient!

### Changing GPIO Pins

Edit `mecanum5.c` lines 53-64 to match your wiring:

```c
#define IN1_FRONT   23      // Change these numbers
#define IN2_FRONT   24      // to your actual
#define IN3_FRONT   27      // GPIO pin numbers
// ... etc
```

### Changing PWM Channels

Edit `mecanum5.c` lines 60-63:

```c
#define ENA_FRONT   0       // PCA9685 channel for front-left
#define ENB_FRONT   1       // PCA9685 channel for front-right
#define ENA_REAR    2       // PCA9685 channel for rear-left
#define ENB_REAR    3       // PCA9685 channel for rear-right
```

### Creating Custom Movements

Example - diagonal movement at specific angle:

```c
void custom_move(uint16_t speed) {
    // Set specific motor directions
    rr_ahead();    // Rear right forward
    fl_ahead();    // Front left forward

    // Set speeds
    change_speed(speed);
}
```

## Troubleshooting

### Motors Don't Move

**Possible Causes**:

1. **I2C not enabled**
   ```bash
   sudo raspi-config nonint do_i2c 0
   sudo reboot
   ```

2. **PCA9685 not detected**
   ```bash
   sudo i2cdetect -y 1
   # Should show 40 at address 0x40
   ```

3. **Wrong power connections**
   - Verify motor power supply is connected and ON
   - Check all grounds are connected together
   - Motors need separate power (NOT from Pi)

4. **Wrong GPIO pins**
   - Verify wiring matches pin definitions in code
   - Check BCM numbering is used (not physical pin numbers)

5. **Insufficient permissions**
   - Run with `sudo ./mecanum5`

### Program Compilation Errors

**Error**: `lgpio.h: No such file or directory`

**Solution**:
```bash
sudo apt-get install liblgpio-dev liblgpio1
# Or build from source (see installation section)
```

**Error**: `undefined reference to lgGpiochipOpen`

**Solution**:
```bash
# Make sure you're linking with -llgpio
gcc -o mecanum5 mecanum5.c -llgpio
```

### Raspberry Pi Reboots During Movement

**Cause**: Insufficient power supply causing voltage drop

**Solutions**:
1. Use better power supply for Pi (5V 3A minimum)
2. Reduce motor speeds (lower MOVE_SPEED and TURN_SPEED)
3. Add decoupling capacitors (100nF + 10µF near PCA9685)
4. Ensure motor power is separate from Pi power

### I2C Communication Fails

**Check I2C bus speed** (if needed):
```bash
# Edit /boot/config.txt
sudo nano /boot/config.txt

# Add or modify:
dtparam=i2c_arm=on
dtparam=i2c_arm_baudrate=100000

# Save and reboot
sudo reboot
```

### Motors Run in Wrong Direction

**Solution**: Swap motor wire pairs or modify direction functions:

```c
// In mecanum5.c, swap HIGH/LOW for affected motor
void rr_ahead(void) {
    lgGpioWrite(gpio_chip, IN1_REAR, 0);  // Swapped
    lgGpioWrite(gpio_chip, IN2_REAR, 1);  // Swapped
}
```

## API Reference

### Movement Functions

All movement functions take `speed` parameter (0-0xFFFF):

```c
// Linear movements
void go_ahead(uint16_t speed);
void go_back(uint16_t speed);
void shift_left(uint16_t speed);
void shift_right(uint16_t speed);

// Diagonal movements
void upper_left(uint16_t speed);
void upper_right(uint16_t speed);
void lower_left(uint16_t speed);
void lower_right(uint16_t speed);

// Rotations
void turn_left(uint16_t speed);
void turn_right(uint16_t speed);

// Stop
void stop_car(void);
```

### Individual Motor Control

```c
// Rear motors
void rr_ahead(void);   // Rear right forward
void rr_back(void);    // Rear right backward
void rl_ahead(void);   // Rear left forward
void rl_back(void);    // Rear left backward

// Front motors
void fr_ahead(void);   // Front right forward
void fr_back(void);    // Front right backward
void fl_ahead(void);   // Front left forward
void fl_back(void);    // Front left backward
```

### Low-Level Functions

```c
// Change all motor speeds
void change_speed(uint16_t speed);

// PCA9685 control
void pca9685_init(int fd);
void pca9685_set_pwm(int fd, uint8_t channel, uint16_t value);

// I2C helpers
int i2c_write_byte(int fd, uint8_t reg, uint8_t value);
int i2c_read_byte(int fd, uint8_t reg);
```

## Power Supply Requirements

### Critical Power Guidelines

⚠️ **Most Important**: Inadequate power is the #1 cause of issues!

### Minimum Requirements

| Component | Voltage | Current | Notes |
|-----------|---------|---------|-------|
| Raspberry Pi | 5V | 3A | Official power supply recommended |
| Motors (total) | 6-12V | 2-4A | Depends on motor spec |
| PCA9685 | 5V | 50mA | Can power from Pi if no servos |

### Best Practices

1. **Separate Power Supplies**
   - Raspberry Pi: Dedicated 5V 3A+ USB power supply
   - Motors: Separate battery/power supply (6-12V depending on motors)
   - **NEVER** power motors from Pi GPIO pins

2. **Common Ground**
   - Connect all GND together (Pi, motor supply, PCA9685)
   - This is REQUIRED for proper operation

3. **Decoupling Capacitors**
   - Add 100nF ceramic + 10µF electrolytic near PCA9685
   - Reduces noise and voltage spikes

4. **Voltage Drop Protection**
   - If Pi reboots during operation, power supply is insufficient
   - Reduce motor speeds or upgrade power supply

### Power Supply Checklist

- [ ] Raspberry Pi has dedicated 5V 3A+ power supply
- [ ] Motors have separate power supply (correct voltage for your motors)
- [ ] All grounds (GND) are connected together
- [ ] PCA9685 is powered (5V from Pi is OK if no servos connected)
- [ ] No motors connected to Pi GPIO pins (only control signals)
- [ ] Power supplies can handle peak current draw
- [ ] Wiring is secure with no loose connections

## Project Structure

```
mecanum-pi-c/
├── mecanum5.c                 # Main C source code
├── mecanum5.py                # Original Python version (reference)
├── test.py                    # Diagnostic tool for troubleshooting
├── Makefile                   # Build automation
├── install_dependencies.sh    # Automated library installation
├── README.md                  # This file
├── QUICKSTART.md             # Quick start guide
├── README_C_VERSION.md       # Detailed technical documentation
├── BUGFIX_SUMMARY.md         # Development notes and bug fixes
└── LICENSE                    # MIT License

```

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

### Coding Guidelines

- Follow existing code style
- Add comments for complex logic
- Test on real hardware before submitting
- Update documentation if needed

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Based on Python implementation from OSOYOO Mecanum Robot Kit
- Uses [lgpio](https://github.com/joan2937/lg) library by Joan2937
- PCA9685 control inspired by Adafruit's CircuitPython library

## Support

- **Issues**: [GitHub Issues](https://github.com/osoyoo/mecanum-pi-c/issues)
- **Discussions**: [GitHub Discussions](https://github.com/osoyoo/mecanum-pi-c/discussions)
- **Email**: support@osoyoo.com

## Related Projects

- [OSOYOO Mecanum Robot Kit](https://osoyoo.com/mecanum-robot-car/)
- [Python Version](https://github.com/osoyoo/mecanum-pi-python)

---

**Made with ❤️ for the Raspberry Pi and Robotics Community**
