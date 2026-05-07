/*
 * Mecanum Wheel Robot Controller in C for Raspberry Pi (Trixie OS)
 *
 * This program controls a 4-wheeled mecanum robot using:
 * - GPIO pins for motor direction control (via L298N motor driver)
 * - PCA9685 PWM controller for motor speed control via I2C
 *
 * REQUIRED LIBRARIES:
 * 1. lgpio - Modern GPIO library for Raspberry Pi (replaces deprecated wiringPi)
 * 2. i2c-dev - For I2C communication with PCA9685
 *
 * INSTALLATION:
 * Run the included install_dependencies.sh script to install all required libraries:
 *   chmod +x install_dependencies.sh
 *   sudo ./install_dependencies.sh
 *
 * COMPILATION:
 *   gcc -o mecanum5 mecanum5.c -llgpio
 *
 * EXECUTION:
 *   sudo ./mecanum5
 *   (sudo is required for GPIO and I2C access)
 *
 * IMPORTANT - POWER SUPPLY REQUIREMENTS:
 * Running 4 motors simultaneously draws significant current. If your Raspberry Pi
 * reboots or crashes during turns/movement, you have insufficient power:
 *
 * 1. Use a quality power supply for the Raspberry Pi (5V, 3A minimum recommended)
 * 2. Motors should have a SEPARATE power supply (do NOT power from Pi)
 * 3. Connect all grounds together (Pi GND, motor power GND, PCA9685 GND)
 * 4. Motor speeds are reduced in this code (MOVE_SPEED, TURN_SPEED) to prevent
 *    voltage drops that can reboot the Pi. Increase speeds only if you have
 *    adequate power supply with proper decoupling capacitors.
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
#define PCA9685_ADDRESS     0x40        // Default I2C address for PCA9685
#define MODE1               0x00        // Mode register 1
#define MODE2               0x01        // Mode register 2
#define PRESCALE            0xFE        // Prescaler for PWM output frequency
#define LED0_ON_L           0x06        // LED0 output and brightness control byte 0
#define LED0_ON_H           0x07        // LED0 output and brightness control byte 1
#define LED0_OFF_L          0x08        // LED0 output and brightness control byte 2
#define LED0_OFF_H          0x09        // LED0 output and brightness control byte 3

// I2C Bus (usually /dev/i2c-1 on Raspberry Pi)
#define I2C_BUS             "/dev/i2c-1"

// GPIO Pin Definitions (using BCM numbering)
// FRONT motors connected to L298N driver
#define IN1_FRONT   23      // Front Right motor (BK1 port) direction pin
#define IN2_FRONT   24      // Front Right motor (BK1 port) direction pin
#define IN3_FRONT   27      // Front Left motor (BK3 port) direction pin
#define IN4_FRONT   22      // Front Left motor (BK3 port) direction pin

// REAR motors connected to L298N driver
#define IN1_REAR    21      // Rear Right motor (AK1 port) direction pin
#define IN2_REAR    20      // Rear Right motor (AK1 port) direction pin
#define IN3_REAR    16      // Rear Left motor (AK3 port) direction pin
#define IN4_REAR    12      // Rear Left motor (AK3 port) direction pin

// PCA9685 PWM Channel Assignments for motor speed control
#define ENA_FRONT   0       // Front Left motor speed (PCA9685 channel 0)
#define ENB_FRONT   1       // Front Right motor speed (PCA9685 channel 1)
#define ENA_REAR    2       // Rear Left motor speed (PCA9685 channel 2)
#define ENB_REAR    3       // Rear Right motor speed (PCA9685 channel 3)

// Motor speeds - adjusted to prevent power brown-out
// When all 4 motors run simultaneously, current draw can cause voltage drop
// and reboot the Raspberry Pi if power supply is insufficient
#define MOVE_SPEED  0x4FFF  // Normal movement speed (31% max) - safe for most power supplies
#define TURN_SPEED  0x2FFF  // Reduced speed for turns (18% max) - extra safe for rotations

// Global variables
int i2c_fd;         // I2C file descriptor
int gpio_chip;      // GPIO chip handle

/*
 * Write a byte to an I2C register
 */
int i2c_write_byte(int fd, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    int result = write(fd, buf, 2);
    if (result != 2) {
        fprintf(stderr, "ERROR: i2c_write_byte failed for reg 0x%02X (returned %d)\n", reg, result);
        perror("  write() error");
        return -1;
    }
    return 0;
}

/*
 * Read a byte from an I2C register
 */
int i2c_read_byte(int fd, uint8_t reg) {
    uint8_t value;
    if (write(fd, &reg, 1) != 1) {
        return -1;
    }
    if (read(fd, &value, 1) != 1) {
        return -1;
    }
    return value;
}

/*
 * Initialize PCA9685 PWM controller
 * Sets up I2C communication and configures the chip for 60Hz PWM frequency
 * This frequency is ideal for motor control
 */
void pca9685_init(int fd) {
    // Reset the PCA9685 MODE1 register
    i2c_write_byte(fd, MODE1, 0x00);
    usleep(10000);  // Wait for reset

    // Set PWM frequency to 60Hz (good for motors)
    // Calculation: prescale = round(25MHz / (4096 * frequency)) - 1
    // For 60Hz: prescale = round(25000000 / (4096 * 60)) - 1 = 101
    uint8_t prescale = 101;

    // Enter sleep mode to set prescale
    int oldmode = i2c_read_byte(fd, MODE1);
    int newmode = (oldmode & 0x7F) | 0x10;    // Sleep
    i2c_write_byte(fd, MODE1, newmode);        // Go to sleep
    usleep(2000);                              // Wait for sleep mode
    i2c_write_byte(fd, PRESCALE, prescale);    // Set prescale
    usleep(2000);                              // Wait for prescale to take effect
    i2c_write_byte(fd, MODE1, oldmode);        // Restore mode
    usleep(10000);                             // Wait for oscillator

    // Enable auto-increment (bit 5 = 0x20, NOT bit 7 = 0x80!)
    // Bit 5 (0x20) = AI (Auto-Increment) - required for multi-byte writes
    // Bit 7 (0x80) = RESTART - different function!
    i2c_write_byte(fd, MODE1, oldmode | 0x20);  // CRITICAL: 0x20 not 0x80!
    usleep(5000);                               // Wait for settings to stabilize
}

/*
 * Set PWM duty cycle for a specific channel on PCA9685
 * channel: PWM channel number (0-15)
 * value: Duty cycle value (0-0xFFFF, where 0xFFFF is 100% duty cycle)
 *
 * NOTE: PCA9685 has 12-bit resolution (0-4095), so we scale the 16-bit
 * input value down to 12-bit to match the Python adafruit_pca9685 library behavior
 */
void pca9685_set_pwm(int fd, uint8_t channel, uint16_t value) {
    // Scale 16-bit value (0-65535) to 12-bit (0-4095)
    // Python adafruit_pca9685 library does: value >> 4 (divide by 16)
    // This converts 16-bit (0-65535) to 12-bit (0-4095)
    uint16_t pwm_value = value >> 4;  // Right-shift by 4 bits = divide by 16

    // Uncomment for debugging:
    // printf("  [DEBUG] Setting channel %d: input=0x%04X, scaled=0x%03X (%d)\n",
    //        channel, value, pwm_value, pwm_value);

    // Each channel has 4 registers: ON_L, ON_H, OFF_L, OFF_H
    // We set ON to 0 and OFF to the desired value for simple PWM
    uint8_t reg = LED0_ON_L + 4 * channel;

    // Write all 4 bytes in a single I2C transaction (using auto-increment)
    // Format: [register_address, ON_L, ON_H, OFF_L, OFF_H]
    uint8_t buf[5];
    buf[0] = reg;                           // Starting register address
    buf[1] = 0;                             // LED_ON_L = 0
    buf[2] = 0;                             // LED_ON_H = 0
    buf[3] = pwm_value & 0xFF;              // LED_OFF_L (low byte)
    buf[4] = (pwm_value >> 8) & 0x0F;       // LED_OFF_H (high byte, masked to 12-bit)

    int result = write(fd, buf, 5);
    if (result != 5) {
        fprintf(stderr, "ERROR: Failed to write PWM to channel %d (returned %d)\n", channel, result);
        perror("  I2C write error");
        return;  // Don't hang - just return on error
    }

    // No delay - test without delays to see if that's causing the hang
}

/*
 * Change speed of all 4 motors simultaneously
 * speed: PWM duty cycle value (0-0xFFFF)
 */
void change_speed(uint16_t speed) {
    pca9685_set_pwm(i2c_fd, ENA_FRONT, speed);
    pca9685_set_pwm(i2c_fd, ENB_FRONT, speed);
    pca9685_set_pwm(i2c_fd, ENA_REAR, speed);
    pca9685_set_pwm(i2c_fd, ENB_REAR, speed);
}

/*
 * Stop all motors
 * Sets all direction pins LOW and speed to 0
 */
void stop_car(void) {
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

// Individual motor control functions
// For each motor, one direction pin HIGH (1) and one LOW (0) determines direction

void rr_ahead(void) {  // Rear Right motor forward
    lgGpioWrite(gpio_chip, IN1_REAR, 1);
    lgGpioWrite(gpio_chip, IN2_REAR, 0);
}

void rr_back(void) {   // Rear Right motor backward
    lgGpioWrite(gpio_chip, IN1_REAR, 0);
    lgGpioWrite(gpio_chip, IN2_REAR, 1);
}

void rl_ahead(void) {  // Rear Left motor forward
    lgGpioWrite(gpio_chip, IN3_REAR, 1);
    lgGpioWrite(gpio_chip, IN4_REAR, 0);
}

void rl_back(void) {   // Rear Left motor backward
    lgGpioWrite(gpio_chip, IN3_REAR, 0);
    lgGpioWrite(gpio_chip, IN4_REAR, 1);
}

void fr_ahead(void) {  // Front Right motor forward
    lgGpioWrite(gpio_chip, IN1_FRONT, 1);
    lgGpioWrite(gpio_chip, IN2_FRONT, 0);
}

void fr_back(void) {   // Front Right motor backward
    lgGpioWrite(gpio_chip, IN1_FRONT, 0);
    lgGpioWrite(gpio_chip, IN2_FRONT, 1);
}

void fl_ahead(void) {  // Front Left motor forward
    lgGpioWrite(gpio_chip, IN3_FRONT, 1);
    lgGpioWrite(gpio_chip, IN4_FRONT, 0);
}

void fl_back(void) {   // Front Left motor backward
    lgGpioWrite(gpio_chip, IN3_FRONT, 0);
    lgGpioWrite(gpio_chip, IN4_FRONT, 1);
}

/*
 * Mecanum wheel movement functions
 * By controlling each wheel independently, mecanum wheels enable:
 * - Forward/backward movement
 * - Rotation in place
 * - Lateral (sideways) movement
 * - Diagonal movement
 */

// Move forward - all wheels rotate forward
void go_ahead(uint16_t speed) {
    rl_ahead();
    rr_ahead();
    fl_ahead();
    fr_ahead();
    change_speed(speed);
}

// Move backward - all wheels rotate backward
void go_back(uint16_t speed) {
    rr_back();
    rl_back();
    fr_back();
    fl_back();
    change_speed(speed);
}

// Rotate right (clockwise) - left wheels forward, right wheels backward
void turn_right(uint16_t speed) {
    rl_ahead();
    rr_back();
    fl_ahead();
    fr_back();
    change_speed(speed);
}

// Rotate left (counter-clockwise) - right wheels forward, left wheels backward
void turn_left(uint16_t speed) {
    rr_ahead();
    rl_back();
    fr_ahead();
    fl_back();
    change_speed(speed);
}

// Shift left (parallel lateral movement) - diagonal wheels work together
void shift_left(uint16_t speed) {
    fr_ahead();
    rr_back();
    rl_ahead();
    fl_back();
    change_speed(speed);
}

// Shift right (parallel lateral movement) - diagonal wheels work together
void shift_right(uint16_t speed) {
    fr_back();
    rr_ahead();
    rl_back();
    fl_ahead();
    change_speed(speed);
}

// Move diagonally upper-right - only rear-right and front-left wheels active
void upper_right(uint16_t speed) {
    rr_ahead();
    fl_ahead();
    change_speed(speed);
}

// Move diagonally lower-left - opposite of upper-right
void lower_left(uint16_t speed) {
    rr_back();
    fl_back();
    change_speed(speed);
}

// Move diagonally upper-left - only front-right and rear-left wheels active
void upper_left(uint16_t speed) {
    fr_ahead();
    rl_ahead();
    change_speed(speed);
}

// Move diagonally lower-right - opposite of upper-left
void lower_right(uint16_t speed) {
    fr_back();
    rl_back();
    change_speed(speed);
}

/*
 * Main function - Demonstrates all movement capabilities
 */
int main(void) {
    printf("Initializing Mecanum Wheel Robot for Raspberry Pi...\n");

    // Open GPIO chip (gpiochip0 for Raspberry Pi)
    gpio_chip = lgGpiochipOpen(0);
    if (gpio_chip < 0) {
        fprintf(stderr, "Failed to open GPIO chip\n");
        fprintf(stderr, "Make sure you run this program with sudo\n");
        return 1;
    }

    // Claim all GPIO pins as outputs
    int pins[] = {IN1_FRONT, IN2_FRONT, IN3_FRONT, IN4_FRONT,
                  IN1_REAR, IN2_REAR, IN3_REAR, IN4_REAR};
    for (int i = 0; i < 8; i++) {
        if (lgGpioClaimOutput(gpio_chip, 0, pins[i], 0) < 0) {
            fprintf(stderr, "Failed to claim GPIO pin %d\n", pins[i]);
            lgGpiochipClose(gpio_chip);
            return 1;
        }
    }

    // Open I2C bus
    i2c_fd = open(I2C_BUS, O_RDWR);
    if (i2c_fd < 0) {
        fprintf(stderr, "Failed to open I2C bus %s\n", I2C_BUS);
        fprintf(stderr, "Make sure I2C is enabled: sudo raspi-config -> Interface Options -> I2C\n");
        lgGpiochipClose(gpio_chip);
        return 1;
    }

    // Set I2C slave address
    if (ioctl(i2c_fd, I2C_SLAVE, PCA9685_ADDRESS) < 0) {
        fprintf(stderr, "Failed to set I2C slave address to 0x%02X\n", PCA9685_ADDRESS);
        close(i2c_fd);
        lgGpiochipClose(gpio_chip);
        return 1;
    }

    // Initialize PCA9685 PWM controller
    pca9685_init(i2c_fd);
    printf("Initialization complete!\n\n");

    // Demonstration sequence - test all movement capabilities

    printf("Moving ahead...\n");
    go_ahead(MOVE_SPEED);
    sleep(1);
    stop_car();
    usleep(500000);  // 0.5 second pause

    printf("Moving backward...\n");
    go_back(MOVE_SPEED);
    sleep(1);
    stop_car();
    usleep(500000);

    printf("Shifting left...\n");
    shift_left(MOVE_SPEED);
    sleep(1);
    stop_car();
    usleep(500000);

    printf("Shifting right...\n");
    shift_right(MOVE_SPEED);
    sleep(1);
    stop_car();
    usleep(500000);

    printf("Moving upper-left (diagonal)...\n");
    upper_left(MOVE_SPEED);
    sleep(1);
    stop_car();
    usleep(500000);

    printf("Moving lower-right (diagonal)...\n");
    lower_right(MOVE_SPEED);
    sleep(1);
    stop_car();
    usleep(500000);

    printf("Moving upper-right (diagonal)...\n");
    upper_right(MOVE_SPEED);
    sleep(1);
    stop_car();
    usleep(500000);

    printf("Moving lower-left (diagonal)...\n");
    lower_left(MOVE_SPEED);
    sleep(1);
    stop_car();
    usleep(500000);

    printf("Turning left (reduced speed)...\n");
    turn_left(TURN_SPEED);  // Use reduced speed to prevent voltage drop
    sleep(1);
    stop_car();

    printf("Turning right (reduced speed)...\n");
    turn_right(TURN_SPEED);  // Use reduced speed to prevent voltage drop
    sleep(1);
    stop_car();

    printf("\nDemo complete!\n");

    // Cleanup
    close(i2c_fd);
    lgGpiochipClose(gpio_chip);

    return 0;
}
