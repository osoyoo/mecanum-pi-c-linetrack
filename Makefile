# Makefile for Mecanum Wheel Line Tracking Robot
# Raspberry Pi OS (Bookworm or newer)

# Compiler and flags
CC = gcc
CFLAGS = -Wall -O2
LIBS = -llgpio

# Target executables
TARGET = line_tracking
MOTOR_TEST = motor_test

# Source files
SRC = line_tracking.c
MOTOR_TEST_SRC = motor_test.c

# Default target - build the line tracking program
all: $(TARGET)

# Build all including motor test
all-with-test: $(TARGET) $(MOTOR_TEST)

# Build the line tracking executable
$(TARGET): $(SRC)
	@echo "Compiling $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)
	@echo "Build complete! Run with: sudo ./$(TARGET)"

# Build the motor test executable
$(MOTOR_TEST): $(MOTOR_TEST_SRC)
	@echo "Compiling $(MOTOR_TEST)..."
	$(CC) $(CFLAGS) -o $(MOTOR_TEST) $(MOTOR_TEST_SRC) $(LIBS)
	@echo "Build complete! Run with: sudo ./$(MOTOR_TEST)"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGET) $(MOTOR_TEST)
	@echo "Clean complete!"

# Install dependencies (requires sudo)
install-deps:
	@echo "Installing dependencies..."
	@if [ "$$EUID" -ne 0 ]; then \
		echo "Please run with sudo: sudo make install-deps"; \
		exit 1; \
	fi
	chmod +x install_dependencies.sh
	./install_dependencies.sh

# Run the program (requires sudo)
run: $(TARGET)
	@echo "Running $(TARGET)..."
	sudo ./$(TARGET)

# Help target
help:
	@echo "Mecanum Wheel Line Tracking Robot - Makefile Commands"
	@echo "======================================================"
	@echo "make                 - Build the line tracking program"
	@echo "make motor_test      - Build motor diagnostic test"
	@echo "make all-with-test   - Build both programs"
	@echo "make clean           - Remove built files"
	@echo "make install-deps    - Install required libraries (needs sudo)"
	@echo "make run             - Build and run line tracking (needs sudo)"
	@echo "make help            - Show this help message"
	@echo ""
	@echo "Diagnostic commands:"
	@echo "  sudo ./motor_test    - Test motors (run this FIRST if motors don't work)"
	@echo ""
	@echo "Manual commands:"
	@echo "  Compile: gcc -o line_tracking line_tracking.c -llgpio"
	@echo "  Run:     sudo ./line_tracking"

.PHONY: all all-with-test clean install-deps run help
