# Quick Start Commands for Raspberry Pi

These are the exact commands to run on your Raspberry Pi to get the line tracking code working.

## Step-by-Step Commands

### 1. Update System
```bash
sudo apt-get update
sudo apt-get upgrade -y
```

### 2. Enable I2C
```bash
sudo raspi-config
# Navigate to: Interface Options -> I2C -> Enable -> Finish
sudo reboot
```

### 3. Install Git (if needed)
```bash
sudo apt-get install git -y
```

### 4. Clone Repository from GitHub
```bash
cd ~
mkdir -p mecanum-projects
cd mecanum-projects
git clone https://github.com/osoyoo/mecanum-pi-c-linetrack.git
cd mecanum-pi-c-linetrack
```

### 5. Install Dependencies
```bash
chmod +x install_dependencies.sh
sudo ./install_dependencies.sh
```

### 6. Verify I2C (PCA9685 should show at address 40)
```bash
sudo i2cdetect -y 1
```

### 7. Configure IR Sensor Pins (if using different GPIO pins)
```bash
nano line_tracking.c
# Edit lines 58-62 to match your wiring
# Save: Ctrl+O, Enter, Ctrl+X
```

### 8. Compile the Program
```bash
make line_tracking
```

### 9. Run Line Tracking
```bash
sudo ./line_tracking
```

### 10. Stop the Program
Press **Ctrl+C**

---

## Troubleshooting Commands

### Check I2C devices
```bash
sudo i2cdetect -y 1
```

### Check GPIO pins
```bash
gpio readall
```

### Re-compile after changes
```bash
make clean
make line_tracking
```

### View system logs
```bash
tail -f /var/log/syslog
```

---

## File Locations

- **Code**: `~/mecanum-projects/mecanum-pi-c-linetrack/`
- **Executable**: `~/mecanum-projects/mecanum-pi-c-linetrack/line_tracking`
- **Tutorial**: `~/mecanum-projects/mecanum-pi-c-linetrack/TUTORIAL.md`
- **Documentation**: `~/mecanum-projects/mecanum-pi-c-linetrack/README_LINE_TRACKING.md`

---

For detailed explanations, see **TUTORIAL.md**.
