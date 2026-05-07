#!/bin/bash

#####################################################################
# Dependency Installation Script for Mecanum Robot on Raspberry Pi
# Compatible with Raspberry Pi OS Trixie (Debian 13)
#
# This script will:
# 1. Update package lists
# 2. Install required system libraries for GPIO and I2C
# 3. Enable I2C interface if not already enabled
# 4. Install build tools (gcc, make)
# 5. Verify installation
#
# Usage: sudo ./install_dependencies.sh
#####################################################################

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "ERROR: This script must be run as root (use sudo)"
    echo "Usage: sudo ./install_dependencies.sh"
    exit 1
fi

echo "======================================================================"
echo "Installing dependencies for Mecanum Robot Controller"
echo "Raspberry Pi OS: Trixie (Debian 13)"
echo "======================================================================"
echo ""

# Step 1: Update package lists
echo "[1/5] Updating package lists..."
apt-get update
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to update package lists"
    exit 1
fi
echo "✓ Package lists updated"
echo ""

# Step 2: Install build tools
echo "[2/5] Installing build tools (gcc, make)..."
apt-get install -y build-essential
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to install build tools"
    exit 1
fi
echo "✓ Build tools installed"
echo ""

# Step 3: Install I2C tools and libraries
echo "[3/5] Installing I2C libraries and tools..."
apt-get install -y i2c-tools libi2c-dev
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to install I2C libraries"
    exit 1
fi
echo "✓ I2C libraries installed"
echo ""

# Step 4: Install lgpio library (modern GPIO library for Raspberry Pi)
echo "[4/5] Installing lgpio library..."
apt-get install -y liblgpio-dev liblgpio1
if [ $? -ne 0 ]; then
    echo "WARNING: Package installation failed, attempting to build from source..."

    # Install dependencies for building lgpio
    apt-get install -y git swig python3-dev

    # Clone and build lgpio
    cd /tmp
    if [ -d "lg" ]; then
        rm -rf lg
    fi

    git clone https://github.com/joan2937/lg.git
    cd lg
    make
    make install
    ldconfig

    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to install lgpio library"
        exit 1
    fi

    cd ~
    echo "✓ lgpio library built and installed from source"
else
    echo "✓ lgpio library installed from package"
fi
echo ""

# Step 5: Enable I2C interface
echo "[5/5] Enabling I2C interface..."

# Check if I2C is already enabled
if lsmod | grep -q "i2c_dev"; then
    echo "✓ I2C interface is already enabled"
else
    # Add i2c-dev to /etc/modules if not already there
    if ! grep -q "^i2c-dev" /etc/modules; then
        echo "i2c-dev" >> /etc/modules
    fi

    # Load the module immediately
    modprobe i2c-dev

    # Enable I2C using raspi-config non-interactive mode
    if command -v raspi-config &> /dev/null; then
        raspi-config nonint do_i2c 0
    fi

    echo "✓ I2C interface enabled"
    echo "  NOTE: A reboot may be required for I2C changes to take effect"
fi
echo ""

# Verify installations
echo "======================================================================"
echo "Verifying installations..."
echo "======================================================================"

# Check gcc
echo -n "Checking gcc... "
if command -v gcc &> /dev/null; then
    gcc_version=$(gcc --version | head -n1)
    echo "✓ $gcc_version"
else
    echo "✗ NOT FOUND"
fi

# Check I2C tools
echo -n "Checking i2cdetect... "
if command -v i2cdetect &> /dev/null; then
    echo "✓ FOUND"
else
    echo "✗ NOT FOUND"
fi

# Check lgpio header
echo -n "Checking lgpio library... "
if [ -f "/usr/include/lgpio.h" ] || [ -f "/usr/local/include/lgpio.h" ]; then
    echo "✓ FOUND"
else
    echo "✗ NOT FOUND"
fi

# Check I2C device
echo -n "Checking I2C device... "
if [ -e "/dev/i2c-1" ]; then
    echo "✓ /dev/i2c-1 exists"
else
    echo "⚠ /dev/i2c-1 not found (may require reboot)"
fi

echo ""
echo "======================================================================"
echo "Installation Complete!"
echo "======================================================================"
echo ""
echo "Next steps:"
echo "1. Compile the program:"
echo "   gcc -o mecanum5 mecanum5.c -llgpio"
echo ""
echo "2. Run the program:"
echo "   sudo ./mecanum5"
echo ""
echo "IMPORTANT NOTES:"
echo "- If I2C device was not found, please reboot your Raspberry Pi:"
echo "  sudo reboot"
echo ""
echo "- To verify I2C is working after reboot, run:"
echo "  sudo i2cdetect -y 1"
echo "  (You should see your PCA9685 device at address 0x40)"
echo ""
echo "- Make sure your hardware connections are correct before running"
echo "======================================================================"
