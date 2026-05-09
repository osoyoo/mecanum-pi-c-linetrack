# Makefile for Mecanum Wheel Line Tracking Robot
# Raspberry Pi OS (Bookworm or newer)

# Compiler and flags
CC = gcc
CFLAGS = -Wall -O2
LIBS = -llgpio

# Target executable
TARGET = line_tracking

# Source files
SRC = line_tracking.c

# Default target - build the line tracking program
all: $(TARGET)

# Build the line tracking executable
$(TARGET): $(SRC)
	@echo "Compiling $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)
	@echo "Build complete! Run with: sudo ./$(TARGET)"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGET)
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
	@echo "make clean           - Remove built files"
	@echo "make install-deps    - Install required libraries (needs sudo)"
	@echo "make run             - Build and run line tracking (needs sudo)"
	@echo "make help            - Show this help message"
	@echo ""
	@echo "Manual commands:"
	@echo "  Compile: gcc -o line_tracking line_tracking.c -llgpio"
	@echo "  Run:     sudo ./line_tracking"

.PHONY: all clean install-deps run help
