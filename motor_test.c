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

// Test speeds - HIGHER for testing
#define TEST_SPEED  0x8FFF  // 56% max - should definitely be visible

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
    i2c_write_byte(fd, MODE1, 0x00);
    usleep(10000);
    
    uint8_t prescale = 101;  // 60Hz
    int oldmode = i2c_read_byte(fd, MODE1);
    int newmode = (oldmode & 0x7F) | 0x10;
    i2c_write_byte(fd, MODE1, newmode);
    usleep(2000);
    i2c_write_byte(fd, PRESCALE, prescale);
    usleep(2000);
    i2c_write_byte(fd, MODE1, oldmode);
    usleep(10000);
    i2c_write_byte(fd, MODE1, oldmode | 0x20);
    usleep(5000);
    printf("PCA9685 initialized!\n");
}

void pca9685_set_pwm(int fd, uint8_t channel, uint16_t value) {
    uint16_t pwm_value = value >> 4;
    uint8_t reg = LED0_ON_L + 4 * channel;
    uint8_t buf[5];
    buf[0] = reg;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = pwm_value & 0xFF;
    buf[4] = (pwm_value >> 8) & 0x0F;
    
    if (write(fd, buf, 5) != 5) {
        fprintf(stderr, "ERROR: Failed to write PWM to channel %d\n", channel);
    } else {
        printf("  PWM channel %d set to 0x%04X (scaled: 0x%03X)\n", channel, value, pwm_value);
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
    
    printf("\n=== Hardware Check Complete ===\n");
    printf("Starting motor tests in 2 seconds...\n");
    printf("Each motor will spin forward then backward.\n");
    printf("Press Ctrl+C to stop at any time.\n\n");
    sleep(2);
    
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
    printf("\nDiagnostic Information:\n");
    printf("- If motors didn't move, check:\n");
    printf("  1. L298N motor power supply connected?\n");
    printf("  2. PCA9685 channels connected to L298N enable pins (ENA/ENB)?\n");
    printf("  3. Motor wires connected?\n");
    printf("  4. All grounds connected together?\n");
    printf("- If motors moved: Your hardware is working!\n");
    printf("  The issue might be with sensor logic or speed values.\n");
    
    // Cleanup
    close(i2c_fd);
    lgGpiochipClose(gpio_chip);
    
    return 0;
}
