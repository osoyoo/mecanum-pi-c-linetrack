# Makefile for Mecanum Robot Controller
# Raspberry Pi OS Trixie (Debian 13)

# Compiler and flags
CC = gcc
CFLAGS = -Wall -O2
LIBS = -llgpio

# Target executables
TARGET = mecanum5
LINE_TRACKING = line_tracking

# Source files
SRC = mecanum5.c
LINE_TRACKING_SRC = line_tracking.c

# Default target - build all programs
all: $(TARGET) $(LINE_TRACKING)

# Build the mecanum demo executable
$(TARGET): $(SRC)
	@echo "Compiling $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)
	@echo "Build complete! Run with: sudo ./$(TARGET)"

# Build the line tracking executable
$(LINE_TRACKING): $(LINE_TRACKING_SRC)
	@echo "Compiling $(LINE_TRACKING)..."
	$(CC) $(CFLAGS) -o $(LINE_TRACKING) $(LINE_TRACKING_SRC) $(LIBS)
	@echo "Build complete! Run with: sudo ./$(LINE_TRACKING)"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGET) $(LINE_TRACKING)
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
	@echo "Mecanum Robot Controller - Makefile Commands"
	@echo "=============================================="
	@echo "make                 - Build all programs"
	@echo "make mecanum5        - Build mecanum demo only"
	@echo "make line_tracking   - Build line tracking only"
	@echo "make clean           - Remove built files"
	@echo "make install-deps    - Install required libraries (needs sudo)"
	@echo "make run             - Build and run mecanum demo (needs sudo)"
	@echo "make help            - Show this help message"
	@echo ""
	@echo "Manual commands:"
	@echo "  Compile demo:     gcc -o mecanum5 mecanum5.c -llgpio"
	@echo "  Compile tracking: gcc -o line_tracking line_tracking.c -llgpio"
	@echo "  Run demo:         sudo ./mecanum5"
	@echo "  Run tracking:     sudo ./line_tracking"

.PHONY: all clean install-deps run help
