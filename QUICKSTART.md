# Quick Start Guide - Mecanum Robot C Version

Get your mecanum robot running in under 5 minutes!

## Prerequisites

✅ Raspberry Pi with Raspberry Pi OS (Trixie or newer)
✅ Hardware connected (motors, L298N driver, PCA9685)
✅ Internet connection for installing dependencies
✅ PCA9685 powered and connected via I2C

## 3-Step Installation

### Step 1: Download and Install Dependencies

```bash
# Clone the repository
git clone https://github.com/osoyoo/mecanum-pi-c.git
cd mecanum-pi-c

# Install required libraries
chmod +x install_dependencies.sh
sudo ./install_dependencies.sh
```

**If I2C was just enabled by the script, reboot:**
```bash
sudo reboot
cd mecanum-pi-c  # Navigate back after reboot
```

### Step 2: Compile the Program

```bash
make
```

Or manually:
```bash
gcc -o mecanum5 mecanum5.c -llgpio
```

Expected output:
```
Compiling mecanum5...
Build complete! Run with: sudo ./mecanum5
```

### Step 3: Run the Program

```bash
sudo ./mecanum5
```

🎉 **Your robot should now perform all movement demonstrations!**

## Expected Output

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

## Verify I2C Setup (Optional)

Check if PCA9685 is detected on I2C bus:

```bash
sudo i2cdetect -y 1
```

Expected output - you should see `40` at address 0x40:
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
...
40: 40 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
...
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| **Motors don't move** | Check power supply to motors (separate from Pi) |
| **`Failed to open GPIO chip`** | Run with `sudo ./mecanum5` |
| **`Failed to open I2C bus`** | Enable I2C: `sudo raspi-config` → Interface Options → I2C |
| **`Failed to set I2C slave address`** | Check PCA9685 connections and power (5V + GND) |
| **Compilation error** | Run `sudo ./install_dependencies.sh` again |
| **Pi reboots during turns** | Insufficient power - use better power supply or reduce speeds |
| **Wrong direction** | Check motor wiring or swap direction in code |

## Quick Commands Reference

```bash
# Build
make

# Clean and rebuild
make clean && make

# Run
sudo ./mecanum5

# Install/update dependencies
sudo make install-deps

# Build and run in one command
make run
```

## Hardware Connection Checklist

Essential connections:

- [ ] **PCA9685 to Raspberry Pi I2C**
  - [ ] SDA → GPIO 2 (Pin 3)
  - [ ] SCL → GPIO 3 (Pin 5)
  - [ ] VCC → 5V
  - [ ] GND → GND

- [ ] **L298N to Raspberry Pi GPIO**
  - [ ] IN1-IN4 (front) → GPIO 23, 24, 27, 22
  - [ ] IN1-IN4 (rear) → GPIO 21, 20, 16, 12
  - [ ] GND → GND (common ground)

- [ ] **PCA9685 to L298N**
  - [ ] PWM 0-3 → ENA/ENB pins of motor drivers

- [ ] **Motors and Power**
  - [ ] 4 motors → L298N outputs
  - [ ] Motor power supply → L298N (6-12V, separate from Pi!)
  - [ ] All GND connected together

⚠️ **CRITICAL**: Never power motors from Raspberry Pi GPIO!

## Adjusting Motor Speeds

If robot moves too fast/slow, edit `mecanum5.c` lines 69-70:

```c
#define MOVE_SPEED  0x4FFF  // Normal movement (try 0x3FFF for slower)
#define TURN_SPEED  0x2FFF  // Rotation (try 0x1FFF for slower)
```

Then recompile:
```bash
make clean && make
sudo ./mecanum5
```

## Movement Examples

Once running, you can use these functions in your own programs:

```c
go_ahead(MOVE_SPEED);      // Move forward
go_back(MOVE_SPEED);       // Move backward
shift_left(MOVE_SPEED);    // Slide left
shift_right(MOVE_SPEED);   // Slide right
turn_left(TURN_SPEED);     // Rotate left
turn_right(TURN_SPEED);    // Rotate right
upper_left(MOVE_SPEED);    // Diagonal forward-left
stop_car();                // Stop all motors
```

## Safety Tips ⚠️

- **Test in open space** - no obstacles or edges
- **Start with lower speeds** - increase gradually
- **Keep Ctrl+C ready** - emergency stop anytime
- **Check wiring first** - loose connections can cause issues
- **Use proper power supply** - 5V 3A+ for Pi, 6-12V for motors

## Next Steps

📖 **Full Documentation**: See [README.md](README.md) for complete tutorial
🔧 **Advanced Setup**: See [README_C_VERSION.md](README_C_VERSION.md)
🐛 **Debugging**: See [BUGFIX_SUMMARY.md](BUGFIX_SUMMARY.md)
💻 **Customize**: Edit `mecanum5.c` to create custom movements

## Getting Help

- **GitHub Issues**: [Report a problem](https://github.com/osoyoo/mecanum-pi-c/issues)
- **Discussions**: [Ask questions](https://github.com/osoyoo/mecanum-pi-c/discussions)
- **Email**: support@osoyoo.com

---

**Happy robot building! 🤖**
