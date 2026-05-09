/*
 * Motor Test Program for Mecanum Wheel Robot
 * 
 * This program tests the motor control system to diagnose issues.
 * Run this BEFORE line_tracking to verify motors work.
 *
 * COMPILATION:
 *   gcc -o motor_test motor_test.c -llgpio
 *
 * EXECUTION:
 *   sudo ./motor_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <lgpio.h>

// PCA9685 Register Addresses
#define PCA9685_ADDRESS     0x40
#define MODE1               0x00
#define MODE2               0x01
#define PRESCALE            0xFE
#define LED0_ON_L           0x06

// I2C Bus
#define I2C_BUS             "/dev/i2c-1"

// Motor GPIO Pins
#define IN1_FRONT   23
#define IN2_FRONT   24
#define IN3_FRONT   27
#define IN4_FRONT   22
#define IN1_REAR    21
#define IN2_REAR    20
#define IN3_REAR    16
#define IN4_REAR    12

// PCA9685 PWM Channels
#define ENA_FRONT   0
#define ENB_FRONT   1
#define ENA_REAR    2
#define ENB_REAR    3

// Test speeds - Match Python working code
#define TEST_SPEED  0x7FFF  // 50% - same as Python move_speed

// Global variables
int i2c_fd;
int gpio_chip;

int i2c_write_byte(int fd, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    if (write(fd, buf, 2) != 2) {
        fprintf(stderr, "ERROR: i2c_write_byte failed for reg 0x%02X\n", reg);
        return -1;
    }
    return 0;
}

int i2c_read_byte(int fd, uint8_t reg) {
    uint8_t value;
    if (write(fd, &reg, 1) != 1) return -1;
    if (read(fd, &value, 1) != 1) return -1;
    return value;
}

void pca9685_init(int fd) {
    printf("Initializing PCA9685...\n");
    // Reset MODE1
    i2c_write_byte(fd, MODE1, 0x00);
    usleep(10000);

    // CRITICAL FIX: Set MODE2 for totem pole output
    // This is what the Python adafruit library does!
    printf("  Setting MODE2 = 0x04 (totem pole output)...\n");
    i2c_write_byte(fd, MODE2, 0x04);
    usleep(5000);

    // Set frequency to 60Hz
    uint8_t prescale = 101;
    int oldmode = i2c_read_byte(fd, MODE1);
    int newmode = (oldmode & 0x7F) | 0x10;
    i2c_write_byte(fd, MODE1, newmode);
    usleep(2000);
    i2c_write_byte(fd, PRESCALE, prescale);
    usleep(2000);
    i2c_write_byte(fd, MODE1, oldmode);
    usleep(10000);
    i2c_write_byte(fd, MODE1, oldmode | 0xA0);  // Auto-increment + Restart
    usleep(5000);
    printf("✓ PCA9685 initialized with MODE2=0x04!\n");
}

void pca9685_set_pwm(int fd, uint8_t channel, uint16_t value) {
    // CRITICAL FIX: Invert PWM for active-LOW enable pins on L298N
    // Your L298N enable pins are active-LOW (motors run when pin is LOW)
    // So we invert: high input value = low PWM output = motor ON
    uint16_t pwm_value_12bit = value >> 4;          // Scale 16-bit to 12-bit
    uint16_t inverted_pwm = 0xFFF - pwm_value_12bit; // Invert for active-LOW

    uint8_t reg = LED0_ON_L + 4 * channel;
    uint8_t buf[5];
    buf[0] = reg;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = inverted_pwm & 0xFF;
    buf[4] = (inverted_pwm >> 8) & 0x0F;

    if (write(fd, buf, 5) != 5) {
        fprintf(stderr, "ERROR: Failed to write PWM to channel %d\n", channel);
    } else {
        printf("  PWM channel %d: input=0x%04X, inverted=0x%03X\n",
               channel, value, inverted_pwm);
    }
}

void change_speed(uint16_t speed) {
    printf("Setting all motor speeds to 0x%04X\n", speed);
    pca9685_set_pwm(i2c_fd, ENA_FRONT, speed);
    pca9685_set_pwm(i2c_fd, ENB_FRONT, speed);
    pca9685_set_pwm(i2c_fd, ENA_REAR, speed);
    pca9685_set_pwm(i2c_fd, ENB_REAR, speed);
}

void stop_all(void) {
    printf("Stopping all motors\n");
    lgGpioWrite(gpio_chip, IN1_FRONT, 0);
    lgGpioWrite(gpio_chip, IN2_FRONT, 0);
    lgGpioWrite(gpio_chip, IN3_FRONT, 0);
    lgGpioWrite(gpio_chip, IN4_FRONT, 0);
    lgGpioWrite(gpio_chip, IN1_REAR, 0);
    lgGpioWrite(gpio_chip, IN2_REAR, 0);
    lgGpioWrite(gpio_chip, IN3_REAR, 0);
    lgGpioWrite(gpio_chip, IN4_REAR, 0);
    change_speed(0);
}

void test_motor(const char* name, int pin1, int pin2, int pwm_channel) {
    printf("\n=== Testing %s ===\n", name);

    // CRITICAL: Ensure ALL other channels are stopped before testing this motor
    // This prevents interference between channels
    printf("  Initializing all channels to stopped state...\n");
    for (int ch = 0; ch < 4; ch++) {
        if (ch != pwm_channel) {
            pca9685_set_pwm(i2c_fd, ch, 0);  // 0 = stopped (HIGH in inverted mode)
        }
    }
    usleep(100000);  // Wait 100ms for channels to settle

    // Forward direction
    printf("  Direction: FORWARD\n");
    lgGpioWrite(gpio_chip, pin1, 1);
    lgGpioWrite(gpio_chip, pin2, 0);
    pca9685_set_pwm(i2c_fd, pwm_channel, TEST_SPEED);
    printf("  Motor should spin now. Waiting 2 seconds...\n");
    sleep(2);

    // Stop
    lgGpioWrite(gpio_chip, pin1, 0);
    lgGpioWrite(gpio_chip, pin2, 0);
    pca9685_set_pwm(i2c_fd, pwm_channel, 0);
    printf("  Motor stopped. Waiting 1 second...\n");
    sleep(1);

    // Backward direction
    printf("  Direction: BACKWARD\n");
    lgGpioWrite(gpio_chip, pin1, 0);
    lgGpioWrite(gpio_chip, pin2, 1);
    pca9685_set_pwm(i2c_fd, pwm_channel, TEST_SPEED);
    printf("  Motor should spin now. Waiting 2 seconds...\n");
    sleep(2);

    // Stop
    lgGpioWrite(gpio_chip, pin1, 0);
    lgGpioWrite(gpio_chip, pin2, 0);
    pca9685_set_pwm(i2c_fd, pwm_channel, 0);
    printf("  Motor stopped.\n");
}

int main(void) {
    printf("=== Mecanum Wheel Motor Test Program ===\n\n");
    
    // Open GPIO chip
    gpio_chip = lgGpiochipOpen(0);
    if (gpio_chip < 0) {
        fprintf(stderr, "Failed to open GPIO chip\n");
        fprintf(stderr, "Make sure you run with sudo\n");
        return 1;
    }
    printf("✓ GPIO chip opened\n");
    
    // Claim GPIO pins
    int pins[] = {IN1_FRONT, IN2_FRONT, IN3_FRONT, IN4_FRONT,
                  IN1_REAR, IN2_REAR, IN3_REAR, IN4_REAR};
    for (int i = 0; i < 8; i++) {
        if (lgGpioClaimOutput(gpio_chip, 0, pins[i], 0) < 0) {
            fprintf(stderr, "Failed to claim GPIO pin %d\n", pins[i]);
            lgGpiochipClose(gpio_chip);
            return 1;
        }
    }
    printf("✓ GPIO pins claimed\n");
    
    // Open I2C bus
    i2c_fd = open(I2C_BUS, O_RDWR);
    if (i2c_fd < 0) {
        fprintf(stderr, "Failed to open I2C bus %s\n", I2C_BUS);
        fprintf(stderr, "Make sure I2C is enabled\n");
        lgGpiochipClose(gpio_chip);
        return 1;
    }
    printf("✓ I2C bus opened\n");
    
    // Set I2C slave address
    if (ioctl(i2c_fd, I2C_SLAVE, PCA9685_ADDRESS) < 0) {
        fprintf(stderr, "Failed to set I2C slave address to 0x%02X\n", PCA9685_ADDRESS);
        fprintf(stderr, "Is PCA9685 connected?\n");
        close(i2c_fd);
        lgGpiochipClose(gpio_chip);
        return 1;
    }
    printf("✓ I2C slave address set to 0x%02X\n", PCA9685_ADDRESS);
    
    // Initialize PCA9685
    pca9685_init(i2c_fd);

    // Initialize all PWM channels to stopped state (critical for active-LOW enables)
    printf("Initializing all PWM channels to stopped state...\n");
    for (int ch = 0; ch < 16; ch++) {
        pca9685_set_pwm(i2c_fd, ch, 0);  // 0 = stopped (HIGH voltage for active-LOW)
    }
    printf("✓ All PWM channels initialized to stopped\n");

    printf("\n=== Hardware Check Complete ===\n");
    printf("\n⚠️  CRITICAL: Before testing, verify:\n");
    printf("   1. L298N enable pin JUMPERS are REMOVED!\n");
    printf("   2. PCA9685 channels 0-3 are connected to L298N ENA/ENB pins\n");
    printf("   3. Motor power supply is connected (6-12V)\n");
    printf("   4. All grounds are connected together\n");
    printf("\nStarting motor tests in 3 seconds...\n");
    printf("Each motor will spin at FULL SPEED forward then backward.\n");
    printf("Press Ctrl+C to stop at any time.\n\n");
    sleep(3);
    
    // Test each motor individually
    test_motor("Front Left Motor (ENA_FRONT, IN3/IN4)", IN3_FRONT, IN4_FRONT, ENA_FRONT);
    sleep(1);
    
    test_motor("Front Right Motor (ENB_FRONT, IN1/IN2)", IN1_FRONT, IN2_FRONT, ENB_FRONT);
    sleep(1);
    
    test_motor("Rear Left Motor (ENA_REAR, IN3/IN4)", IN3_REAR, IN4_REAR, ENA_REAR);
    sleep(1);
    
    test_motor("Rear Right Motor (ENB_REAR, IN1/IN2)", IN1_REAR, IN2_REAR, ENB_REAR);
    sleep(1);
    
    printf("\n=== All Motors Forward Test ===\n");
    printf("All motors forward for 3 seconds...\n");
    lgGpioWrite(gpio_chip, IN1_FRONT, 1);
    lgGpioWrite(gpio_chip, IN2_FRONT, 0);
    lgGpioWrite(gpio_chip, IN3_FRONT, 1);
    lgGpioWrite(gpio_chip, IN4_FRONT, 0);
    lgGpioWrite(gpio_chip, IN1_REAR, 1);
    lgGpioWrite(gpio_chip, IN2_REAR, 0);
    lgGpioWrite(gpio_chip, IN3_REAR, 1);
    lgGpioWrite(gpio_chip, IN4_REAR, 0);
    change_speed(TEST_SPEED);
    sleep(3);
    
    stop_all();
    
    printf("\n=== Motor Test Complete! ===\n");
    printf("\n🔍 DIAGNOSTIC RESULTS:\n\n");
    printf("❌ If motors didn't move AT ALL (most common issues):\n");
    printf("   1. ⚠️  L298N enable pin JUMPERS still ON\n");
    printf("      → REMOVE the jumpers from ENA and ENB pins!\n");
    printf("      → Connect PCA9685 PWM outputs to these pins instead\n");
    printf("   2. Motor power supply not connected (need 6-12V separate supply)\n");
    printf("   3. PCA9685 PWM outputs not wired to L298N enable pins\n");
    printf("   4. Motor power supply insufficient voltage/current\n");
    printf("   5. All grounds not connected together\n\n");
    printf("✅ If motors moved: Your hardware is working!\n");
    printf("   → Recompile line_tracking and test: make clean && make && sudo ./line_tracking\n");
    
    // Cleanup
    close(i2c_fd);
    lgGpiochipClose(gpio_chip);
    
    return 0;
}
