/*
 * Mecanum Wheel Robot Line Tracking Controller
 *
 * This program enables a mecanum wheel robot to follow a black line on white ground
 * using 5 IR sensors:
 * - Sensors return 0 for black, 1 for white
 * - Sensor arrangement: S0(left) - S1 - S2(center) - S3 - S4(right)
 *
 * REQUIRED LIBRARIES:
 * 1. lgpio - Modern GPIO library for Raspberry Pi
 * 2. i2c-dev - For I2C communication with PCA9685
 *
 * COMPILATION:
 *   gcc -o line_tracking line_tracking.c -llgpio
 *
 * EXECUTION:
 *   sudo ./line_tracking
 *   (sudo is required for GPIO and I2C access)
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
#define LED0_ON_H           0x07
#define LED0_OFF_L          0x08
#define LED0_OFF_H          0x09

// I2C Bus
#define I2C_BUS             "/dev/i2c-1"

// Motor GPIO Pin Definitions (BCM numbering)
#define IN1_FRONT   23
#define IN2_FRONT   24
#define IN3_FRONT   27
#define IN4_FRONT   22
#define IN1_REAR    21
#define IN2_REAR    20
#define IN3_REAR    16
#define IN4_REAR    12

// PCA9685 PWM Channel Assignments
#define ENA_FRONT   0
#define ENB_FRONT   1
#define ENA_REAR    2
#define ENB_REAR    3

// IR Sensor GPIO Pin Definitions (BCM numbering)
// These are the actual GPIO pins used for the 5 IR sensors
#define IR_SENSOR_0  5   // Left-most sensor
#define IR_SENSOR_1  6   // Left-center sensor
#define IR_SENSOR_2  13  // Center sensor
#define IR_SENSOR_3  19  // Right-center sensor
#define IR_SENSOR_4  26  // Right-most sensor

// Speed settings for line tracking
#define BASE_SPEED      0x3FFF  // Base speed for forward movement (25% max)
#define TURN_SPEED      0x2FFF  // Speed for gentle turns (18% max)
#define SHARP_TURN_SPEED 0x2000 // Speed for sharp turns (12% max)
#define SEARCH_SPEED    0x1FFF  // Speed when searching for line (12% max)

// Global variables
int i2c_fd;
int gpio_chip;

/*
 * I2C Helper Functions
 */
int i2c_write_byte(int fd, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    int result = write(fd, buf, 2);
    if (result != 2) {
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

/*
 * PCA9685 PWM Controller Functions
 */
void pca9685_init(int fd) {
    i2c_write_byte(fd, MODE1, 0x00);
    usleep(10000);

    uint8_t prescale = 101;  // 60Hz PWM frequency
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
    }
}

void change_speed(uint16_t speed) {
    pca9685_set_pwm(i2c_fd, ENA_FRONT, speed);
    pca9685_set_pwm(i2c_fd, ENB_FRONT, speed);
    pca9685_set_pwm(i2c_fd, ENA_REAR, speed);
    pca9685_set_pwm(i2c_fd, ENB_REAR, speed);
}

/*
 * Motor Control Functions
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

void rr_ahead(void) {
    lgGpioWrite(gpio_chip, IN1_REAR, 1);
    lgGpioWrite(gpio_chip, IN2_REAR, 0);
}

void rr_back(void) {
    lgGpioWrite(gpio_chip, IN1_REAR, 0);
    lgGpioWrite(gpio_chip, IN2_REAR, 1);
}

void rl_ahead(void) {
    lgGpioWrite(gpio_chip, IN3_REAR, 1);
    lgGpioWrite(gpio_chip, IN4_REAR, 0);
}

void rl_back(void) {
    lgGpioWrite(gpio_chip, IN3_REAR, 0);
    lgGpioWrite(gpio_chip, IN4_REAR, 1);
}

void fr_ahead(void) {
    lgGpioWrite(gpio_chip, IN1_FRONT, 1);
    lgGpioWrite(gpio_chip, IN2_FRONT, 0);
}

void fr_back(void) {
    lgGpioWrite(gpio_chip, IN1_FRONT, 0);
    lgGpioWrite(gpio_chip, IN2_FRONT, 1);
}

void fl_ahead(void) {
    lgGpioWrite(gpio_chip, IN3_FRONT, 1);
    lgGpioWrite(gpio_chip, IN4_FRONT, 0);
}

void fl_back(void) {
    lgGpioWrite(gpio_chip, IN3_FRONT, 0);
    lgGpioWrite(gpio_chip, IN4_FRONT, 1);
}

/*
 * Movement Functions
 */
void go_ahead(uint16_t speed) {
    rl_ahead();
    rr_ahead();
    fl_ahead();
    fr_ahead();
    change_speed(speed);
}

void go_back(uint16_t speed) {
    rr_back();
    rl_back();
    fr_back();
    fl_back();
    change_speed(speed);
}

void turn_right(uint16_t speed) {
    rl_ahead();
    rr_back();
    fl_ahead();
    fr_back();
    change_speed(speed);
}

void turn_left(uint16_t speed) {
    rr_ahead();
    rl_back();
    fr_ahead();
    fl_back();
    change_speed(speed);
}

/*
 * Gentle turn functions for line tracking
 * These make the car turn while still moving forward
 */
void gentle_turn_right(uint16_t left_speed, uint16_t right_speed) {
    // Left motors at full speed, right motors at reduced speed
    rl_ahead();
    fl_ahead();
    rr_ahead();
    fr_ahead();

    // Set different speeds for left and right sides
    pca9685_set_pwm(i2c_fd, ENA_FRONT, left_speed);   // Front Left
    pca9685_set_pwm(i2c_fd, ENA_REAR, left_speed);    // Rear Left
    pca9685_set_pwm(i2c_fd, ENB_FRONT, right_speed);  // Front Right
    pca9685_set_pwm(i2c_fd, ENB_REAR, right_speed);   // Rear Right
}

void gentle_turn_left(uint16_t left_speed, uint16_t right_speed) {
    // Right motors at full speed, left motors at reduced speed
    rl_ahead();
    fl_ahead();
    rr_ahead();
    fr_ahead();

    // Set different speeds for left and right sides
    pca9685_set_pwm(i2c_fd, ENA_FRONT, left_speed);   // Front Left
    pca9685_set_pwm(i2c_fd, ENA_REAR, left_speed);    // Rear Left
    pca9685_set_pwm(i2c_fd, ENB_FRONT, right_speed);  // Front Right
    pca9685_set_pwm(i2c_fd, ENB_REAR, right_speed);   // Rear Right
}

/*
 * IR Sensor Functions
 */
int read_sensor(int pin) {
    // Read GPIO pin (returns 0 for black, 1 for white)
    return lgGpioRead(gpio_chip, pin);
}

void read_all_sensors(int *sensors) {
    sensors[0] = read_sensor(IR_SENSOR_0);
    sensors[1] = read_sensor(IR_SENSOR_1);
    sensors[2] = read_sensor(IR_SENSOR_2);
    sensors[3] = read_sensor(IR_SENSOR_3);
    sensors[4] = read_sensor(IR_SENSOR_4);
}

void print_sensor_status(int *sensors) {
    printf("Sensors: [%d %d %d %d %d]  ",
           sensors[0], sensors[1], sensors[2], sensors[3], sensors[4]);
}

/*
 * Line Tracking Algorithm
 *
 * Calculates line position based on weighted sensor readings
 * Returns position value:
 *   -2: Far left (sensor 0 detects)
 *   -1: Left (sensor 1 detects)
 *    0: Center (sensor 2 detects)
 *   +1: Right (sensor 3 detects)
 *   +2: Far right (sensor 4 detects)
 *  100: Line lost (all white)
 *  200: Wide line or intersection (all black)
 */
int calculate_line_position(int *sensors) {
    // Count black sensors
    int black_count = 0;
    for (int i = 0; i < 5; i++) {
        if (sensors[i] == 0) black_count++;
    }

    // All sensors see white - line lost
    if (black_count == 0) {
        return 100;
    }

    // All or most sensors see black - wide line or intersection
    if (black_count >= 4) {
        return 200;
    }

    // Calculate weighted position
    // Weights: -2, -1, 0, +1, +2 for sensors 0-4
    int position = 0;
    int weight_sum = 0;
    int weights[] = {-2, -1, 0, 1, 2};

    for (int i = 0; i < 5; i++) {
        if (sensors[i] == 0) {  // Black detected
            position += weights[i];
            weight_sum++;
        }
    }

    // Return average weighted position
    if (weight_sum > 0) {
        return position / weight_sum;
    }

    return 0;  // Default to center
}

/*
 * Line Tracking Control Function
 * Takes action based on calculated line position
 */
void track_line(int position) {
    if (position == 100) {
        // Line lost - stop and search
        printf("LINE LOST - Stopping\n");
        stop_car();
        usleep(100000);  // Brief pause

        // You can add search pattern here if desired
        // For now, just stop

    } else if (position == 200) {
        // Wide line or intersection - go straight
        printf("Wide line/Intersection - Going straight\n");
        go_ahead(BASE_SPEED);

    } else if (position == 0) {
        // Perfect center - go straight
        printf("Center - Going straight\n");
        go_ahead(BASE_SPEED);

    } else if (position == -1) {
        // Slight left - gentle left turn
        printf("Slight left - Gentle turn left\n");
        gentle_turn_left(TURN_SPEED, BASE_SPEED);

    } else if (position == -2) {
        // Far left - sharp left turn
        printf("Far left - Sharp turn left\n");
        turn_left(SHARP_TURN_SPEED);

    } else if (position == 1) {
        // Slight right - gentle right turn
        printf("Slight right - Gentle turn right\n");
        gentle_turn_right(BASE_SPEED, TURN_SPEED);

    } else if (position == 2) {
        // Far right - sharp right turn
        printf("Far right - Sharp turn right\n");
        turn_right(SHARP_TURN_SPEED);
    }
}

/*
 * Main Function - Line Tracking Loop
 */
int main(void) {
    printf("=== Mecanum Wheel Line Tracking Robot ===\n");
    printf("Press Ctrl+C to stop\n\n");

    // Open GPIO chip
    gpio_chip = lgGpiochipOpen(0);
    if (gpio_chip < 0) {
        fprintf(stderr, "Failed to open GPIO chip\n");
        fprintf(stderr, "Make sure you run this program with sudo\n");
        return 1;
    }

    // Claim motor control GPIO pins as outputs
    int motor_pins[] = {IN1_FRONT, IN2_FRONT, IN3_FRONT, IN4_FRONT,
                        IN1_REAR, IN2_REAR, IN3_REAR, IN4_REAR};
    for (int i = 0; i < 8; i++) {
        if (lgGpioClaimOutput(gpio_chip, 0, motor_pins[i], 0) < 0) {
            fprintf(stderr, "Failed to claim motor GPIO pin %d\n", motor_pins[i]);
            lgGpiochipClose(gpio_chip);
            return 1;
        }
    }

    // Claim IR sensor GPIO pins as inputs
    int sensor_pins[] = {IR_SENSOR_0, IR_SENSOR_1, IR_SENSOR_2, IR_SENSOR_3, IR_SENSOR_4};
    for (int i = 0; i < 5; i++) {
        if (lgGpioClaimInput(gpio_chip, 0, sensor_pins[i]) < 0) {
            fprintf(stderr, "Failed to claim IR sensor GPIO pin %d\n", sensor_pins[i]);
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
    printf("Initialization complete!\n");
    printf("Starting line tracking in 2 seconds...\n\n");
    sleep(2);

    // Main line tracking loop
    int sensors[5];
    int loop_count = 0;

    while (1) {
        // Read all sensors
        read_all_sensors(sensors);

        // Calculate line position
        int position = calculate_line_position(sensors);

        // Print status every 10 loops to reduce console spam
        if (loop_count % 10 == 0) {
            print_sensor_status(sensors);
            printf("Position: %d\n", position);
        }

        // Execute tracking control
        track_line(position);

        // Small delay for stability (50ms = 20Hz update rate)
        usleep(50000);

        loop_count++;
    }

    // Cleanup (won't reach here unless you modify the while loop)
    stop_car();
    close(i2c_fd);
    lgGpiochipClose(gpio_chip);

    return 0;
}
