# Makefile for Mecanum Robot Controller
# Raspberry Pi OS Trixie (Debian 13)

# Compiler and flags
CC = gcc
CFLAGS = -Wall -O2
LIBS = -llgpio

# Target executable
TARGET = mecanum5

# Source files
SRC = mecanum5.c

# Default target
all: $(TARGET)

# Build the executable
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
	@echo "Mecanum Robot Controller - Makefile Commands"
	@echo "=============================================="
	@echo "make                 - Build the program"
	@echo "make clean           - Remove built files"
	@echo "make install-deps    - Install required libraries (needs sudo)"
	@echo "make run             - Build and run the program (needs sudo)"
	@echo "make help            - Show this help message"
	@echo ""
	@echo "Manual commands:"
	@echo "  Compile: gcc -o mecanum5 mecanum5.c -llgpio"
	@echo "  Run:     sudo ./mecanum5"

.PHONY: all clean install-deps run help
